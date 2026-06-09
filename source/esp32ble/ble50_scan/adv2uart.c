#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define CMD_ID_INFO    0x00
#define CMD_ID_SCAN    0x01
#define CMD_ID_WMAC    0x02
#define CMD_ID_BMAC    0x03
#define CMD_ID_CLRM    0x04
#define CMD_ID_VBAT    0x0F
#define CMD_ID_GPIOEVT 0x10

#define GPIOEVT_OP_QUERY   0
#define GPIOEVT_OP_ENABLE  1
#define GPIOEVT_OP_DISABLE 2
#define GPIOEVT_OP_CLEAR   3
#define GPIOEVT_EVENT_FLAG 0x80

#define MAX_ADV_PAYLOAD 229
#define HEAD_CRC_ADD_LEN 13
#define PARSER_BUF_CAP 16384

#define LOG_VERBOSE 5
#define LOG_DEBUG 10
#define LOG_INFO 20
#define LOG_WARNING 30
#define LOG_ERROR 40

static volatile sig_atomic_t g_stop = 0;
static int g_log_level = LOG_WARNING;

static void handle_sigint(int signum) {
    (void)signum;
    g_stop = 1;
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void sleep_ms(int ms) {
    if (ms <= 0) {
        return;
    }
    usleep((useconds_t)ms * 1000u);
}

static const char *level_name(int level) {
    switch (level) {
        case LOG_VERBOSE: return "VERBOSE";
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "LOG";
    }
}

static void log_msg(int level, const char *fmt, ...) {
    if (level < g_log_level) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "%s:root:", level_name(level));
    vfprintf(stdout, fmt, ap);
    fputc('\n', stdout);
    fflush(stdout);
    va_end(ap);
}

static uint16_t crc16_modbus(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    size_t i;
    for (i = 0; i < len; ++i) {
        int bit;
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 1u) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001u);
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void bytes_to_hex_lower(const uint8_t *src, size_t len, char *out, size_t out_sz) {
    static const char *hex = "0123456789abcdef";
    size_t i;
    size_t pos = 0;
    if (out_sz == 0) {
        return;
    }
    for (i = 0; i < len && pos + 2 < out_sz; ++i) {
        out[pos++] = hex[(src[i] >> 4) & 0x0F];
        out[pos++] = hex[src[i] & 0x0F];
    }
    out[pos] = '\0';
}

static void mac_wire_to_upper12(const uint8_t *wire6, char out[13]) {
    static const char *hex = "0123456789ABCDEF";
    int i;
    int p = 0;
    for (i = 5; i >= 0; --i) {
        out[p++] = hex[(wire6[i] >> 4) & 0x0F];
        out[p++] = hex[wire6[i] & 0x0F];
    }
    out[p] = '\0';
}

static bool parse_hex_digit(char c, uint8_t *v) {
    if (c >= '0' && c <= '9') {
        *v = (uint8_t)(c - '0');
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        *v = (uint8_t)(10 + c - 'a');
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        *v = (uint8_t)(10 + c - 'A');
        return true;
    }
    return false;
}

static bool normalize_mac(const char *text, char *normalized, size_t out_sz) {
    size_t i;
    size_t n = 0;
    if (out_sz < 13) {
        return false;
    }
    for (i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (isxdigit((unsigned char)c)) {
            if (n >= 12) {
                return false;
            }
            normalized[n++] = (char)toupper((unsigned char)c);
        }
    }
    if (n == 0 || (n % 2) != 0 || n > 12) {
        return false;
    }
    normalized[n] = '\0';
    return true;
}

static bool mac_to_wire(const char *text, uint8_t *out, size_t *out_len) {
    char norm[13];
    size_t n;
    size_t i;
    if (!normalize_mac(text, norm, sizeof(norm))) {
        return false;
    }
    n = strlen(norm) / 2;
    for (i = 0; i < n; ++i) {
        uint8_t hi, lo;
        if (!parse_hex_digit(norm[i * 2], &hi) || !parse_hex_digit(norm[i * 2 + 1], &lo)) {
            return false;
        }
        out[n - 1 - i] = (uint8_t)((hi << 4) | lo);
    }
    *out_len = n;
    return true;
}

typedef struct {
    char **items;
    size_t count;
    size_t cap;
} StringList;

static void strlist_init(StringList *lst) {
    memset(lst, 0, sizeof(*lst));
}

static void strlist_free(StringList *lst) {
    size_t i;
    for (i = 0; i < lst->count; ++i) {
        free(lst->items[i]);
    }
    free(lst->items);
    memset(lst, 0, sizeof(*lst));
}

static bool strlist_push(StringList *lst, const char *s) {
    char *dup;
    if (lst->count == lst->cap) {
        size_t new_cap = lst->cap == 0 ? 8 : lst->cap * 2;
        char **tmp = (char **)realloc(lst->items, new_cap * sizeof(char *));
        if (!tmp) {
            return false;
        }
        lst->items = tmp;
        lst->cap = new_cap;
    }
    dup = strdup(s);
    if (!dup) {
        return false;
    }
    lst->items[lst->count++] = dup;
    return true;
}

typedef struct {
    int *items;
    size_t count;
    size_t cap;
} IntList;

static void intlist_init(IntList *lst) {
    memset(lst, 0, sizeof(*lst));
}

static void intlist_free(IntList *lst) {
    free(lst->items);
    memset(lst, 0, sizeof(*lst));
}

static bool intlist_push(IntList *lst, int v) {
    if (lst->count == lst->cap) {
        size_t new_cap = lst->cap == 0 ? 8 : lst->cap * 2;
        int *tmp = (int *)realloc(lst->items, new_cap * sizeof(int));
        if (!tmp) {
            return false;
        }
        lst->items = tmp;
        lst->cap = new_cap;
    }
    lst->items[lst->count++] = v;
    return true;
}

static bool parse_mac_filters(const StringList *raw_values, StringList *out_macs, const char *opt_name) {
    size_t i;
    for (i = 0; i < raw_values->count; ++i) {
        char *work = strdup(raw_values->items[i]);
        char *tok;
        char *save = NULL;
        if (!work) {
            return false;
        }
        tok = strtok_r(work, ",;", &save);
        while (tok) {
            char norm[13];
            while (*tok == ' ' || *tok == '\t') {
                ++tok;
            }
            if (*tok != '\0') {
                if (!normalize_mac(tok, norm, sizeof(norm))) {
                    fprintf(stderr, "%s: MAC address must contain 2-12 even-count hexadecimal digits (1-6 bytes prefix): '%s'\n", opt_name, tok);
                    free(work);
                    return false;
                }
                if (!strlist_push(out_macs, norm)) {
                    free(work);
                    return false;
                }
            }
            tok = strtok_r(NULL, ",;", &save);
        }
        free(work);
    }
    return true;
}

static speed_t baud_to_speed(int baud) {
    switch (baud) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
#ifdef B230400
        case 230400: return B230400;
#endif
#ifdef B460800
        case 460800: return B460800;
#endif
#ifdef B500000
        case 500000: return B500000;
#endif
#ifdef B921600
        case 921600: return B921600;
#endif
#ifdef B1000000
        case 1000000: return B1000000;
#endif
#ifdef B1500000
        case 1500000: return B1500000;
#endif
#ifdef B2000000
        case 2000000: return B2000000;
#endif
        default: return 0;
    }
}

static int serial_set_dtr_rts(int fd, bool dtr_on, bool rts_on) {
#ifdef TIOCMGET
    int status;
    if (ioctl(fd, TIOCMGET, &status) != 0) {
        return -1;
    }
    if (dtr_on) {
        status |= TIOCM_DTR;
    } else {
        status &= ~TIOCM_DTR;
    }
    if (rts_on) {
        status |= TIOCM_RTS;
    } else {
        status &= ~TIOCM_RTS;
    }
    if (ioctl(fd, TIOCMSET, &status) != 0) {
        return -1;
    }
#else
    (void)fd; (void)dtr_on; (void)rts_on;
#endif
    return 0;
}

static int serial_open_port(const char *port, int baud, double timeout) {
    int fd;
    struct termios tio;
    speed_t speed = baud_to_speed(baud);
    if (speed == 0) {
        errno = EINVAL;
        return -1;
    }
    fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }
    if (tcgetattr(fd, &tio) != 0) {
        close(fd);
        return -1;
    }
    cfmakeraw(&tio);
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CRTSCTS;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = (cc_t)(timeout > 0 ? timeout * 10.0 : 0.0);
    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static ssize_t serial_read_timeout(int fd, uint8_t *buf, size_t len, double timeout) {
    fd_set rfds;
    struct timeval tv;
    int rc;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = (int)timeout;
    tv.tv_usec = (int)((timeout - (double)tv.tv_sec) * 1000000.0);
    if (tv.tv_usec < 0) {
        tv.tv_usec = 0;
    }
    do {
        rc = select(fd + 1, &rfds, NULL, NULL, &tv);
    } while (rc < 0 && errno == EINTR && !g_stop);
    if (rc < 0) {
        return -1;
    }
    if (rc == 0) {
        return 0;
    }
    for (;;) {
        ssize_t n = read(fd, buf, len);
        if (n < 0 && errno == EINTR && !g_stop) {
            continue;
        }
        return n;
    }
}

typedef struct {
    int rssi;
    char evtp[3];
    char adtp[3];
    char phys[3];
    char mac[13];
    char payload[2 * MAX_ADV_PAYLOAD + 1];
    bool has_payload;
} ReadResult;

typedef struct {
    int fd;
    double timeout;
    int baud;
    char port[256];

    uint8_t data[PARSER_BUF_CAP];
    size_t data_len;
    bool sync;

    double cmd_time;
    int config_cmd[256];

    int scan_enabled; /* -1 unknown, 0 false, 1 true */

    int last_vbat_status;
    int last_vbat_mv;
    int last_vbat_temp_c;
    bool last_vbat_valid;
    bool last_vbat_mv_valid;
    bool last_vbat_temp_valid;

    ReadResult last_result;
} Ble2Uart;

static const char *command_status_name(uint8_t status) {
    switch (status) {
        case 0: return "OK";
        case 1: return "ARGS";
        case 2: return "PIN";
        case 3: return "DENIED";
        case 4: return "VALUE";
        default: return NULL;
    }
}

static double config_start(Ble2Uart *dv) {
    double now = now_sec();
    double ret = now - dv->cmd_time;
    memset(dv->config_cmd, 0, sizeof(dv->config_cmd));
    dv->cmd_time = now;
    return ret;
}

static bool config_has_pending(const Ble2Uart *dv) {
    size_t i;
    for (i = 0; i < 256; ++i) {
        if (dv->config_cmd[i] > 0) {
            return true;
        }
    }
    return false;
}

static double config_still_running(const Ble2Uart *dv) {
    if (!config_has_pending(dv)) {
        return -1.0;
    }
    return now_sec() - dv->cmd_time;
}

static void config_account(Ble2Uart *dv, uint8_t cmd) {
    if (dv->config_cmd[cmd] > 0) {
        dv->config_cmd[cmd]--;
    }
}

static int ble2uart_reopen(Ble2Uart *dv, const char *port, int baud, double timeout) {
    memset(dv, 0, sizeof(*dv));
    dv->fd = -1;
    dv->scan_enabled = -1;
    dv->timeout = timeout;
    dv->baud = baud;
    snprintf(dv->port, sizeof(dv->port), "%s", port);
    config_start(dv);

    log_msg(LOG_WARNING, "Open %s, %d baud, timeout: %.3f secs...", dv->port, dv->baud, dv->timeout);
    dv->fd = serial_open_port(dv->port, dv->baud, dv->timeout);
    if (dv->fd < 0) {
        log_msg(LOG_ERROR, "OS error: %s. Terminating.", strerror(errno));
        return -1;
    }

    sleep_ms(50);
    serial_set_dtr_rts(dv->fd, true, true);
    sleep_ms(50);
    serial_set_dtr_rts(dv->fd, false, false);

    dv->sync = false;
    log_msg(LOG_WARNING, "Connected");
    return 0;
}

static ssize_t ble_read(Ble2Uart *dv, uint8_t *buf, size_t rdlen) {
    ssize_t n = serial_read_timeout(dv->fd, buf, rdlen, dv->timeout);
    if (n < 0) {
        if (errno == EINTR && g_stop) {
            return 0;
        }
        log_msg(LOG_ERROR, "%s read error: %s Terminating.", dv->port, strerror(errno));
    }
    return n;
}

static int ble_write_all(Ble2Uart *dv, const uint8_t *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(dv->fd, buf + off, len - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            log_msg(LOG_ERROR, "Write error %s: %s. Terminating.", dv->port, strerror(errno));
            return -1;
        }
        off += (size_t)n;
    }
    tcdrain(dv->fd);
    return 0;
}

static int ble_command(Ble2Uart *dv, const uint8_t *cmd, size_t cmd_len) {
    uint8_t frame[512];
    uint16_t crc;
    char bhex[1024];
    if (cmd_len + 2 > sizeof(frame)) {
        return -1;
    }
    memcpy(frame, cmd, cmd_len);
    crc = crc16_modbus(frame, cmd_len);
    frame[cmd_len] = (uint8_t)(crc & 0xFF);
    frame[cmd_len + 1] = (uint8_t)((crc >> 8) & 0xFF);
    if (ble_write_all(dv, frame, cmd_len + 2) != 0) {
        return -1;
    }
    if (g_log_level <= LOG_DEBUG) {
        bytes_to_hex_lower(frame, cmd_len + 2, bhex, sizeof(bhex));
        log_msg(LOG_DEBUG, "send cmd: %s", bhex);
    }
    dv->config_cmd[cmd[0]]++;
    sleep_ms(50);
    return 0;
}

static int add_mac_list(Ble2Uart *dv, const char *mac, uint8_t cmd_id) {
    uint8_t wire[6];
    size_t wire_len = 0;
    uint8_t payload[7];
    if (!mac_to_wire(mac, wire, &wire_len)) {
        return -1;
    }
    payload[0] = cmd_id;
    memcpy(payload + 1, wire, wire_len);
    return ble_command(dv, payload, 1 + wire_len);
}

static size_t drain_input(Ble2Uart *dv, double wait_seconds, size_t rdlen) {
    double deadline;
    size_t total = 0;
    uint8_t tmp[512];
    if (rdlen > sizeof(tmp)) {
        rdlen = sizeof(tmp);
    }
    if (wait_seconds <= 0.0) {
        wait_seconds = dv->timeout * 4.0;
        if (wait_seconds < 0.5) {
            wait_seconds = 0.5;
        }
    }
    deadline = now_sec() + wait_seconds;
    while (now_sec() < deadline) {
        ssize_t n = ble_read(dv, tmp, rdlen);
        if (n <= 0) {
            break;
        }
        total += (size_t)n;
    }
    return total;
}

static void set_last_result_empty(Ble2Uart *dv) {
    memset(&dv->last_result, 0, sizeof(dv->last_result));
}

static void handle_gpioevt_frame(Ble2Uart *dv, uint8_t index, const uint8_t *payload, uint8_t len_cmd) {
    (void)dv;
    if (index & GPIOEVT_EVENT_FLAG) {
        if (len_cmd < 6) {
            char hx[64];
            bytes_to_hex_lower(payload, len_cmd, hx, sizeof(hx));
            log_msg(LOG_WARNING, "GPIOEVT event (truncated): %s", hx);
            return;
        }
        {
            uint8_t pin = payload[0];
            uint8_t level = payload[1];
            uint32_t ts_ms = (uint32_t)payload[2] |
                             ((uint32_t)payload[3] << 8) |
                             ((uint32_t)payload[4] << 16) |
                             ((uint32_t)payload[5] << 24);
            const char *edge = level ? "RISE" : "FALL";
            log_msg(LOG_WARNING, "GPIOEVT event: GPIO%d -> %d (%s)  t=%u ms", pin, level, edge, ts_ms);
        }
        return;
    }

    {
        const char *st = command_status_name(index);
        char stbuf[16];
        if (!st) {
            snprintf(stbuf, sizeof(stbuf), "0x%02X", index);
            st = stbuf;
        }
        if (len_cmd >= 5) {
            uint8_t pin_acted = payload[0];
            uint32_t mask = (uint32_t)payload[1] |
                            ((uint32_t)payload[2] << 8) |
                            ((uint32_t)payload[3] << 16) |
                            ((uint32_t)payload[4] << 24);
            log_msg(LOG_WARNING, "GPIOEVT ack: status=%s pin=0x%02X armed_mask=0x%08X", st, pin_acted, mask);
        } else if (len_cmd >= 4) {
            uint32_t mask = (uint32_t)payload[0] |
                            ((uint32_t)payload[1] << 8) |
                            ((uint32_t)payload[2] << 16) |
                            ((uint32_t)payload[3] << 24);
            log_msg(LOG_WARNING, "GPIOEVT query: status=%s armed_mask=0x%08X", st, mask);
        } else {
            char hx[64];
            bytes_to_hex_lower(payload, len_cmd, hx, sizeof(hx));
            log_msg(LOG_WARNING, "GPIOEVT: status=%s data=%s", st, hx);
        }
    }
}

static void format_scan_flags(uint8_t flag, int window_raw, char *out, size_t out_sz) {
    int window_ms = (int)((double)window_raw * 0.625);
    snprintf(out, out_sz,
             "{address_type=PUBLIC, address_type_filter_random=%s, address_type_filter_private=%s, DUP_FILTER_ENABLE=%s, SCAN_TYPE_ACTIVE=%s, SCAN_PHY_CODED=%s, SCAN_PHY_1M=%s, window_ms=%d}",
             (flag & (1u << 5)) ? "True" : "False",
             (flag & (1u << 4)) ? "True" : "False",
             (flag & (1u << 3)) ? "True" : "False",
             (flag & (1u << 2)) ? "True" : "False",
             (flag & (1u << 1)) ? "True" : "False",
             (flag & (1u << 0)) ? "True" : "False",
             window_ms);
}

static void process_packet(Ble2Uart *dv, const uint8_t *pkt, size_t total_len) {
    uint8_t len_payload = pkt[0];
    int8_t rssi = (int8_t)pkt[1];
    uint8_t adtp = pkt[2];
    uint8_t evtp = pkt[3];
    uint8_t phys = pkt[4];
    char mac[13];

    (void)total_len;
    mac_wire_to_upper12(pkt + 5, mac);

    if (phys == 0xFF) {
        uint8_t cmd = pkt[1];
        uint8_t len_cmd = pkt[3];
        const uint8_t *data = pkt + 5;

        if (memcmp(pkt + 1, (uint8_t[]){0x05,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00}, 10) == 0) {
            char dbg[512];
            size_t n = len_payload;
            if (n >= sizeof(dbg)) {
                n = sizeof(dbg) - 1;
            }
            memcpy(dbg, pkt + 11, n);
            dbg[n] = '\0';
            log_msg(LOG_WARNING, "Debug message: %s", dbg);
            return;
        }

        if (cmd == CMD_ID_INFO) {
            config_account(dv, cmd);
            log_msg(LOG_WARNING, "resp: %d=CmdInfo, version: %02x; local MAC: %s", rssi, adtp, mac);
            return;
        }

        if (cmd == CMD_ID_CLRM) {
            config_account(dv, cmd);
            log_msg(LOG_WARNING, "resp: %d=ClearMacList, definable elements: %u", rssi, adtp);
            return;
        }

        if (cmd == CMD_ID_WMAC || cmd == CMD_ID_BMAC) {
            config_account(dv, cmd);
            log_msg(LOG_WARNING, "resp: %d=Add %s List, position %u, MAC: %s", rssi,
                    cmd == CMD_ID_WMAC ? "WHITE" : "BLACK", adtp, mac);
            return;
        }

        if (cmd == CMD_ID_SCAN) {
            config_account(dv, cmd);
            if (len_cmd >= 1 && pkt[5] == 0) {
                dv->scan_enabled = 0;
                log_msg(LOG_WARNING, "resp: %d=SCAN Disable %02x", rssi, adtp);
            } else {
                char rawhex[32];
                char parsed[256];
                int window_raw = 0;
                uint8_t flag = len_cmd >= 1 ? pkt[5] : 0;
                if (len_cmd >= 3) {
                    window_raw = (int16_t)((uint16_t)pkt[6] | ((uint16_t)pkt[7] << 8));
                }
                bytes_to_hex_lower(pkt + 5, len_cmd, rawhex, sizeof(rawhex));
                format_scan_flags(flag, window_raw, parsed, sizeof(parsed));
                dv->scan_enabled = 1;
                log_msg(LOG_WARNING, "resp: %d=SCAN Enable, MAC addresses in list: %u, %s=%s", rssi, pkt[2], rawhex, parsed);
            }
            return;
        }

        if (cmd == CMD_ID_VBAT) {
            const char *st;
            char stbuf[16];
            config_account(dv, cmd);
            dv->last_vbat_status = pkt[2];
            dv->last_vbat_valid = true;
            dv->last_vbat_mv_valid = false;
            dv->last_vbat_temp_valid = false;
            if (len_cmd >= 2) {
                dv->last_vbat_mv = (int)((uint16_t)pkt[5] | ((uint16_t)pkt[6] << 8));
                dv->last_vbat_mv_valid = true;
            }
            if (len_cmd >= 4) {
                int16_t tc = (int16_t)((uint16_t)pkt[7] | ((uint16_t)pkt[8] << 8));
                if (tc != (int16_t)0x8000) {
                    dv->last_vbat_temp_c = tc;
                    dv->last_vbat_temp_valid = true;
                }
            }
            st = command_status_name((uint8_t)dv->last_vbat_status);
            if (!st) {
                snprintf(stbuf, sizeof(stbuf), "0x%02X", (uint8_t)dv->last_vbat_status);
                st = stbuf;
            }
            if (dv->last_vbat_mv_valid && dv->last_vbat_temp_valid) {
                log_msg(LOG_WARNING, "resp: %d=VBAT, status: %s, voltage: %d mV, temperature: %d C",
                        rssi, st, dv->last_vbat_mv, dv->last_vbat_temp_c);
            } else if (dv->last_vbat_mv_valid && len_cmd >= 4) {
                log_msg(LOG_WARNING, "resp: %d=VBAT, status: %s, voltage: %d mV, temperature: unavailable",
                        rssi, st, dv->last_vbat_mv);
            } else if (dv->last_vbat_mv_valid) {
                log_msg(LOG_WARNING, "resp: %d=VBAT, status: %s, voltage: %d mV",
                        rssi, st, dv->last_vbat_mv);
            } else {
                log_msg(LOG_WARNING, "resp: %d=VBAT, status: %s", rssi, st);
            }
            return;
        }

        if (cmd == CMD_ID_GPIOEVT) {
            config_account(dv, cmd);
            handle_gpioevt_frame(dv, pkt[2], data, len_cmd);
            return;
        }

        {
            char hx[512];
            bytes_to_hex_lower(pkt, len_payload + 11, hx, sizeof(hx));
            log_msg(LOG_ERROR, "blk: %s", hx);
        }
        return;
    }

    {
        char payload[2 * MAX_ADV_PAYLOAD + 1];
        snprintf(dv->last_result.evtp, sizeof(dv->last_result.evtp), "%02x", evtp);
        snprintf(dv->last_result.adtp, sizeof(dv->last_result.adtp), "%02x", adtp);
        snprintf(dv->last_result.phys, sizeof(dv->last_result.phys), "%02x", phys);
        snprintf(dv->last_result.mac, sizeof(dv->last_result.mac), "%s", mac);
        dv->last_result.rssi = rssi;

        bytes_to_hex_lower(pkt + 11, len_payload, payload, sizeof(payload));
        snprintf(dv->last_result.payload, sizeof(dv->last_result.payload), "%s", payload);
        dv->last_result.has_payload = len_payload > 0;

        log_msg(LOG_INFO, "adv: %u %d %s %s %s %s %s",
                len_payload, rssi, dv->last_result.evtp, dv->last_result.adtp,
                dv->last_result.phys, dv->last_result.mac, dv->last_result.payload);
    }
}

static void ble_read_adv(Ble2Uart *dv) {
    uint8_t blk[256];
    ssize_t n = ble_read(dv, blk, 64);
    if (n < 0) {
        exit(1);
    }
    if (n == 0) {
        return;
    }

    if (dv->data_len + (size_t)n > sizeof(dv->data)) {
        dv->data_len = 0;
    }
    memcpy(dv->data + dv->data_len, blk, (size_t)n);
    dv->data_len += (size_t)n;

    while (dv->data_len >= HEAD_CRC_ADD_LEN) {
        uint8_t len_payload = dv->data[0];
        size_t total = (size_t)len_payload + HEAD_CRC_ADD_LEN;

        if (total > dv->data_len) {
            break;
        }

        if (crc16_modbus(dv->data, total) == 0) {
            process_packet(dv, dv->data, total);
            dv->sync = true;
            memmove(dv->data, dv->data + total, dv->data_len - total);
            dv->data_len -= total;
        } else {
            if (dv->sync) {
                log_msg(LOG_ERROR, "CRC error. Discard %02x", dv->data[0]);
            } else {
                log_msg(LOG_DEBUG, "Discard %02x", dv->data[0]);
            }
            memmove(dv->data, dv->data + 1, dv->data_len - 1);
            dv->data_len -= 1;
        }
    }
}

static bool build_scan_command(uint8_t *out, size_t *out_len,
                               bool scan_phy_1m,
                               bool scan_phy_coded,
                               bool filter_random,
                               bool filter_private,
                               float window_ms,
                               uint8_t cmd_id) {
    uint8_t flag = 0;
    int16_t raw;
    if (!scan_phy_1m && !scan_phy_coded) {
        /* allowed: disables scan */
    }
    if (filter_random) {
        flag |= (1u << 5);
    }
    if (filter_private) {
        flag |= (1u << 4);
    }
    if (scan_phy_coded) {
        flag |= (1u << 1);
    }
    if (scan_phy_1m) {
        flag |= (1u << 0);
    }

    raw = (int16_t)(window_ms / 0.625f);
    out[0] = cmd_id;
    out[1] = flag;
    out[2] = (uint8_t)(raw & 0xFF);
    out[3] = (uint8_t)((raw >> 8) & 0xFF);
    *out_len = 4;
    return true;
}

static int black_white_list(Ble2Uart *dv,
                            const StringList *white_list,
                            const StringList *black_list,
                            bool info,
                            bool clear,
                            bool start,
                            const uint8_t *start_cmd,
                            size_t start_cmd_len) {
    size_t i;
    if (info) {
        uint8_t c = CMD_ID_INFO;
        if (ble_command(dv, &c, 1) != 0) return -1;
    }
    if (clear) {
        uint8_t c = CMD_ID_CLRM;
        if (ble_command(dv, &c, 1) != 0) return -1;
    }
    for (i = 0; i < white_list->count; ++i) {
        if (add_mac_list(dv, white_list->items[i], CMD_ID_WMAC) != 0) return -1;
    }
    for (i = 0; i < black_list->count; ++i) {
        if (add_mac_list(dv, black_list->items[i], CMD_ID_BMAC) != 0) return -1;
    }
    if (start) {
        if (ble_command(dv, start_cmd, start_cmd_len) != 0) return -1;
    }
    return 0;
}

static bool stop_scan(Ble2Uart *dv, double wait_seconds) {
    uint8_t stop_cmd[] = {CMD_ID_SCAN, 0x00, 0x00, 0x00};
    double deadline;
    if (ble_command(dv, stop_cmd, sizeof(stop_cmd)) != 0) {
        return false;
    }
    deadline = now_sec() + wait_seconds;
    while (now_sec() < deadline) {
        ble_read_adv(dv);
        if (dv->scan_enabled == 0) {
            return true;
        }
    }
    log_msg(LOG_WARNING, "SCAN Disable confirmation not received");
    return false;
}

static bool read_vbat(Ble2Uart *dv, double wait_seconds, int *status, int *mv, int *temp_c, bool *temp_valid) {
    uint8_t c = CMD_ID_VBAT;
    double deadline;
    dv->last_vbat_valid = false;
    dv->last_vbat_mv_valid = false;
    dv->last_vbat_temp_valid = false;

    if (ble_command(dv, &c, 1) != 0) {
        return false;
    }

    deadline = now_sec() + wait_seconds;
    while (now_sec() < deadline) {
        ble_read_adv(dv);
        if (dv->last_vbat_valid) {
            *status = dv->last_vbat_status;
            *mv = dv->last_vbat_mv;
            *temp_c = dv->last_vbat_temp_c;
            *temp_valid = dv->last_vbat_temp_valid;
            return true;
        }
    }
    return false;
}

static int gpioevt_cmd(Ble2Uart *dv, uint8_t op, int with_pin, uint8_t pin) {
    uint8_t payload[3];
    size_t len = 0;
    payload[len++] = CMD_ID_GPIOEVT;
    payload[len++] = op;
    if (with_pin) {
        payload[len++] = pin;
    }
    return ble_command(dv, payload, len);
}

typedef struct {
    bool debug;
    bool verbose;
    bool info;
    int sleep_s;
    int baudrate;
    const char *serial_port;
    double timeout;
    int number;
    double duration;
    double idle_timeout;
    double status_interval;
    const char *phy;
    float scan_window_ms;
    bool filter_random;
    bool filter_private;
    StringList whitelist_raw;
    StringList blacklist_raw;
    double info_after;
    bool battery;
    IntList gpio_event_pin;
    bool gpio_event_only;
    const char *device;
    int led_gpio;        /* -1 = unset */
    int rgb_gpio;        /* -1 = unset */
    int led_active_low;  /* -1 = unset */
} Args;

static void args_init(Args *a) {
    memset(a, 0, sizeof(*a));
    a->sleep_s = 1;
    a->baudrate = 2000000;
    a->serial_port = "/dev/ttyUSB0";
    a->timeout = 0.3;
    a->number = 0;
    a->duration = 0.0;
    a->idle_timeout = 0.0;
    a->status_interval = 10.0;
    a->phy = "both";
    a->scan_window_ms = 30.0f;
    a->filter_random = false;
    a->filter_private = false;
    a->info_after = 0.0;
    a->device = "esp32-c3";
    a->led_gpio = -1;
    a->rgb_gpio = -1;
    a->led_active_low = -1;
    strlist_init(&a->whitelist_raw);
    strlist_init(&a->blacklist_raw);
    intlist_init(&a->gpio_event_pin);
}

static void args_free(Args *a) {
    strlist_free(&a->whitelist_raw);
    strlist_free(&a->blacklist_raw);
    intlist_free(&a->gpio_event_pin);
}

static void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [options]\n"
            "\n"
            "BLE ADV_BLE2UART scanner\n"
            "  -d, --debug\n"
            "  -v, --verbose\n"
            "  -i, --info\n"
            "  -s, --sleep SEC\n"
            "  -b, --baudrate BAUD\n"
            "  -p, --port PORT\n"
            "  -t, --timeout SEC\n"
            "  -n, --number N\n"
            "      --duration SEC\n"
            "      --idle-timeout SEC\n"
            "      --status-interval SEC\n"
            "      --phy 1m|coded|both\n"
            "      --scan-window-ms MS\n"
            "      --filter-random / --accept-random\n"
            "      --filter-private / --accept-private\n"
            "      --whitelist MAC[,MAC...]\n"
            "      --blacklist MAC[,MAC...]\n"
            "      --backlist MAC[,MAC...]\n"
            "      --info-after SEC\n"
            "      --battery\n"
            "      --gpio-event PIN\n"
            "      --gpio-event-only\n"
            "      --device ID          device profile (default: esp32-c3)\n"
            "      --led-gpio N         override regular LED GPIO\n"
            "      --rgb-gpio N         override RGB LED GPIO\n"
            "      --led-active-low 0|1 override LED active-low polarity\n",
            prog);
}

static bool parse_int_opt(const char *s, int *out) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!s || *s == '\0' || !end || *end != '\0') {
        return false;
    }
    *out = (int)v;
    return true;
}

static bool parse_double_opt(const char *s, double *out) {
    char *end = NULL;
    double v = strtod(s, &end);
    if (!s || *s == '\0' || !end || *end != '\0') {
        return false;
    }
    *out = v;
    return true;
}

static bool parse_float_opt(const char *s, float *out) {
    char *end = NULL;
    float v = strtof(s, &end);
    if (!s || *s == '\0' || !end || *end != '\0') {
        return false;
    }
    *out = v;
    return true;
}

static int parse_args(int argc, char **argv, Args *args) {
    int c;
    enum {
        OPT_DURATION = 1000,
        OPT_IDLE_TIMEOUT,
        OPT_STATUS_INTERVAL,
        OPT_PHY,
        OPT_SCAN_WINDOW_MS,
        OPT_FILTER_RANDOM,
        OPT_ACCEPT_RANDOM,
        OPT_FILTER_PRIVATE,
        OPT_ACCEPT_PRIVATE,
        OPT_WHITELIST,
        OPT_BLACKLIST,
        OPT_BACKLIST,
        OPT_INFO_AFTER,
        OPT_BATTERY,
        OPT_GPIO_EVENT,
        OPT_GPIO_EVENT_ONLY,
        OPT_DEVICE,
        OPT_LED_GPIO,
        OPT_RGB_GPIO,
        OPT_LED_ACTIVE_LOW
    };

    static const struct option long_opts[] = {
        {"debug", no_argument, NULL, 'd'},
        {"verbose", no_argument, NULL, 'v'},
        {"info", no_argument, NULL, 'i'},
        {"sleep", required_argument, NULL, 's'},
        {"baudrate", required_argument, NULL, 'b'},
        {"port", required_argument, NULL, 'p'},
        {"timeout", required_argument, NULL, 't'},
        {"number", required_argument, NULL, 'n'},
        {"duration", required_argument, NULL, OPT_DURATION},
        {"idle-timeout", required_argument, NULL, OPT_IDLE_TIMEOUT},
        {"status-interval", required_argument, NULL, OPT_STATUS_INTERVAL},
        {"phy", required_argument, NULL, OPT_PHY},
        {"scan-window-ms", required_argument, NULL, OPT_SCAN_WINDOW_MS},
        {"filter-random", no_argument, NULL, OPT_FILTER_RANDOM},
        {"accept-random", no_argument, NULL, OPT_ACCEPT_RANDOM},
        {"filter-private", no_argument, NULL, OPT_FILTER_PRIVATE},
        {"accept-private", no_argument, NULL, OPT_ACCEPT_PRIVATE},
        {"whitelist", required_argument, NULL, OPT_WHITELIST},
        {"blacklist", required_argument, NULL, OPT_BLACKLIST},
        {"backlist", required_argument, NULL, OPT_BACKLIST},
        {"info-after", required_argument, NULL, OPT_INFO_AFTER},
        {"battery", no_argument, NULL, OPT_BATTERY},
        {"gpio-event", required_argument, NULL, OPT_GPIO_EVENT},
        {"gpio-event-only", no_argument, NULL, OPT_GPIO_EVENT_ONLY},
        {"device", required_argument, NULL, OPT_DEVICE},
        {"led-gpio", required_argument, NULL, OPT_LED_GPIO},
        {"rgb-gpio", required_argument, NULL, OPT_RGB_GPIO},
        {"led-active-low", required_argument, NULL, OPT_LED_ACTIVE_LOW},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "dvis:b:p:t:n:h", long_opts, NULL)) != -1) {
        switch (c) {
            case 'd':
                args->debug = true;
                break;
            case 'v':
                args->verbose = true;
                break;
            case 'i':
                args->info = true;
                break;
            case 's':
                if (!parse_int_opt(optarg, &args->sleep_s)) {
                    return -1;
                }
                break;
            case 'b':
                if (!parse_int_opt(optarg, &args->baudrate)) {
                    return -1;
                }
                break;
            case 'p':
                args->serial_port = optarg;
                break;
            case 't':
                if (!parse_double_opt(optarg, &args->timeout)) {
                    return -1;
                }
                break;
            case 'n':
                if (!parse_int_opt(optarg, &args->number)) {
                    return -1;
                }
                break;
            case OPT_DURATION:
                if (!parse_double_opt(optarg, &args->duration)) {
                    return -1;
                }
                break;
            case OPT_IDLE_TIMEOUT:
                if (!parse_double_opt(optarg, &args->idle_timeout)) {
                    return -1;
                }
                break;
            case OPT_STATUS_INTERVAL:
                if (!parse_double_opt(optarg, &args->status_interval)) {
                    return -1;
                }
                break;
            case OPT_PHY:
                if (strcmp(optarg, "1m") && strcmp(optarg, "coded") && strcmp(optarg, "both")) {
                    return -1;
                }
                args->phy = optarg;
                break;
            case OPT_SCAN_WINDOW_MS:
                if (!parse_float_opt(optarg, &args->scan_window_ms)) {
                    return -1;
                }
                break;
            case OPT_FILTER_RANDOM:
                args->filter_random = true;
                break;
            case OPT_ACCEPT_RANDOM:
                args->filter_random = false;
                break;
            case OPT_FILTER_PRIVATE:
                args->filter_private = true;
                break;
            case OPT_ACCEPT_PRIVATE:
                args->filter_private = false;
                break;
            case OPT_WHITELIST:
                if (!strlist_push(&args->whitelist_raw, optarg)) {
                    return -1;
                }
                break;
            case OPT_BLACKLIST:
            case OPT_BACKLIST:
                if (!strlist_push(&args->blacklist_raw, optarg)) {
                    return -1;
                }
                break;
            case OPT_INFO_AFTER:
                if (!parse_double_opt(optarg, &args->info_after)) {
                    return -1;
                }
                break;
            case OPT_BATTERY:
                args->battery = true;
                break;
            case OPT_GPIO_EVENT: {
                int pin;
                if (!parse_int_opt(optarg, &pin)) {
                    return -1;
                }
                if (!intlist_push(&args->gpio_event_pin, pin)) {
                    return -1;
                }
                break;
            }
            case OPT_GPIO_EVENT_ONLY:
                args->gpio_event_only = true;
                break;
            case OPT_DEVICE:
                args->device = optarg;
                break;
            case OPT_LED_GPIO:
                if (!parse_int_opt(optarg, &args->led_gpio)) return -1;
                break;
            case OPT_RGB_GPIO:
                if (!parse_int_opt(optarg, &args->rgb_gpio)) return -1;
                break;
            case OPT_LED_ACTIVE_LOW: {
                int v;
                if (!parse_int_opt(optarg, &v) || (v != 0 && v != 1)) return -1;
                args->led_active_low = v;
                break;
            }
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    Args args;
    Ble2Uart dv;
    StringList white_list;
    StringList black_list;
    uint8_t start_scan_cmd[8];
    size_t start_scan_cmd_len = 0;
    bool scan_started = false;
    int exit_code = 0;
    size_t i;

    args_init(&args);
    strlist_init(&white_list);
    strlist_init(&black_list);

    if (parse_args(argc, argv, &args) != 0) {
        print_usage(argv[0]);
        args_free(&args);
        strlist_free(&white_list);
        strlist_free(&black_list);
        return 2;
    }

    if (!parse_mac_filters(&args.whitelist_raw, &white_list, "--whitelist")) {
        args_free(&args);
        strlist_free(&white_list);
        strlist_free(&black_list);
        return 2;
    }
    if (!parse_mac_filters(&args.blacklist_raw, &black_list, "--blacklist/--backlist")) {
        args_free(&args);
        strlist_free(&white_list);
        strlist_free(&black_list);
        return 2;
    }

    if (args.info) {
        g_log_level = LOG_INFO;
    }
    if (args.verbose) {
        g_log_level = LOG_VERBOSE;
    }
    if (args.debug) {
        g_log_level = LOG_DEBUG;
    }

    if (args.info || args.verbose || args.debug) {
        log_msg(LOG_WARNING, "Set loglevel %d", g_log_level);
    } else if (!args.battery) {
        g_log_level = LOG_INFO;
    }

    if (args.battery) {
        log_msg(LOG_WARNING, "VBAT/temperature query mode");
    } else {
        log_msg(LOG_WARNING, "Press Ctrl+C to exit");
    }
    log_msg(LOG_WARNING, "Connecting to %s...", args.serial_port);

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    if (ble2uart_reopen(&dv, args.serial_port, args.baudrate, args.timeout) != 0) {
        exit_code = 1;
        goto done;
    }

    set_last_result_empty(&dv);

    sleep_ms(args.sleep_s * 1000);

    {
        size_t drained = drain_input(&dv, 0.0, 256);
        if (drained) {
            log_msg(LOG_WARNING, "Discarded %zu startup bytes from serial input", drained);
        }
    }

    for (i = 0; i < args.gpio_event_pin.count; ++i) {
        int pin = args.gpio_event_pin.items[i];
        log_msg(LOG_WARNING, "Arming GPIOEVT on GPIO%d", pin);
        gpioevt_cmd(&dv, GPIOEVT_OP_ENABLE, 1, (uint8_t)(pin & 0xFF));
    }
    if (args.gpio_event_pin.count > 0) {
        int j;
        for (j = 0; j < 20; ++j) {
            ble_read_adv(&dv);
            sleep_ms(50);
        }
    }

    if (args.gpio_event_only) {
        if (args.gpio_event_pin.count == 0) {
            log_msg(LOG_ERROR, "--gpio-event-only requires at least one --gpio-event PIN");
            exit_code = 2;
            goto finalize;
        }
        {
            double started = now_sec();
            log_msg(LOG_WARNING, "GPIO-event-only mode (Ctrl+C to exit)");
            while (!g_stop) {
                if (args.duration > 0.0 && now_sec() - started >= args.duration) {
                    break;
                }
                ble_read_adv(&dv);
            }
        }
    } else if (args.battery) {
        int st = 0;
        int mv = 0;
        int tc = 0;
        bool tc_valid = false;
        double wait = args.timeout * 4.0;
        const char *stname;
        char stbuf[16];
        if (wait < 2.0) {
            wait = 2.0;
        }
        if (!read_vbat(&dv, wait, &st, &mv, &tc, &tc_valid)) {
            log_msg(LOG_ERROR, "VBAT query timed out");
            exit_code = 1;
            goto finalize;
        }
        stname = command_status_name((uint8_t)st);
        if (!stname) {
            snprintf(stbuf, sizeof(stbuf), "0x%02X", (uint8_t)st);
            stname = stbuf;
        }
        if (st != 0) {
            log_msg(LOG_ERROR, "VBAT query failed: %s", stname);
            exit_code = 1;
            goto finalize;
        }
        if (!tc_valid) {
            log_msg(LOG_WARNING, "VBAT = %d mV (device 3.3V / VBAT rail); chip temperature unavailable", mv);
        } else {
            log_msg(LOG_WARNING, "VBAT = %d mV (device 3.3V / VBAT rail); chip temperature = %d C", mv, tc);
        }
    } else {
        double started_at;
        double last_adv_at;
        double last_status_at;
        double info_sent_at = 0.0;
        bool info_timeout_reported = false;
        int count = 0;

        config_start(&dv);
        build_scan_command(start_scan_cmd, &start_scan_cmd_len,
                           strcmp(args.phy, "coded") != 0,
                           strcmp(args.phy, "1m") != 0,
                           args.filter_random,
                           args.filter_private,
                           args.scan_window_ms,
                           CMD_ID_SCAN);

        if (black_white_list(&dv, &white_list, &black_list, true, true, true,
                             start_scan_cmd, start_scan_cmd_len) != 0) {
            exit_code = 1;
            goto finalize;
        }
        scan_started = true;

        started_at = now_sec();
        last_adv_at = started_at;
        last_status_at = started_at;

        while (!g_stop) {
            double now = now_sec();

            if (args.duration > 0.0 && now - started_at >= args.duration) {
                log_msg(LOG_WARNING, "Duration reached: %.1f seconds", args.duration);
                break;
            }
            if (args.idle_timeout > 0.0 && now - last_adv_at >= args.idle_timeout) {
                log_msg(LOG_WARNING, "Idle timeout reached: %.1f seconds without advertisements", args.idle_timeout);
                break;
            }
            if (args.status_interval > 0.0 &&
                now - last_adv_at >= args.status_interval &&
                now - last_status_at >= args.status_interval) {
                log_msg(LOG_INFO, "idle: no advertisements for %.1f seconds; scanner still running", now - last_adv_at);
                last_status_at = now;
            }

            if (args.info_after > 0.0 && info_sent_at == 0.0 && now - started_at >= args.info_after) {
                uint8_t info_cmd = CMD_ID_INFO;
                log_msg(LOG_WARNING, "Sending INFO during active scan at %.1f seconds", now - started_at);
                ble_command(&dv, &info_cmd, 1);
                info_sent_at = now;
            }

            set_last_result_empty(&dv);
            ble_read_adv(&dv);

            if (info_sent_at > 0.0 && !info_timeout_reported) {
                if (dv.config_cmd[CMD_ID_INFO] == 0) {
                    log_msg(LOG_WARNING, "INFO during scan acknowledged");
                    info_timeout_reported = true;
                } else if (now - info_sent_at >= 2.0) {
                    log_msg(LOG_WARNING, "INFO during scan not acknowledged within 2.0 seconds");
                    info_timeout_reported = true;
                }
            }

            if (dv.last_result.has_payload) {
                last_adv_at = now_sec();
                if (args.number > 0) {
                    count++;
                    if (count == args.number) {
                        break;
                    }
                }
            }

            if (config_still_running(&dv) > 3.0) {
                int pending = 0;
                int cmd;
                char pending_buf[256];
                size_t pos = 0;
                pending_buf[0] = '\0';
                for (cmd = 0; cmd < 256; ++cmd) {
                    if (dv.config_cmd[cmd] > 0) {
                        int n = snprintf(pending_buf + pos, sizeof(pending_buf) - pos,
                                         "%s0x%02X:%d", pending ? ", " : "", cmd, dv.config_cmd[cmd]);
                        if (n > 0) {
                            pos += (size_t)n;
                            if (pos >= sizeof(pending_buf)) {
                                pos = sizeof(pending_buf) - 1;
                                break;
                            }
                        }
                        pending++;
                    }
                }
                log_msg(LOG_WARNING, "Commands not answered in time: {%s}. Retrying...", pending_buf);
                config_start(&dv);
                black_white_list(&dv, &white_list, &black_list, true, true, true,
                                 start_scan_cmd, start_scan_cmd_len);
            }
        }

        if (g_stop) {
            log_msg(LOG_WARNING, "Interrupted");
            exit_code = 130;
        }
    }

finalize:
    if (scan_started) {
        stop_scan(&dv, 2.0);
    }

    if (dv.fd >= 0) {
        close(dv.fd);
        dv.fd = -1;
    }

done:
    args_free(&args);
    strlist_free(&white_list);
    strlist_free(&black_list);

    return exit_code;
}
