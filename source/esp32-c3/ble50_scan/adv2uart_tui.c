#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

#define CMD_ID_INFO 0x00
#define CMD_ID_SCAN 0x01
#define CMD_ID_WMAC 0x02
#define CMD_ID_BMAC 0x03
#define CMD_ID_CLRM 0x04
#define CMD_ID_GPIO 0x06
#define CMD_ID_UART 0x08
#define CMD_ID_RFSDK 0x09
#define CMD_ID_VERSION 0x0A
#define CMD_ID_TXADV 0x0B
#define CMD_ID_CONN 0x0C
#define CMD_ID_TXDATA 0x0D
#define CMD_ID_RXDATA 0x0E
#define CMD_ID_VBAT 0x0F
#define CMD_ID_GPIOEVT 0x10

#define GPIO_OP_STATUS 0
#define GPIO_OP_READ 1
#define GPIO_OP_WRITE 2
#define GPIO_OP_TOGGLE 3
#define GPIO_OP_CONFIG 4
#define GPIO_OP_ANALOG_READ 7

#define GPIOEVT_OP_QUERY 0
#define GPIOEVT_OP_ENABLE 1
#define GPIOEVT_OP_DISABLE 2
#define GPIOEVT_OP_CLEAR 3

#define NAV_COUNT 7
static const char *nav_labels[NAV_COUNT] = {
    "PHY1M", "CODED", "ACTIVE", "DUP", "RAND", "PRIV", "WINDOW"
};

#define ACTION_COUNT 13
static const char *action_labels[ACTION_COUNT] = {
    "HELP", "QUIT", "SCAN", "GPIO", "BLE", "MAC", "EVENTS", "TXADV", "INFO", "CLEAR", "VBAT", "LED ON", "LED OFF"
};

#define BOARD_LED_GPIO 8

#define GPIO_PANEL_COUNT 13
static const uint8_t gpio_pins[GPIO_PANEL_COUNT]  = {0,1,2,3,4,5,6,7,8,9,10,20,21};
static const bool    gpio_analog[GPIO_PANEL_COUNT] = {1,1,1,1,1,0,0,0,0,0, 0, 0, 0};
static const char   *gpio_label[GPIO_PANEL_COUNT]  = {
    "GPIO0","GPIO1","GPIO2","GPIO3","GPIO4",
    "GPIO5","GPIO6","GPIO7","GPIO8(LED)","GPIO9(BOOT)",
    "GPIO10","GPIO20","GPIO21"
};

typedef struct {
    int    digital;   /* 0=LOW 1=HIGH -1=unknown */
    int    adc_raw;   /* 0..4095, -1=n/a or unknown */
    bool   valid;
    double ts;
} GpioPinState;

#define HEAD_CRC_ADD_LEN 13
#define MAX_ADV_PAYLOAD 229
#define MAX_FRAME (MAX_ADV_PAYLOAD + HEAD_CRC_ADD_LEN)

#define ADV_RING_CAP 4096
#define LOG_RING_CAP 220
#define STATS_CAP 256
#define PARSER_BUF_CAP 8192
#define MAC_LIST_CAP 64

static volatile sig_atomic_t g_stop = 0;

static void handle_sigint(int signum) {
    (void)signum;
    g_stop = 1;
}

typedef struct {
    char lines[ADV_RING_CAP][1024];
    int head;
    int count;
} TextRing;

typedef struct {
    char mac[13];
    uint32_t count;
    int8_t last_rssi;
    int8_t min_rssi;
    int8_t max_rssi;
    uint8_t last_phys;
    double last_seen;
} MacStat;

typedef struct {
    uint8_t buf[PARSER_BUF_CAP];
    size_t len;
    bool synced;
} FrameParser;

typedef struct {
    int fd;
    const char *port;
    int baud;

    bool scan_running;
    bool phy_1m;
    bool phy_coded;
    bool active_scan;
    bool duplicate_filter;
    bool filter_random;
    bool filter_private;
    float window_ms;
    bool scan_start_wait_ack;
    double scan_start_cmd_ts;

    bool usb_connected;
    bool usb_pulse_reset;

    bool show_ble_panel;
    uint8_t ble_conn_state;
    uint8_t ble_peer_addr_type;
    uint16_t ble_att_handle;
    uint16_t ble_interval_units;
    char ble_peer_mac[32];

    uint64_t total_count;
    uint64_t legacy_count;
    uint64_t one_m_count;
    uint64_t coded_count;
    double last_packet_ts;

    TextRing adv_ring;
    TextRing log_ring;
    MacStat stats[STATS_CAP];
    int stats_count;
    char local_mac[32];
    uint8_t fw_version;

    FrameParser parser;
    bool show_help;
    bool use_color;
    bool log_fullscreen;
    int  log_cursor;
    int  log_hscroll;

    int  adv_cursor;         /* selected advertisement index */
    int  adv_hscroll;        /* horizontal scroll inside adv text */
    bool adv_follow_latest;   /* auto-follow newest advertisements */
    bool adv_inspect_mode;    /* true when user froze advertisement scrolling */
    bool adv_detail_open;     /* popup showing selected advertisement details */
    double adv_pause_until;   /* auto-resume timestamp after ESC live-pause */
    uint64_t adv_overflow_count; /* number of overwritten adv records due to full ring */
    bool adv_panel_nav;       /* browse advertisements while scan is stopped */

    bool         show_gpio_panel;
    int          gpio_cursor;        /* selected row in GPIO panel */
    bool         gpio_action_open;   /* action popup visible */
    int          gpio_action_cursor;
    GpioPinState gpio_state[GPIO_PANEL_COUNT];
    bool         gpio_out_known[GPIO_PANEL_COUNT];
    bool         gpio_out_mode[GPIO_PANEL_COUNT];
    double       gpio_refresh_ts;
    float        gpio_refresh_interval;

    bool         show_events_panel;
    int          events_cursor;
    uint32_t     gpioevt_mask;
    TextRing     evt_ring;

    bool         show_txadv_panel;
    int          txadv_cursor;
    int          txadv_phy;          /* 0=legacy 1M, 1=extended 1M, 2=coded */
    float        txadv_interval_ms;
    char         txadv_payload_hex[128];
    bool         txadv_running;
    char         txadv_last_status[96];
    bool         crc_debug;

    int          main_cursor;        /* selected scan-cfg item (0..NAV_COUNT-1) */
    int          action_cursor;      /* selected pre-scan action item */
    int          nav_row;            /* 0=Cfg row, 1=Action row */

    bool         show_mac_panel;
    int          mac_active_list;    /* 0=whitelist, 1=blacklist */
    int          mac_cursor;
    char         whitelist[MAC_LIST_CAP][13];
    int          whitelist_count;
    char         blacklist[MAC_LIST_CAP][13];
    int          blacklist_count;
} App;

/* Forward declarations for helpers used before their definitions. */
static void app_log(App *app, const char *fmt, ...);
static int serial_open_port(const char *port, int baud);
static void serial_set_dtr_rts(int fd, bool dtr, bool rts);
static int parse_hex_pair(char hi, char lo);
static void draw_box_unicode(int y, int x, int h, int w);
static void draw_adv_line_soft_labels(int y, int x, int maxw, const char *line);
static void draw_payload_alternating_bytes(int y, int x, int maxw, const char *payload, int start_byte, int total_bytes);
static void draw_ble_panel(App *app);
static void draw_events_panel(App *app);
static void draw_txadv_panel(App *app);
static void ensure_gpio_output_for_toggle(App *app, uint8_t pin);
static void send_mac_cmd(App *app, uint8_t cmd, const char *input);
static void sync_mac_lists_to_firmware(App *app);

static bool usb_set_connected(App *app, bool on, const char *reason) {
    if (on) {
        if (app->fd >= 0) {
            app->usb_connected = true;
            return true;
        }
        app->fd = serial_open_port(app->port, app->baud);
        if (app->fd < 0) {
            app->usb_connected = false;
            app_log(app, "USB connect failed %s: %s", app->port, strerror(errno));
            return false;
        }
        if (app->usb_pulse_reset) {
            serial_set_dtr_rts(app->fd, true, true);
            usleep(50000);
        }
        serial_set_dtr_rts(app->fd, false, false);
        app->usb_connected = true;
        if (reason != NULL && reason[0] != '\0') {
            app_log(app, "USB connected to %s @ %d (%s)", app->port, app->baud, reason);
        } else {
            app_log(app, "USB connected to %s @ %d", app->port, app->baud);
        }
        return true;
    }

    if (app->fd >= 0) {
        close(app->fd);
        app->fd = -1;
    }
    app->usb_connected = false;
    app->scan_running = false;
    app->scan_start_wait_ack = false;
    app_log(app, "USB disconnected");
    return true;
}

static void ring_add(TextRing *ring, const char *line) {
    snprintf(ring->lines[ring->head], sizeof(ring->lines[ring->head]), "%s", line);
    ring->head = (ring->head + 1) % (int)(sizeof(ring->lines) / sizeof(ring->lines[0]));
    if (ring->count < (int)(sizeof(ring->lines) / sizeof(ring->lines[0]))) {
        ring->count++;
    }
}

static void adv_ring_add(App *app, const char *line) {
    int cap = (int)(sizeof(app->adv_ring.lines) / sizeof(app->adv_ring.lines[0]));
    bool was_full = (app->adv_ring.count >= cap);

    if (was_full) {
        app->adv_overflow_count++;
        /* Keep selection stable while frozen: indexes shift left by one on overwrite. */
        if (!app->adv_follow_latest && app->adv_cursor > 0) {
            app->adv_cursor--;
        }
    }

    ring_add(&app->adv_ring, line);
}

static void app_log(App *app, const char *fmt, ...) {
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    ring_add(&app->log_ring, msg);
}

static void clear_advertisements_panel(App *app) {
    memset(&app->adv_ring, 0, sizeof(app->adv_ring));
    memset(&app->stats, 0, sizeof(app->stats));
    app->stats_count = 0;
    app->total_count = 0;
    app->legacy_count = 0;
    app->one_m_count = 0;
    app->coded_count = 0;
    app->last_packet_ts = 0.0;

    app->adv_cursor = -1;
    app->adv_hscroll = 0;
    app->adv_follow_latest = true;
    app->adv_inspect_mode = false;
    app->adv_detail_open = false;
    app->adv_pause_until = 0.0;
    app->adv_panel_nav = false;
    app->adv_overflow_count = 0;

    app_log(app, "Advertisements panel cleared");
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void format_time(double ts, char *out, size_t out_sz) {
    time_t sec = (time_t)ts;
    struct tm tmv;
    localtime_r(&sec, &tmv);
    snprintf(out, out_sz, "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

static uint16_t crc16_modbus(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 1) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001);
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static const char *command_name(uint8_t cmd) {
    switch (cmd) {
        case CMD_ID_INFO: return "INFO";
        case CMD_ID_SCAN: return "SCAN";
        case CMD_ID_WMAC: return "WMAC";
        case CMD_ID_BMAC: return "BMAC";
        case CMD_ID_CLRM: return "CLRM";
        case CMD_ID_GPIO: return "GPIO";
        case CMD_ID_UART: return "UART";
        case CMD_ID_RFSDK: return "RFSDK";
        case CMD_ID_VERSION: return "VERSION";
        case CMD_ID_TXADV: return "TXADV";
        case CMD_ID_CONN: return "CONN";
        case CMD_ID_TXDATA: return "TXDATA";
        case CMD_ID_RXDATA: return "RXDATA";
        case CMD_ID_VBAT: return "VBAT";
        case CMD_ID_GPIOEVT: return "GPIOEVT";
        default: return "UNKNOWN";
    }
}

static const char *status_name(uint8_t status) {
    switch (status) {
        case 0: return "OK";
        case 1: return "Bad arguments";
        case 2: return "Unknown pin";
        case 3: return "Denied";
        case 4: return "Bad value";
        default: return "?";
    }
}

static const char *phy_label(uint8_t phys, char *scratch, size_t scratch_sz) {
    if (phys == 0) {
        return "Legacy/1M";
    }
    unsigned primary = phys & 0x0F;
    unsigned secondary = (phys >> 4) & 0x0F;
    const char *p = primary == 1 ? "1M" : primary == 2 ? "2M" : primary == 3 ? "Coded" : "?";
    if (secondary == 0) {
        return p;
    }
    const char *s = secondary == 1 ? "1M" : secondary == 2 ? "2M" : secondary == 3 ? "Coded" : "?";
    snprintf(scratch, scratch_sz, "P:%s S:%s", p, s);
    return scratch;
}

static bool is_coded(uint8_t phys) {
    uint8_t p = phys & 0x0F;
    uint8_t s = (phys >> 4) & 0x0F;
    return p == 3 || s == 3;
}

static bool is_one_m(uint8_t phys) {
    uint8_t p = phys & 0x0F;
    uint8_t s = (phys >> 4) & 0x0F;
    return phys == 0 || p == 1 || s == 1;
}

static void mac_wire_to_str(const uint8_t *wire, int wire_len, char *out, size_t out_sz) {
    char hex[13] = {0};
    int out_idx = 0;
    for (int i = wire_len - 1; i >= 0 && out_idx < 12; --i) {
        out_idx += snprintf(hex + out_idx, sizeof(hex) - (size_t)out_idx, "%02X", wire[i]);
    }
    if (wire_len == 6) {
        snprintf(out, out_sz, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
                 hex[0], hex[1], hex[2], hex[3], hex[4], hex[5],
                 hex[6], hex[7], hex[8], hex[9], hex[10], hex[11]);
    } else {
        snprintf(out, out_sz, "%s", hex);
    }
}

static void bytes_to_hex_str(const uint8_t *data, size_t data_len, char *out, size_t out_sz) {
    size_t out_idx = 0;
    if (out_sz == 0) {
        return;
    }
    out[0] = '\0';
    for (size_t i = 0; i < data_len && out_idx + 2 < out_sz; ++i) {
        int written = snprintf(out + out_idx, out_sz - out_idx, "%02X", data[i]);
        if (written <= 0) {
            break;
        }
        out_idx += (size_t)written;
    }
}

static int find_or_add_stat(App *app, const char *mac12) {
    for (int i = 0; i < app->stats_count; ++i) {
        if (strcmp(app->stats[i].mac, mac12) == 0) {
            return i;
        }
    }
    if (app->stats_count >= STATS_CAP) {
        return -1;
    }
    int idx = app->stats_count++;
    memset(&app->stats[idx], 0, sizeof(app->stats[idx]));
    snprintf(app->stats[idx].mac, sizeof(app->stats[idx].mac), "%s", mac12);
    app->stats[idx].min_rssi = 127;
    app->stats[idx].max_rssi = -127;
    return idx;
}

static speed_t baud_to_speed(int baud) {
    switch (baud) {
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
#ifdef B1500000
        case 1500000: return B1500000;
#endif
#ifdef B2000000
        case 2000000: return B2000000;
#endif
        default: return B115200;
    }
}

static int serial_open_port(const char *port, int baud) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        close(fd);
        return -1;
    }

    cfmakeraw(&tio);
    tio.c_cflag |= CLOCAL | CREAD;
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CRTSCTS;

    speed_t speed = baud_to_speed(baud);
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        close(fd);
        return -1;
    }

    tcflush(fd, TCIOFLUSH);
    return fd;
}

static void serial_set_dtr_rts(int fd, bool dtr, bool rts) {
    int status = 0;
    if (ioctl(fd, TIOCMGET, &status) != 0) {
        return;
    }
    if (dtr) {
        status |= TIOCM_DTR;
    } else {
        status &= ~TIOCM_DTR;
    }
    if (rts) {
        status |= TIOCM_RTS;
    } else {
        status &= ~TIOCM_RTS;
    }
    ioctl(fd, TIOCMSET, &status);
}

static bool serial_send_payload(App *app, const uint8_t *payload, size_t len) {
    if (app->fd < 0) {
        if (!usb_set_connected(app, true, "auto-on by command")) {
            return false;
        }
    }
    if (len + 2 > 512) {
        return false;
    }
    uint8_t frame[512];
    memcpy(frame, payload, len);
    uint16_t crc = crc16_modbus(payload, len);
    frame[len] = (uint8_t)(crc & 0xFF);
    frame[len + 1] = (uint8_t)((crc >> 8) & 0xFF);

    size_t total = len + 2;
    size_t off = 0;
    while (off < total) {
        ssize_t wr = write(app->fd, frame + off, total - off);
        if (wr > 0) {
            off += (size_t)wr;
            continue;
        }
        if (wr < 0 && errno == EINTR) {
            continue;
        }
        if (wr < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            usleep(1000);
            continue;
        }
        app_log(app, "TX error: %s", strerror(errno));
        return false;
    }
    tcdrain(app->fd);
    app_log(app, "TX %s", command_name(payload[0]));
    return true;
}

static uint16_t scan_units_from_ms(float ms) {
    if (ms < 0.0f) {
        ms = 0.0f;
    }
    int units = (int)(ms / 0.625f);
    if (units < 0) {
        units = 0;
    }
    if (units > 0x7FFF) {
        units = 0x7FFF;
    }
    return (uint16_t)units;
}

static void send_scan_start(App *app) {
    uint8_t payload[4];
    uint8_t flag = 0;
    if (app->filter_random) {
        flag |= (1U << 5);
    }
    if (app->filter_private) {
        flag |= (1U << 4);
    }
    if (app->duplicate_filter) {
        flag |= (1U << 3);
    }
    if (app->active_scan) {
        flag |= (1U << 2);
    }
    if (app->phy_coded) {
        flag |= (1U << 1);
    }
    if (app->phy_1m) {
        flag |= 1U;
    }
    uint16_t win = scan_units_from_ms(app->window_ms);

    payload[0] = CMD_ID_SCAN;
    payload[1] = flag;
    payload[2] = (uint8_t)(win & 0xFF);
    payload[3] = (uint8_t)((win >> 8) & 0xFF);
    sync_mac_lists_to_firmware(app);
    usleep(50000);
    app_log(app, "SCAN cfg tx: flag=0x%02X win=%.1fms (%u)", flag, (double)app->window_ms, (unsigned)win);
    if (serial_send_payload(app, payload, sizeof(payload))) {
        app->scan_start_wait_ack = true;
        app->scan_start_cmd_ts = now_sec();
    }
}

static void send_scan_stop(App *app) {
    const uint8_t payload[] = {CMD_ID_SCAN, 0, 0, 0};
    serial_send_payload(app, payload, sizeof(payload));
    app->scan_start_wait_ack = false;
}

static int normalize_mac_hex(const char *input, char *hex_out, size_t out_sz) {
    char tmp[13] = {0};
    size_t n = 0;
    for (size_t i = 0; input[i] != '\0'; ++i) {
        if (isxdigit((unsigned char)input[i])) {
            if (n >= sizeof(tmp) - 1) {
                return -1;
            }
            tmp[n++] = (char)toupper((unsigned char)input[i]);
        }
    }
    if (n == 0 || (n % 2) != 0 || n > 12) {
        return -1;
    }
    if (n + 1 > out_sz) {
        return -1;
    }
    memcpy(hex_out, tmp, n);
    hex_out[n] = '\0';
    return (int)n;
}

static int *mac_count_ptr(App *app, int list_id) {
    return list_id == 0 ? &app->whitelist_count : &app->blacklist_count;
}

static char (*mac_array_ptr(App *app, int list_id))[13] {
    return list_id == 0 ? app->whitelist : app->blacklist;
}

static const char *mac_list_name(int list_id) {
    return list_id == 0 ? "Whitelist" : "Blacklist";
}

static int mac_find_index(char arr[][13], int count, const char *hex12) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(arr[i], hex12) == 0) {
            return i;
        }
    }
    return -1;
}

static bool mac_list_add_local(App *app, int list_id, const char *input) {
    char hex[13];
    int nhex = normalize_mac_hex(input, hex, sizeof(hex));
    int *count = mac_count_ptr(app, list_id);
    char (*arr)[13] = mac_array_ptr(app, list_id);

    if (nhex < 0) {
        app_log(app, "Invalid MAC/prefix: use 1-6 bytes (2-12 hex)");
        return false;
    }
    if (mac_find_index(arr, *count, hex) >= 0) {
        app_log(app, "%s already contains %s", mac_list_name(list_id), hex);
        return false;
    }
    if (*count >= MAC_LIST_CAP) {
        app_log(app, "%s full (%d entries)", mac_list_name(list_id), MAC_LIST_CAP);
        return false;
    }
    snprintf(arr[*count], sizeof(arr[*count]), "%s", hex);
    (*count)++;
    app_log(app, "%s add %s", mac_list_name(list_id), hex);
    return true;
}

static bool mac_list_edit_local(App *app, int list_id, int idx, const char *input) {
    char hex[13];
    int nhex = normalize_mac_hex(input, hex, sizeof(hex));
    int *count = mac_count_ptr(app, list_id);
    char (*arr)[13] = mac_array_ptr(app, list_id);

    if (idx < 0 || idx >= *count) {
        return false;
    }
    if (nhex < 0) {
        app_log(app, "Invalid MAC/prefix: use 1-6 bytes (2-12 hex)");
        return false;
    }
    for (int i = 0; i < *count; ++i) {
        if (i != idx && strcmp(arr[i], hex) == 0) {
            app_log(app, "%s already contains %s", mac_list_name(list_id), hex);
            return false;
        }
    }
    snprintf(arr[idx], sizeof(arr[idx]), "%s", hex);
    app_log(app, "%s edit row %d -> %s", mac_list_name(list_id), idx + 1, hex);
    return true;
}

static bool mac_list_remove_local(App *app, int list_id, int idx) {
    int *count = mac_count_ptr(app, list_id);
    char (*arr)[13] = mac_array_ptr(app, list_id);
    char removed[13];

    if (idx < 0 || idx >= *count) {
        return false;
    }
    snprintf(removed, sizeof(removed), "%s", arr[idx]);
    for (int i = idx; i + 1 < *count; ++i) {
        memmove(arr[i], arr[i + 1], sizeof(arr[i]));
        arr[i][sizeof(arr[i]) - 1] = '\0';
    }
    (*count)--;
    if (*count < 0) {
        *count = 0;
    }
    app_log(app, "%s remove %s", mac_list_name(list_id), removed);
    return true;
}

static void mac_list_clear_local(App *app, int list_id) {
    int *count = mac_count_ptr(app, list_id);
    *count = 0;
    app_log(app, "%s cleared (local)", mac_list_name(list_id));
}

static void sync_mac_lists_to_firmware(App *app) {
    const uint8_t clrm[] = {CMD_ID_CLRM};
    serial_send_payload(app, clrm, sizeof(clrm));
    usleep(50000);
    for (int i = 0; i < app->whitelist_count; ++i) {
        send_mac_cmd(app, CMD_ID_WMAC, app->whitelist[i]);
        usleep(50000);
    }
    for (int i = 0; i < app->blacklist_count; ++i) {
        send_mac_cmd(app, CMD_ID_BMAC, app->blacklist[i]);
        usleep(50000);
    }
    app_log(app, "MAC lists synced: white=%d black=%d", app->whitelist_count, app->blacklist_count);
}

static void draw_mac_lists_panel(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);

    int panel_y = 4;
    int panel_h = h - panel_y - 1;
    if (panel_h < 6) {
        return;
    }

    for (int r = panel_y; r < panel_y + panel_h; ++r) {
        mvhline(r, 0, ' ', w);
    }

    draw_box_unicode(panel_y, 0, panel_h, w);
    attron(A_BOLD | A_REVERSE);
    mvprintw(panel_y, 2, " MAC Lists Editor ");
    attroff(A_BOLD | A_REVERSE);

    char meta[96];
    snprintf(meta, sizeof(meta), "W:%d B:%d  Active:%s",
             app->whitelist_count, app->blacklist_count,
             mac_list_name(app->mac_active_list));
    attron(A_BOLD);
    mvprintw(panel_y, w - 2 - (int)strlen(meta), "%s", meta);
    attroff(A_BOLD);

    int hdr = panel_y + 1;
    attron(A_BOLD | A_UNDERLINE);
    mvprintw(hdr, 2, "#   %-10s  %-12s", "List", "MAC/PREFIX");
    attroff(A_BOLD | A_UNDERLINE);

    int rows = panel_h - 4;
    char (*arr)[13] = mac_array_ptr(app, app->mac_active_list);
    int count = *mac_count_ptr(app, app->mac_active_list);

    if (count <= 0) {
        app->mac_cursor = 0;
    } else if (app->mac_cursor >= count) {
        app->mac_cursor = count - 1;
    } else if (app->mac_cursor < 0) {
        app->mac_cursor = 0;
    }

    int start = app->mac_cursor - rows + 1;
    if (start < 0) start = 0;
    if (start > count - rows) start = count - rows;
    if (start < 0) start = 0;

    if (count == 0) {
        attron(A_DIM);
        mvprintw(hdr + 1, 2, "(List is empty)  Press 'a' to add an entry.");
        attroff(A_DIM);
    }

    for (int i = 0; i < rows; ++i) {
        int idx = start + i;
        if (idx >= count) {
            break;
        }
        int row = hdr + 1 + i;
        bool sel = (idx == app->mac_cursor);
        const char *lname = app->mac_active_list == 0 ? "WHITE" : "BLACK";
        if (sel) {
            attron(A_REVERSE | A_BOLD);
        }
        mvprintw(row, 2, "%02d  %-10s  %-12s", idx + 1, lname, arr[idx]);
        if (sel) {
            attroff(A_REVERSE | A_BOLD);
        }
    }

    attron(A_DIM);
    mvprintw(panel_y + panel_h - 2, 2,
             "ESC/M close  Left/Right switch list  Up/Down browse  a add  e edit  Del/d remove  c clear active  C clear all  S sync FW");
    attroff(A_DIM);
}

static int parse_hex_pair(char hi, char lo) {
    int a = isdigit((unsigned char)hi) ? (hi - '0') : (toupper((unsigned char)hi) - 'A' + 10);
    int b = isdigit((unsigned char)lo) ? (lo - '0') : (toupper((unsigned char)lo) - 'A' + 10);
    return (a << 4) | b;
}

static bool parse_u8(const char *text, uint8_t *out) {
    if (text == NULL || text[0] == '\0') {
        return false;
    }
    char *end = NULL;
    long v = strtol(text, &end, 0);
    if (end == text || *end != '\0' || v < 0 || v > 255) {
        return false;
    }
    *out = (uint8_t)v;
    return true;
}

static bool parse_u8_triplet(const char *text, uint8_t *a, uint8_t *b, uint8_t *c) {
    if (text == NULL) {
        return false;
    }
    unsigned int ua = 0, ub = 0, uc = 0;
    if (sscanf(text, "%u %u %u", &ua, &ub, &uc) != 3) {
        return false;
    }
    if (ua > 255 || ub > 255 || uc > 255) {
        return false;
    }
    *a = (uint8_t)ua;
    *b = (uint8_t)ub;
    *c = (uint8_t)uc;
    return true;
}

static void send_gpio_cmd(App *app, uint8_t op, uint8_t pin, int extra_count, uint8_t e0, uint8_t e1, uint8_t e2) {
    uint8_t payload[6] = {CMD_ID_GPIO, op, pin, 0, 0, 0};
    size_t len = 3;
    if (extra_count > 0) {
        payload[3] = e0;
        len = 4;
    }
    if (extra_count > 1) {
        payload[4] = e1;
        len = 5;
    }
    if (extra_count > 2) {
        payload[5] = e2;
        len = 6;
    }
    serial_send_payload(app, payload, len);
}

static void send_gpioevt_cmd(App *app, uint8_t op, int with_pin, uint8_t pin) {
    if (with_pin) {
        uint8_t payload[] = {CMD_ID_GPIOEVT, op, pin};
        serial_send_payload(app, payload, sizeof(payload));
    } else {
        uint8_t payload[] = {CMD_ID_GPIOEVT, op};
        serial_send_payload(app, payload, sizeof(payload));
    }
}

static void evt_log(App *app, const char *fmt, ...) {
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    ring_add(&app->evt_ring, msg);
}

static int parse_hex_bytes(const char *text, uint8_t *out, int out_cap) {
    char hex[256];
    int n = 0;
    for (int i = 0; text[i] != '\0' && n < (int)sizeof(hex) - 1; ++i) {
        if (isxdigit((unsigned char)text[i])) {
            hex[n++] = (char)toupper((unsigned char)text[i]);
        }
    }
    if (n == 0 || (n % 2) != 0) {
        return -1;
    }
    int bytes = n / 2;
    if (bytes > out_cap) {
        return -1;
    }
    for (int i = 0; i < bytes; ++i) {
        out[i] = (uint8_t)parse_hex_pair(hex[i * 2], hex[i * 2 + 1]);
    }
    return bytes;
}

static void send_txadv_status(App *app) {
    const uint8_t payload[] = {CMD_ID_TXADV, 2};
    serial_send_payload(app, payload, sizeof(payload));
}

static void send_txadv_stop(App *app) {
    const uint8_t payload[] = {CMD_ID_TXADV, 0};
    serial_send_payload(app, payload, sizeof(payload));
}

static void send_txadv_start(App *app) {
    uint8_t adv[31];
    int adv_len = parse_hex_bytes(app->txadv_payload_hex, adv, (int)sizeof(adv));
    if (adv_len < 0) {
        app_log(app, "TXADV payload hex invalid (use even hex bytes, max 31 bytes)");
        return;
    }
    if (app->txadv_interval_ms < 20.0f) {
        app->txadv_interval_ms = 20.0f;
    }
    uint16_t units = (uint16_t)(app->txadv_interval_ms / 0.625f);
    if (units < 0x20) {
        units = 0x20;
    }
    uint8_t payload[64];
    size_t len = 0;
    payload[len++] = CMD_ID_TXADV;
    payload[len++] = 1;
    payload[len++] = (uint8_t)(app->txadv_phy & 0xFF);
    payload[len++] = (uint8_t)(units & 0xFF);
    payload[len++] = (uint8_t)((units >> 8) & 0xFF);
    payload[len++] = (uint8_t)adv_len;
    memcpy(payload + len, adv, (size_t)adv_len);
    len += (size_t)adv_len;
    serial_send_payload(app, payload, len);
}

static void draw_hline_acs(int y, int x, int width) {
    mvhline(y, x, ACS_HLINE, width);
}

static void draw_box_custom(int y, int x, int h, int w,
                            chtype tl, chtype tr, chtype bl, chtype br) {
    if (h < 2 || w < 2) {
        return;
    }
    mvaddch(y,         x,         tl);
    mvaddch(y,         x + w - 1, tr);
    mvaddch(y + h - 1, x,         bl);
    mvaddch(y + h - 1, x + w - 1, br);
    draw_hline_acs(y,         x + 1, w - 2);
    draw_hline_acs(y + h - 1, x + 1, w - 2);
    mvvline(y + 1, x,         ACS_VLINE, h - 2);
    mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);
}

static void draw_box_unicode(int y, int x, int h, int w);   /* forward */
static void draw_gpio_action_popup(App *app);               /* forward */

/* ---- GPIO panel support ------------------------------------------------- */

static int gpio_pin_index(uint8_t pin) {
    for (int i = 0; i < GPIO_PANEL_COUNT; ++i) {
        if (gpio_pins[i] == pin) { return i; }
    }
    return -1;
}

static void send_gpio_read_all(App *app) {
    for (int i = 0; i < GPIO_PANEL_COUNT; ++i) {
        send_gpio_cmd(app, GPIO_OP_READ, gpio_pins[i], 0, 0, 0, 0);
        if (gpio_analog[i]) {
            send_gpio_cmd(app, GPIO_OP_ANALOG_READ, gpio_pins[i], 0, 0, 0, 0);
        }
    }
}

static void ensure_gpio_output_for_toggle(App *app, uint8_t pin) {
    int idx = gpio_pin_index(pin);
    if (idx < 0) {
        return;
    }
    if (!app->gpio_out_known[idx] || !app->gpio_out_mode[idx]) {
        send_gpio_cmd(app, GPIO_OP_CONFIG, pin, 3, 0, 1, 0);
        app->gpio_out_known[idx] = true;
        app->gpio_out_mode[idx] = true;
        app_log(app, "GPIO%u auto-config to output before toggle", pin);
    }
}

static void draw_gpio_panel(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);

    int panel_y = 4;                    /* below fixed header rows 0-3 */
    int panel_h = h - panel_y - 1;     /* leave 1 row for status bar */
    if (panel_h < 5) { return; }

    draw_box_unicode(panel_y, 0, panel_h, w);
    attron(A_BOLD);
    mvprintw(panel_y, 2, " GPIO Panel — ESP32-C3 Super Mini ");
    attroff(A_BOLD);

    /* Column header */
    int hy = panel_y + 1;
    attron(A_BOLD | A_UNDERLINE);
    mvprintw(hy, 2, "%-14s %-6s  %-8s  %-5s  %s",
             "Pin", "Level", "ADC raw", "mV", "Updated");
    attroff(A_BOLD | A_UNDERLINE);

    int max_pin_rows = panel_h - 4;   /* borders + header + footer */

    for (int i = 0; i < GPIO_PANEL_COUNT && i < max_pin_rows; ++i) {
        GpioPinState *st = &app->gpio_state[i];
        int row = hy + 1 + i;
        bool selected = (i == app->gpio_cursor);

        char tbuf[16] = "-";
        if (st->valid && st->ts > 0.0) {
            format_time(st->ts, tbuf, sizeof(tbuf));
        }

        /* Selection indicator */
        if (selected) {
            attron(A_BOLD);
            mvaddch(row, 1, '>');
            attroff(A_BOLD);
        } else {
            mvaddch(row, 1, ' ');
        }

        /* Pin label — bold when selected */
        if (selected) attron(A_BOLD);
        mvprintw(row, 2, "%-14s", gpio_label[i]);
        if (selected) attroff(A_BOLD);

        /* Digital level */
        if (!st->valid) {
            attron(A_DIM);
            addstr(" ---   ");
            attroff(A_DIM);
        } else if (st->digital == 1) {
            attron(A_REVERSE | A_BOLD);
            addstr(" HIGH  ");
            attroff(A_REVERSE | A_BOLD);
        } else {
            attron(A_DIM);
            addstr(" low   ");
            attroff(A_DIM);
        }

        /* ADC raw + mV columns (only analog-capable pins) */
        if (gpio_analog[i] && st->valid && st->adc_raw >= 0) {
            int mv = st->adc_raw * 3300 / 4095;
            printw("  %-8d  %-5d", st->adc_raw, mv);
        } else if (gpio_analog[i]) {
            attron(A_DIM);
            printw("  %-8s  %-5s", "-", "-");
            attroff(A_DIM);
        } else {
            printw("  %-8s  %-5s", "n/a", "n/a");
        }

        /* Timestamp */
        printw("  %s", tbuf);
    }

    /* Footer inside bottom border */
    char last_ts[16] = "-";
    if (app->gpio_refresh_ts > 0.0) {
        format_time(app->gpio_refresh_ts, last_ts, sizeof(last_ts));
    }
    attron(A_DIM);
    mvprintw(panel_y + panel_h - 2, 2,
             "Auto-refresh: %.1fs  Last poll: %s  "
             "| Up/Down select  Enter action  G close  j refresh",
             (double)app->gpio_refresh_interval, last_ts);
    attroff(A_DIM);

    /* Draw action popup on top if open */
    if (app->gpio_action_open) {
        draw_gpio_action_popup(app);
    }
}

/* Action popup for selected GPIO pin */
static void draw_gpio_action_popup(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);

    int pin_idx  = app->gpio_cursor;
    bool is_an   = gpio_analog[pin_idx];
    int ph       = is_an ? 14 : 13;
    int pw       = 68;
    int py       = (h - ph) / 2;
    int px       = (w - pw) / 2;
    if (py < 4)  py = 4;
    if (px < 0)  px = 0;

    draw_box_unicode(py, px, ph, pw);

    attron(A_BOLD | A_REVERSE);
    mvprintw(py, px + 2, " %s — Action ", gpio_label[pin_idx]);
    attroff(A_BOLD | A_REVERSE);

    const char *items[8];
    int count = 0;
    items[count++] = "Read digital level";
    items[count++] = "Write level  (0 = LOW / 1 = HIGH)";
    items[count++] = "Toggle level";
    items[count++] = "Blink (pulse)";
    items[count++] = "Configure input/output + pull float/up/down";
    if (is_an) {
        items[count++] = "Analog read  (ADC, 0-4095 -> mV)";
    }
    items[count++] = "Query full status";

    if (app->gpio_action_cursor < 0) {
        app->gpio_action_cursor = 0;
    }
    if (app->gpio_action_cursor >= count) {
        app->gpio_action_cursor = count - 1;
    }

    int r = py + 1;
    for (int i = 0; i < count; ++i) {
        if (i == app->gpio_action_cursor) {
            attron(A_REVERSE | A_BOLD);
        }
        mvprintw(r++, px + 3, "[%d]  %s", i + 1, items[i]);
        if (i == app->gpio_action_cursor) {
            attroff(A_REVERSE | A_BOLD);
        }
    }
    r += 1;
    attron(A_DIM);
    mvprintw(r++, px + 3, "Up/Down select  Enter execute  ESC cancel");
    mvprintw(r, px + 3, "Quick keys: r w t c a s b");
    attroff(A_DIM);
}

static void draw_events_panel(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);

    int panel_y = 4;
    int panel_h = h - panel_y - 1;
    if (panel_h < 8) {
        return;
    }

    draw_box_unicode(panel_y, 0, panel_h, w);
    attron(A_BOLD);
    mvprintw(panel_y, 2, " GPIO Events ");
    attroff(A_BOLD);

    int split_x = w > 56 ? 34 : (w / 2);
    if (split_x < 24) {
        split_x = 24;
    }
    if (split_x > w - 16) {
        split_x = w - 16;
    }

    attron(A_BOLD | A_UNDERLINE);
    mvprintw(panel_y + 1, 2, "Pin  Armed");
    mvprintw(panel_y + 1, split_x + 2, "Events");
    attroff(A_BOLD | A_UNDERLINE);

    int rows = panel_h - 4;
    int list_rows = GPIO_PANEL_COUNT;
    if (list_rows > rows) {
        list_rows = rows;
    }
    for (int i = 0; i < list_rows; ++i) {
        uint8_t pin = gpio_pins[i];
        bool armed = (app->gpioevt_mask & (1u << pin)) != 0;
        int row = panel_y + 2 + i;
        if (i == app->events_cursor) {
            attron(A_REVERSE | A_BOLD);
        }
        mvprintw(row, 2, "GPIO%-2u %s", pin, armed ? "ON " : "OFF");
        if (i == app->events_cursor) {
            attroff(A_REVERSE | A_BOLD);
        }
    }

    int evt_top = panel_y + 2;
    int evt_h = panel_h - 4;
    int evt_start = app->evt_ring.count - evt_h;
    if (evt_start < 0) {
        evt_start = 0;
    }
    for (int i = 0; i < evt_h; ++i) {
        int idx = evt_start + i;
        if (idx >= app->evt_ring.count) {
            break;
        }
        int cap = (int)(sizeof(app->evt_ring.lines) / sizeof(app->evt_ring.lines[0]));
        int ring_idx = (app->evt_ring.head - app->evt_ring.count + idx + cap) % cap;
        mvprintw(evt_top + i, split_x + 2, "%.*s", w - split_x - 4, app->evt_ring.lines[ring_idx]);
    }

    attron(A_DIM);
    mvprintw(panel_y + panel_h - 2, 2,
             "Up/Down select pin  Enter toggle  e enable  d disable  g query mask  c clear all  ESC close");
    attroff(A_DIM);
}

static const char *txadv_phy_label(int phy) {
    switch (phy) {
        case 0: return "Legacy 1M";
        case 1: return "Ext 1M";
        case 2: return "Coded";
        default: return "Unknown";
    }
}

static void draw_txadv_panel(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);

    int panel_y = 4;
    int panel_h = h - panel_y - 1;
    if (panel_h < 10) {
        return;
    }

    draw_box_unicode(panel_y, 0, panel_h, w);
    attron(A_BOLD);
    mvprintw(panel_y, 2, " TX Adv ");
    attroff(A_BOLD);

    const char *items[6] = {
        "PHY",
        "Interval (ms)",
        "Payload (hex bytes)",
        "Start",
        "Stop",
        "Status"
    };

    int row = panel_y + 2;
    for (int i = 0; i < 6; ++i) {
        if (i == app->txadv_cursor) {
            attron(A_REVERSE | A_BOLD);
        }
        mvprintw(row + i, 2, "%-18s", items[i]);
        if (i == app->txadv_cursor) {
            attroff(A_REVERSE | A_BOLD);
        }
        if (i == 0) {
            mvprintw(row + i, 24, "%s", txadv_phy_label(app->txadv_phy));
        } else if (i == 1) {
            mvprintw(row + i, 24, "%.1f", (double)app->txadv_interval_ms);
        } else if (i == 2) {
            mvprintw(row + i, 24, "%.*s", w - 28, app->txadv_payload_hex);
        } else if (i == 3) {
            mvprintw(row + i, 24, "Send CMD start");
        } else if (i == 4) {
            mvprintw(row + i, 24, "Send CMD stop");
        } else if (i == 5) {
            mvprintw(row + i, 24, "Send CMD query");
        }
    }

    attron(A_BOLD);
    mvprintw(panel_y + 10, 2, "Running: %s", app->txadv_running ? "yes" : "no");
    attroff(A_BOLD);
    mvprintw(panel_y + 11, 2, "Last status: %.*s", w - 6, app->txadv_last_status[0] ? app->txadv_last_status : "-");

    attron(A_DIM);
    mvprintw(panel_y + panel_h - 2, 2,
             "Up/Down select  Left/Right PHY  Enter edit/execute  s start  x stop  i status  ESC close");
    attroff(A_DIM);
}

static void draw_box_unicode(int y, int x, int h, int w) {
    draw_box_custom(y, x, h, w, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
}

static const char *adv_line_by_index(const App *app, int idx) {
    if (idx < 0 || idx >= app->adv_ring.count) {
        return "";
    }
    int adv_cap = (int)(sizeof(app->adv_ring.lines) / sizeof(app->adv_ring.lines[0]));
    int ring_idx = (app->adv_ring.head - app->adv_ring.count + idx + adv_cap) % adv_cap;
    return app->adv_ring.lines[ring_idx];
}

static const char *log_line_by_index(const App *app, int idx) {
    if (idx < 0 || idx >= app->log_ring.count) {
        return "";
    }
    int log_cap = (int)(sizeof(app->log_ring.lines) / sizeof(app->log_ring.lines[0]));
    int ring_idx = (app->log_ring.head - app->log_ring.count + idx + log_cap) % log_cap;
    return app->log_ring.lines[ring_idx];
}

static void draw_log_fullscreen_overlay(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);

    int y = 1;
    int x = 0;
    int box_h = h - 1;
    int box_w = w;
    if (box_h < 4 || box_w < 10) {
        return;
    }

    /* Opaque overlay: clear the whole fullscreen log area first. */
    for (int r = y; r < y + box_h; ++r) {
        mvhline(r, x, ' ', box_w);
    }

    draw_box_unicode(y, x, box_h, box_w);
    attron(A_BOLD | A_REVERSE);
    mvprintw(y, x + 2, " Log Fullscreen ");
    attroff(A_BOLD | A_REVERSE);

    if (app->log_ring.count > 0) {
        if (app->log_cursor < 0) {
            app->log_cursor = app->log_ring.count - 1;
        } else if (app->log_cursor >= app->log_ring.count) {
            app->log_cursor = app->log_ring.count - 1;
        }
    } else {
        app->log_cursor = -1;
    }

    char meta[96];
    snprintf(meta, sizeof(meta), "[%d/%d x=%d] ESC close | Up/Down/PgUp/PgDn/Home/End browse",
             app->log_ring.count > 0 ? app->log_cursor + 1 : 0,
             app->log_ring.count,
             app->log_hscroll);
    int meta_x = box_w - 2 - (int)strlen(meta);
    if (meta_x < 20) {
        meta_x = 20;
    }
    attron(A_BOLD);
    mvprintw(y, meta_x, "%s", meta);
    attroff(A_BOLD);

    int rows = box_h - 2;
    int start = 0;
    if (app->log_cursor >= 0) {
        start = app->log_cursor - rows + 1;
        if (start < 0) {
            start = 0;
        }
        if (start > app->log_ring.count - rows) {
            start = app->log_ring.count - rows;
        }
        if (start < 0) {
            start = 0;
        }
    }

    if (app->log_ring.count == 0) {
        attron(A_DIM);
        mvprintw(y + 1, x + 2, "(No log lines yet)");
        attroff(A_DIM);
        return;
    }

    for (int i = 0; i < rows; ++i) {
        int idx = start + i;
        if (idx >= app->log_ring.count) {
            break;
        }
        mvhline(y + 1 + i, x + 1, ' ', box_w - 2);
        const char *line = log_line_by_index(app, idx);
        int line_len = (int)strlen(line);
        int start_col = app->log_hscroll;
        if (start_col < 0) {
            start_col = 0;
        }
        if (start_col > line_len) {
            start_col = line_len;
        }
        const char *view = line + start_col;
        if (idx == app->log_cursor) {
            attron(A_REVERSE | A_BOLD);
        }
        mvprintw(y + 1 + i, x + 1, "%.*s", w - 3, view);
        if (idx == app->log_cursor) {
            attroff(A_REVERSE | A_BOLD);
        }
    }
}

static void draw_adv_detail_overlay(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);
    if (app->adv_ring.count <= 0 || app->adv_cursor < 0 || app->adv_cursor >= app->adv_ring.count) {
        return;
    }

    int box_h = h - 6;
    int box_w = w - 8;
    if (box_h < 10) box_h = h - 2;
    if (box_w < 40) box_w = w - 2;
    int y = (h - box_h) / 2;
    int x = (w - box_w) / 2;

    draw_box_unicode(y, x, box_h, box_w);
    attron(A_BOLD | A_REVERSE);
    mvprintw(y, x + 2, " Advertisement %d/%d ", app->adv_cursor + 1, app->adv_ring.count);
    attroff(A_BOLD | A_REVERSE);

    /* Clear popup interior so only selected advertisement data is visible. */
    for (int r = y + 1; r < y + box_h - 1; ++r) {
        mvhline(r, x + 1, ' ', box_w - 2);
    }

    const char *line = adv_line_by_index(app, app->adv_cursor);
    const char *payload_tag = strstr(line, " payload=");
    const char *payload = payload_tag != NULL ? payload_tag + 9 : "";
    int meta_len = payload_tag != NULL ? (int)(payload_tag - line) : (int)strlen(line);
    int payload_len = (int)strlen(payload);
    int text_w = box_w - 4;
    int content_end = y + box_h - 3;
    int row = y + 1;
    int start_col = app->adv_hscroll;
    if (start_col < 0) start_col = 0;
    if (start_col > payload_len) start_col = payload_len;

    attron(A_BOLD);
    mvprintw(row++, x + 2, "Metadata:");
    attroff(A_BOLD);

    for (int off = 0; off < meta_len && row <= content_end; off += text_w) {
        int chunk = meta_len - off;
        if (chunk > text_w) {
            chunk = text_w;
        }
        char meta_line[1024];
        if (chunk >= (int)sizeof(meta_line)) {
            chunk = (int)sizeof(meta_line) - 1;
        }
        memcpy(meta_line, line + off, (size_t)chunk);
        meta_line[chunk] = '\0';
        draw_adv_line_soft_labels(row++, x + 2, text_w, meta_line);
    }

    if (row <= content_end) {
        row++;
    }

    if (row <= content_end) {
        attron(A_BOLD);
        mvprintw(row++, x + 2, "Payload:");
        attroff(A_BOLD);
    }

    if (payload_len == 0) {
        if (row <= content_end) {
            attron(A_DIM);
            mvprintw(row, x + 2, "(No advertisement payload bytes)");
            attroff(A_DIM);
        }
    } else {
        int payload_hex_len = payload_len;
        int payload_bytes = payload_hex_len / 2;
        int bytes_per_row = text_w / 2;
        int start_byte = start_col / 2;
        if (bytes_per_row < 1) {
            bytes_per_row = 1;
        }
        for (int boff = start_byte; boff < payload_bytes && row <= content_end; boff += bytes_per_row) {
            draw_payload_alternating_bytes(row++, x + 2, text_w, payload, boff, payload_bytes);
        }
        if ((payload_hex_len % 2) != 0 && row <= content_end) {
            attron(A_DIM);
            mvprintw(row, x + 2, "(warning: odd payload hex length)");
            attroff(A_DIM);
        }
    }

    attron(A_DIM);
    mvprintw(y + box_h - 2, x + 2, "ESC resume live | Left/Right pan | Up/Down browse records");
    attroff(A_DIM);
}

static void draw_payload_alternating_bytes(int y, int x, int maxw, const char *payload, int start_byte, int total_bytes) {
    int bytes_per_row = maxw / 2;
    int printed = 0;
    if (bytes_per_row < 1) {
        return;
    }

    for (int i = 0; i < bytes_per_row && (start_byte + i) < total_bytes; ++i) {
        int byte_idx = start_byte + i;
        int p = byte_idx * 2;
        char hi = payload[p];
        char lo = payload[p + 1];

        if ((byte_idx & 1) == 0) {
            attron(A_DIM);
        } else {
            attron(A_BOLD);
        }
        mvaddch(y, x + printed, hi);
        mvaddch(y, x + printed + 1, lo);
        if ((byte_idx & 1) == 0) {
            attroff(A_DIM);
        } else {
            attroff(A_BOLD);
        }
        printed += 2;
    }

    if (printed < maxw) {
        mvhline(y, x + printed, ' ', maxw - printed);
    }
}

static void draw_adv_line_soft_labels(int y, int x, int maxw, const char *line) {
    static const char *labels[] = {
        "RSSI=", "evt=", "addr=", "phy=", "mac=", "len=", "payload="
    };
    const int label_count = (int)(sizeof(labels) / sizeof(labels[0]));
    int printed = 0;
    const char *p = line;

    while (*p != '\0' && printed < maxw) {
        const char *next = NULL;
        const char *hit_label = NULL;

        for (int i = 0; i < label_count; ++i) {
            const char *q = strstr(p, labels[i]);
            if (q != NULL && (next == NULL || q < next)) {
                next = q;
                hit_label = labels[i];
            }
        }

        if (next == NULL) {
            int rem = maxw - printed;
            if (rem > 0) {
                mvprintw(y, x + printed, "%.*s", rem, p);
            }
            break;
        }

        if (next > p) {
            int chunk = (int)(next - p);
            int rem = maxw - printed;
            if (chunk > rem) {
                chunk = rem;
            }
            if (chunk > 0) {
                mvprintw(y, x + printed, "%.*s", chunk, p);
                printed += chunk;
            }
            if (printed >= maxw) {
                break;
            }
        }

        if (hit_label != NULL) {
            int l = (int)strlen(hit_label);
            int rem = maxw - printed;
            if (l > rem) {
                l = rem;
            }
            if (l > 0) {
                attron(A_DIM);
                mvprintw(y, x + printed, "%.*s", l, next);
                attroff(A_DIM);
                printed += l;
            }
            p = next + strlen(hit_label);
        } else {
            break;
        }
    }
}

static void draw_help_overlay(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);
    int box_h = h - 6;
    int box_w = w - 8;
    if (box_h < 16 || box_w < 56) {
        box_h = h - 2;
        box_w = w - 2;
    }
    int y = (h - box_h) / 2;
    int x = (w - box_w) / 2;

    attron(A_REVERSE);
    for (int r = 0; r < h; ++r) {
        mvhline(r, 0, ' ', w);
    }
    attroff(A_REVERSE);

    draw_box_unicode(y, x, box_h, box_w);

    int row = y + 1;
    attron(A_BOLD);
    mvprintw(row++, x + 2, "Command help — press h, ?, or ESC to close");
    attroff(A_BOLD);
    mvprintw(row++, x + 2, "Scan: s start/stop | 1 PHY 1M | 2 PHY coded | a active | d dup | r filt random | p filt private | m window");
    mvprintw(row++, x + 2, "Scan nav: ↑/↓ scroll ads | PgUp/PgDn page | ←/→ pan long rows | Enter details | ESC resume live");
    mvprintw(row++, x + 2, "Config nav (scan stopped): ←/→ select scan-cfg item | Enter toggle/set | PgUp/PgDn/Down(Actions) browse ads");
    mvprintw(row++, x + 2, "USB: U toggle connect on/off | default OFF unless CLI --connect | commands auto-connect when needed");
    mvprintw(row++, x + 2, "BLE panel: B quick toggle or Actions->[BLE] | s status | 1 connect 1M | 2 connect Coded | d disconnect | c cancel");
    mvprintw(row++, x + 2, "GPIO panel: ↑/↓ select pin | Enter open action | ESC/G close panel | j manual refresh");
    mvprintw(row++, x + 2, "Events panel: E quick toggle or Actions->[EVENTS] | Enter toggle pin | e enable | d disable | g query | c clear");
    mvprintw(row++, x + 2, "TXADV panel: T quick toggle or Actions->[TXADV] | edit PHY/interval/payload | s start | x stop | i status");
    mvprintw(row++, x + 2, "MAC panel: M open/close | ←/→ list | a add | e edit | d/del remove | c/C clear | S sync fw");
    mvprintw(row++, x + 2, "Info/list: i INFO | c clear MAC lists | w add whitelist | b add blacklist | v VBAT | X clear adv panel");
    mvprintw(row++, x + 2, "GPIO cmds: x read | y write | t toggle | f config [in out pull] | z analog read");
    mvprintw(row++, x + 2, "LED GPIO%d: n LED ON | o LED OFF | l LED toggle (active-low board LED)", BOARD_LED_GPIO);
    mvprintw(row++, x + 2, "Logs: L open/close fullscreen log | ESC close fullscreen");
    mvprintw(row++, x + 2, "GPIOEVT:   g query mask | e enable pin | u disable pin | k clear all");
    mvprintw(row++, x + 2, "Numeric input: decimal (e.g. 9) or hex 0x (e.g. 0x09), ESC cancels prompts");
    mvprintw(row++, x + 2, "GPIO ops: 0 status, 1 read, 2 write, 3 toggle, 4 config, 7 analog");

    row++;
    mvprintw(row++, x + 2, "GPIO response fields:");
    mvprintw(row++, x + 4, "op=0/1: level in data[2]");
    mvprintw(row++, x + 4, "op=7:   ADC raw in data[2..3]");
    mvprintw(row++, x + 4, "op=2/3/4: echo pin state and LED flag");

    attron(A_DIM);
    mvprintw(y + box_h - 2, x + 2, "q quit | h/? help | ESC closes overlays");
    attroff(A_DIM);

    (void)app;
}

static void send_mac_cmd(App *app, uint8_t cmd, const char *input) {
    char hex[13];
    int nhex = normalize_mac_hex(input, hex, sizeof(hex));
    if (nhex < 0) {
        app_log(app, "Invalid MAC: use 1-6 bytes (2-12 hex)");
        return;
    }

    int nbytes = nhex / 2;
    uint8_t payload[8] = {0};
    payload[0] = cmd;
    for (int i = 0; i < nbytes; ++i) {
        int src = (nbytes - 1 - i) * 2;
        payload[1 + i] = (uint8_t)parse_hex_pair(hex[src], hex[src + 1]);
    }
    serial_send_payload(app, payload, (size_t)(1 + nbytes));
}

static void handle_command_response(App *app, const uint8_t *packet, size_t total_len) {
    (void)total_len;
    uint8_t cmd = packet[1];
    uint8_t index = packet[2];
    uint8_t data_len = packet[3];
    const uint8_t *data = packet + 5;

    if (cmd == CMD_ID_INFO) {
        char macstr[32];
        if (data_len >= 6) {
            mac_wire_to_str(data, 6, macstr, sizeof(macstr));
            snprintf(app->local_mac, sizeof(app->local_mac), "%s", macstr);
        }
        app->fw_version = index;
        app_log(app, "INFO fw=0x%02X mac=%s", app->fw_version, app->local_mac[0] ? app->local_mac : "-");
        return;
    }

    if (cmd == CMD_ID_SCAN) {
        app->scan_start_wait_ack = false;
        if (data_len >= 1 && data[0] != 0) {
            uint8_t flag = data[0];
            int16_t win_units = 0;
            if (data_len >= 3) {
                win_units = (int16_t)((uint16_t)data[1] | ((uint16_t)data[2] << 8));
            }
            app->phy_1m = (flag & (1U << 0)) != 0;
            app->phy_coded = (flag & (1U << 1)) != 0;
            app->active_scan = (flag & (1U << 2)) != 0;
            app->duplicate_filter = (flag & (1U << 3)) != 0;
            app->filter_private = (flag & (1U << 4)) != 0;
            app->filter_random = (flag & (1U << 5)) != 0;
            if (data_len >= 3) {
                app->window_ms = (float)win_units * 0.625f;
            }
            app->scan_running = true;
            app_log(app,
                    "SCAN started cfg: 1M=%s coded=%s active=%s dup=%s rand=%s priv=%s window=%.1fms",
                    app->phy_1m ? "on" : "off",
                    app->phy_coded ? "on" : "off",
                    app->active_scan ? "on" : "off",
                    app->duplicate_filter ? "on" : "off",
                    app->filter_random ? "on" : "off",
                    app->filter_private ? "on" : "off",
                    (double)app->window_ms);
        } else {
            app->scan_running = false;
            app_log(app, "SCAN stopped");
        }
        return;
    }

    if (cmd == CMD_ID_VBAT) {
        if (index != 0) {
            app_log(app, "VBAT status=%s", status_name(index));
            return;
        }
        if (data_len >= 2) {
            uint16_t mv = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
            if (data_len >= 4) {
                int16_t tc = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
                app_log(app, "VBAT %u mV, temp %d C", mv, tc);
            } else {
                app_log(app, "VBAT %u mV", mv);
            }
        } else {
            app_log(app, "VBAT short response");
        }
        return;
    }

    if (cmd == CMD_ID_GPIOEVT) {
        if (index & 0x80) {
            if (data_len >= 6) {
                uint8_t pin = data[0];
                uint8_t level = data[1];
                uint32_t ts = (uint32_t)data[2] | ((uint32_t)data[3] << 8) |
                              ((uint32_t)data[4] << 16) | ((uint32_t)data[5] << 24);
                app_log(app, "GPIOEVT GPIO%u level=%u t=%u ms", pin, level, ts);
                evt_log(app, "GPIO%u level=%u t=%u ms", pin, level, ts);
            } else {
                app_log(app, "GPIOEVT short event");
                evt_log(app, "GPIOEVT short event");
            }
        } else {
            uint32_t mask = 0;
            if (data_len >= 4) {
                mask = (uint32_t)data[0] |
                       ((uint32_t)data[1] << 8) |
                       ((uint32_t)data[2] << 16) |
                       ((uint32_t)data[3] << 24);
            }
            app->gpioevt_mask = mask;
            app_log(app, "GPIOEVT status=%s mask=0x%08X", status_name(index), (unsigned int)mask);
            evt_log(app, "status=%s mask=0x%08X", status_name(index), (unsigned int)mask);
        }
        return;
    }

    if (cmd == CMD_ID_TXADV) {
        char hexbuf[192];
        bytes_to_hex_str(data, data_len, hexbuf, sizeof(hexbuf));
        snprintf(app->txadv_last_status, sizeof(app->txadv_last_status),
                 "status=%s payload=%.56s", status_name(index), hexbuf);
        if (data_len >= 1) {
            app->txadv_running = data[0] ? true : false;
        }
        app_log(app, "TXADV %s", app->txadv_last_status);
        return;
    }

    if (cmd == CMD_ID_GPIO) {
        if (data_len < 2) {
            app_log(app, "GPIO status=%s (short payload)", status_name(index));
            return;
        }
        uint8_t op  = data[0];
        uint8_t pin = data[1];
        if (index != 0) {
            app_log(app, "GPIO op=%u pin=%u status=%s", op, pin, status_name(index));
            return;
        }
        int idx = gpio_pin_index(pin);
        if (op == GPIO_OP_ANALOG_READ) {
            if (data_len >= 4) {
                uint16_t adc = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
                if (idx >= 0) {
                    app->gpio_state[idx].adc_raw = (int)adc;
                    app->gpio_state[idx].valid   = true;
                    app->gpio_state[idx].ts      = now_sec();
                }
                if (!app->show_gpio_panel) {
                    app_log(app, "GPIO analog pin=%u adc=%u (%d mV)",
                            pin, adc, (int)((int)adc * 3300 / 4095));
                }
            }
            return;
        }
        /* op=0 status / op=1 read / op=2 write / op=3 toggle */
        bool has_level = data_len >= 3;
        uint8_t level  = has_level ? data[2] : 0;
        uint8_t is_led = data_len >= 4 ? data[3] : 0;
        if (idx >= 0 && has_level) {
            app->gpio_state[idx].digital = level ? 1 : 0;
            app->gpio_state[idx].valid   = true;
            app->gpio_state[idx].ts      = now_sec();
            if (op == GPIO_OP_CONFIG && data_len >= 6) {
                app->gpio_out_known[idx] = true;
                app->gpio_out_mode[idx] = data[4] ? true : false;
            }
        }
        if (!app->show_gpio_panel) {
            if (has_level) {
                app_log(app, "GPIO op=%u pin=%u level=%u led=%u", op, pin, level, is_led);
            } else {
                app_log(app, "GPIO op=%u pin=%u short response", op, pin);
            }
        }
        return;
    }

    if (cmd == CMD_ID_CONN) {
        if (data_len >= 1) {
            app->ble_conn_state = data[0];
        }
        if (data_len >= 2) {
            app->ble_peer_addr_type = data[1];
        }
        if (data_len >= 4) {
            app->ble_att_handle = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
        }
        if (data_len >= 6) {
            app->ble_interval_units = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
        }
        app_log(app,
                "CONN status=%s state=%u addrType=%u handle=0x%04X interval=%.2fms",
                status_name(index),
                (unsigned)app->ble_conn_state,
                (unsigned)app->ble_peer_addr_type,
                (unsigned)app->ble_att_handle,
                (double)app->ble_interval_units * 1.25);
        return;
    }

    app_log(app, "%s status=%s len=%u", command_name(cmd), status_name(index), data_len);
}

static void handle_adv_packet(App *app, const uint8_t *packet) {
    int8_t rssi = (int8_t)packet[1];
    uint8_t event_type = packet[2];
    uint8_t addr_type = packet[3];
    uint8_t phys = packet[4];
    uint8_t payload_len = packet[0];
    const uint8_t *mac_wire = packet + 5;

    char mac_no_sep[13] = {0};
    char mac_ui[24] = {0};
    char phy_buf[24];
    char tbuf[16];

    mac_wire_to_str(mac_wire, 6, mac_ui, sizeof(mac_ui));
    for (int i = 0, j = 0; mac_ui[i] != '\0' && j < 12; ++i) {
        if (mac_ui[i] != ':') {
            mac_no_sep[j++] = mac_ui[i];
        }
    }

    int stat_idx = find_or_add_stat(app, mac_no_sep);
    if (stat_idx >= 0) {
        MacStat *st = &app->stats[stat_idx];
        st->count++;
        st->last_rssi = rssi;
        if (rssi < st->min_rssi) st->min_rssi = rssi;
        if (rssi > st->max_rssi) st->max_rssi = rssi;
        st->last_phys = phys;
        st->last_seen = now_sec();
    }

    app->total_count++;
    app->last_packet_ts = now_sec();
    if (phys == 0 || is_one_m(phys)) {
        app->one_m_count++;
        app->legacy_count++;
    }
    if (is_coded(phys)) {
        app->coded_count++;
    }

    format_time(app->last_packet_ts, tbuf, sizeof(tbuf));
    const char *phy = phy_label(phys, phy_buf, sizeof(phy_buf));

    char payload_hex[512] = {0};
    if (payload_len > 0) {
        bytes_to_hex_str(packet + 11, payload_len, payload_hex, sizeof(payload_hex));
    }

    char line[1024];
    if (payload_len > 0) {
        snprintf(line, sizeof(line), "%s RSSI=%4d evt=0x%02X addr=0x%02X phy=%-15s mac=%s len=%3u payload=%s",
                 tbuf, rssi, event_type, addr_type, phy, mac_ui, payload_len, payload_hex);
    } else {
        snprintf(line, sizeof(line), "%s RSSI=%4d evt=0x%02X addr=0x%02X phy=%-15s mac=%s len=%3u",
                 tbuf, rssi, event_type, addr_type, phy, mac_ui, payload_len);
    }
    adv_ring_add(app, line);
}

static void parser_consume(App *app, const uint8_t *data, size_t data_len) {
    if (data_len == 0) {
        return;
    }
    if (app->parser.len + data_len > sizeof(app->parser.buf)) {
        app->parser.len = 0;
    }
    memcpy(app->parser.buf + app->parser.len, data, data_len);
    app->parser.len += data_len;

    while (app->parser.len >= HEAD_CRC_ADD_LEN) {
        uint8_t payload_len = app->parser.buf[0];
        if (payload_len > MAX_ADV_PAYLOAD) {
            if (app->parser.synced) {
                if (app->crc_debug) {
                    app_log(app, "[CRC/DBG] discard: 0x%02X", app->parser.buf[0]);
                }
            }
            memmove(app->parser.buf, app->parser.buf + 1, app->parser.len - 1);
            app->parser.len -= 1;
            continue;
        }

        size_t total_len = (size_t)payload_len + HEAD_CRC_ADD_LEN;
        if (app->parser.len < total_len) {
            break;
        }

        if (crc16_modbus(app->parser.buf, total_len) == 0) {
            const uint8_t *packet = app->parser.buf;
            if (packet[4] == 0xFF) {
                handle_command_response(app, packet, total_len);
            } else {
                handle_adv_packet(app, packet);
            }
            app->parser.synced = true;
            memmove(app->parser.buf, app->parser.buf + total_len, app->parser.len - total_len);
            app->parser.len -= total_len;
        } else {
            if (app->parser.synced) {
                app_log(app, "[CRC/WARN] error, discard 0x%02X", app->parser.buf[0]);
            }
            memmove(app->parser.buf, app->parser.buf + 1, app->parser.len - 1);
            app->parser.len -= 1;
        }
    }
}

static void poll_serial(App *app) {
    if (app->fd < 0) {
        return;
    }
    uint8_t buf[512];
    size_t drained = 0;
    const size_t read_budget = sizeof(buf) * 16; /* keep UI and keyboard responsive */
    while (1) {
        ssize_t n = read(app->fd, buf, sizeof(buf));
        if (n > 0) {
            parser_consume(app, buf, (size_t)n);
            drained += (size_t)n;
            if (drained >= read_budget) {
                break;
            }
        } else if (n == 0) {
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            app_log(app, "Read error: %s", strerror(errno));
            break;
        }
    }
}

static const char *on_off(bool v) {
    return v ? "on" : "off";
}

static int prompt_input(const char *prompt, char *out, int out_sz) {
    int h, w;
    getmaxyx(stdscr, h, w);
    (void)w;
    timeout(-1);
    noecho();
    curs_set(1);
    out[0] = '\0';
    int len = 0;

    while (1) {
        mvprintw(h - 1, 0, "%s%s", prompt, out);
        clrtoeol();
        int ch = getch();
        if (ch == 27) {
            out[0] = '\0';
            curs_set(0);
            timeout(30);
            return -1;
        }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            break;
        }
        if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) && len > 0) {
            out[--len] = '\0';
            continue;
        }
        if (isprint(ch) && len < out_sz - 1) {
            out[len++] = (char)ch;
            out[len] = '\0';
        }
    }

    curs_set(0);
    timeout(30);
    return 0;
}

static bool handle_command_hotkeys(App *app, int ch) {
    if (ch == 'U') {
        if (app->usb_connected) {
            (void)usb_set_connected(app, false, "manual toggle");
        } else {
            (void)usb_set_connected(app, true, "manual toggle");
        }
        return true;
    }

    if (ch == 's' || ch == 'S') {
        if (app->scan_running) {
            send_scan_stop(app);
            app->adv_detail_open = false;
        } else {
            if (app->scan_start_wait_ack) {
                double age = now_sec() - app->scan_start_cmd_ts;
                if (age < 1.0) {
                    app_log(app, "SCAN start pending: waiting device ACK");
                    return true;
                }
                app->scan_start_wait_ack = false;
            }
            send_scan_start(app);
            app->adv_inspect_mode = false;
            app->adv_hscroll = 0;
            app->adv_follow_latest = true;
            app->adv_pause_until = 0.0;
        }
    } else if (ch == 'i' || ch == 'I') {
        const uint8_t payload[] = {CMD_ID_INFO};
        serial_send_payload(app, payload, sizeof(payload));
    } else if (ch == 'c' || ch == 'C') {
        const uint8_t payload[] = {CMD_ID_CLRM};
        serial_send_payload(app, payload, sizeof(payload));
    } else if (ch == 'v' || ch == 'V') {
        const uint8_t payload[] = {CMD_ID_VBAT};
        serial_send_payload(app, payload, sizeof(payload));
    } else if (ch == 'g') {
        send_gpioevt_cmd(app, GPIOEVT_OP_QUERY, 0, 0);
    } else if (ch == 'G') {
        app->show_gpio_panel = !app->show_gpio_panel;
        if (app->show_gpio_panel) {
            app->show_mac_panel = false;
            app->show_ble_panel = false;
            app->show_events_panel = false;
            app->show_txadv_panel = false;
            app->gpio_action_open = false;
            send_gpio_read_all(app);
            app->gpio_refresh_ts = now_sec();
        }
    } else if (ch == 'M') {
        app->show_mac_panel = !app->show_mac_panel;
        if (app->show_mac_panel) {
            app->show_gpio_panel = false;
            app->show_ble_panel = false;
            app->show_events_panel = false;
            app->show_txadv_panel = false;
            app->gpio_action_open = false;
        }
    } else if (ch == 'j' || ch == 'J') {
        send_gpio_read_all(app);
        app->gpio_refresh_ts = now_sec();
        app_log(app, "GPIO read-all sent");
    } else if (ch == 'e') {
        char value[32] = {0};
        if (prompt_input("GPIOEVT enable pin: ", value, sizeof(value)) == 0) {
            uint8_t pin = 0;
            if (parse_u8(value, &pin)) {
                send_gpioevt_cmd(app, GPIOEVT_OP_ENABLE, 1, pin);
            } else {
                app_log(app, "Invalid pin number");
            }
        }
    } else if (ch == 'u') {
        char value[32] = {0};
        if (prompt_input("GPIOEVT disable pin: ", value, sizeof(value)) == 0) {
            uint8_t pin = 0;
            if (parse_u8(value, &pin)) {
                send_gpioevt_cmd(app, GPIOEVT_OP_DISABLE, 1, pin);
            } else {
                app_log(app, "Invalid pin number");
            }
        }
    } else if (ch == 'k' || ch == 'K') {
        send_gpioevt_cmd(app, GPIOEVT_OP_CLEAR, 0, 0);
    } else if (ch == 'x') {
        char value[32] = {0};
        if (prompt_input("GPIO read pin: ", value, sizeof(value)) == 0) {
            uint8_t pin = 0;
            if (parse_u8(value, &pin)) {
                send_gpio_cmd(app, GPIO_OP_READ, pin, 0, 0, 0, 0);
            } else {
                app_log(app, "Invalid pin number");
            }
        }
    } else if (ch == 'X') {
        clear_advertisements_panel(app);
    } else if (ch == 'y' || ch == 'Y') {
        char pin_s[32] = {0};
        char lvl_s[32] = {0};
        if (prompt_input("GPIO write pin: ", pin_s, sizeof(pin_s)) == 0 &&
            prompt_input("Write level (0/1): ", lvl_s, sizeof(lvl_s)) == 0) {
            uint8_t pin = 0;
            uint8_t lvl = 0;
            if (parse_u8(pin_s, &pin) && parse_u8(lvl_s, &lvl) && (lvl == 0 || lvl == 1)) {
                send_gpio_cmd(app, GPIO_OP_WRITE, pin, 1, lvl, 0, 0);
            } else {
                app_log(app, "Invalid GPIO write input");
            }
        }
    } else if (ch == 't' || ch == 'T') {
        char value[32] = {0};
        if (prompt_input("GPIO toggle pin: ", value, sizeof(value)) == 0) {
            uint8_t pin = 0;
            if (parse_u8(value, &pin)) {
                ensure_gpio_output_for_toggle(app, pin);
                send_gpio_cmd(app, GPIO_OP_TOGGLE, pin, 0, 0, 0, 0);
                send_gpio_cmd(app, GPIO_OP_READ, pin, 0, 0, 0, 0);
            } else {
                app_log(app, "Invalid pin number");
            }
        }
    } else if (ch == 'f' || ch == 'F') {
        char value[64] = {0};
        if (prompt_input("GPIO cfg: pin in out pull (e.g. 9 1 0 1): ", value, sizeof(value)) == 0) {
            uint8_t pin = 0, in = 0, out = 0, pull = 0;
            unsigned int p = 0;
            if (sscanf(value, "%u", &p) == 1 && p <= 255) {
                pin = (uint8_t)p;
                const char *rest = strchr(value, ' ');
                if (rest != NULL && parse_u8_triplet(rest + 1, &in, &out, &pull) && pull <= 2) {
                    send_gpio_cmd(app, GPIO_OP_CONFIG, pin, 3, in ? 1 : 0, out ? 1 : 0, pull);
                        int idx = gpio_pin_index(pin);
                        if (idx >= 0) {
                            app->gpio_out_known[idx] = true;
                            app->gpio_out_mode[idx] = out ? true : false;
                        }
                } else {
                    app_log(app, "Invalid config format");
                }
            } else {
                app_log(app, "Invalid pin number");
            }
        }
    } else if (ch == 'z' || ch == 'Z') {
        char value[32] = {0};
        if (prompt_input("GPIO analog read pin (0..4): ", value, sizeof(value)) == 0) {
            uint8_t pin = 0;
            if (parse_u8(value, &pin)) {
                send_gpio_cmd(app, GPIO_OP_ANALOG_READ, pin, 0, 0, 0, 0);
            } else {
                app_log(app, "Invalid pin number");
            }
        }
    } else if (ch == 'n' || ch == 'N') {
        send_gpio_cmd(app, GPIO_OP_WRITE, BOARD_LED_GPIO, 1, 0, 0, 0);
        app_log(app, "LED ON (GPIO%d active-low)", BOARD_LED_GPIO);
    } else if (ch == 'o' || ch == 'O') {
        send_gpio_cmd(app, GPIO_OP_WRITE, BOARD_LED_GPIO, 1, 1, 0, 0);
        app_log(app, "LED OFF (GPIO%d active-low)", BOARD_LED_GPIO);
    } else if (ch == 'l') {
        ensure_gpio_output_for_toggle(app, BOARD_LED_GPIO);
        send_gpio_cmd(app, GPIO_OP_TOGGLE, BOARD_LED_GPIO, 0, 0, 0, 0);
        app_log(app, "LED toggle (GPIO%d)", BOARD_LED_GPIO);
    } else if (ch == '1') {
        app->phy_1m = !app->phy_1m;
        app_log(app, "PHY 1M %s", on_off(app->phy_1m));
    } else if (ch == '2') {
        app->phy_coded = !app->phy_coded;
        app_log(app, "PHY Coded %s", on_off(app->phy_coded));
    } else if (ch == 'a' || ch == 'A') {
        app->active_scan = !app->active_scan;
        app_log(app, "Active scan %s", on_off(app->active_scan));
    } else if (ch == 'd' || ch == 'D') {
        app->duplicate_filter = !app->duplicate_filter;
        app_log(app, "Duplicate filter %s", on_off(app->duplicate_filter));
    } else if (ch == 'r' || ch == 'R') {
        app->filter_random = !app->filter_random;
        app_log(app, "Filter random %s", on_off(app->filter_random));
    } else if (ch == 'p' || ch == 'P') {
        app->filter_private = !app->filter_private;
        app_log(app, "Filter private %s", on_off(app->filter_private));
    } else if (ch == 'm' || ch == 'M') {
        char value[32] = {0};
        if (prompt_input("Scan window ms (1..5000): ", value, sizeof(value)) == 0 && value[0] != '\0') {
            float v = strtof(value, NULL);
            if (v >= 1.0f && v <= 5000.0f) {
                app->window_ms = v;
                app_log(app, "Scan window %.1f ms", app->window_ms);
            } else {
                app_log(app, "Window out of range (1..5000)");
            }
        }
    } else if (ch == 'w' || ch == 'W') {
        char mac[64] = {0};
        if (prompt_input("Whitelist MAC/prefix: ", mac, sizeof(mac)) == 0 && mac[0] != '\0') {
            (void)mac_list_add_local(app, 0, mac);
            send_mac_cmd(app, CMD_ID_WMAC, mac);
        }
    } else if (ch == 'b') {
        char mac[64] = {0};
        if (prompt_input("Blacklist MAC/prefix: ", mac, sizeof(mac)) == 0 && mac[0] != '\0') {
            (void)mac_list_add_local(app, 1, mac);
            send_mac_cmd(app, CMD_ID_BMAC, mac);
        }
    } else {
        return false;
    }

    return true;
}

static int read_key_event(void) {
    int ch = getch();
    if (ch != 27) {
        return ch;
    }

    timeout(25);
    int c1 = getch();
    if (c1 == ERR) {
        timeout(30);
        return 27;
    }

    if (c1 == '[' || c1 == 'O') {
        int c2 = getch();
        if (c2 == ERR) {
            timeout(30);
            return 27;
        }
        if (c2 == 'A') { timeout(30); return KEY_UP; }
        if (c2 == 'B') { timeout(30); return KEY_DOWN; }
        if (c2 == 'C') { timeout(30); return KEY_RIGHT; }
        if (c2 == 'D') { timeout(30); return KEY_LEFT; }
        if (c2 == 'H') { timeout(30); return KEY_HOME; }
        if (c2 == 'F') { timeout(30); return KEY_END; }
        if (c2 >= '0' && c2 <= '9') {
            char seq[8] = {0};
            int len = 0;
            seq[len++] = (char)c2;
            while (len < (int)sizeof(seq) - 1) {
                int cx = getch();
                if (cx == ERR) {
                    break;
                }
                seq[len++] = (char)cx;
                if (cx == '~' || isalpha((unsigned char)cx)) {
                    break;
                }
            }
            timeout(30);
            if (strcmp(seq, "5~") == 0) return KEY_PPAGE;
            if (strcmp(seq, "6~") == 0) return KEY_NPAGE;
            if (strcmp(seq, "1~") == 0 || strcmp(seq, "7~") == 0) return KEY_HOME;
            if (strcmp(seq, "4~") == 0 || strcmp(seq, "8~") == 0) return KEY_END;
            return 27;
        }
    }

    timeout(30);
    return 27;
}

static void execute_cfg_cursor(App *app) {
    switch (app->main_cursor) {
        case 0: app->phy_1m = !app->phy_1m;
                app_log(app, "PHY 1M %s", on_off(app->phy_1m)); break;
        case 1: app->phy_coded = !app->phy_coded;
                app_log(app, "PHY Coded %s", on_off(app->phy_coded)); break;
        case 2: app->active_scan = !app->active_scan;
                app_log(app, "Active scan %s", on_off(app->active_scan)); break;
        case 3: app->duplicate_filter = !app->duplicate_filter;
                app_log(app, "Duplicate filter %s", on_off(app->duplicate_filter)); break;
        case 4: app->filter_random = !app->filter_random;
                app_log(app, "Filter random %s", on_off(app->filter_random)); break;
        case 5: app->filter_private = !app->filter_private;
                app_log(app, "Filter private %s", on_off(app->filter_private)); break;
        case 6: {
            char value[32] = {0};
            if (prompt_input("Scan window ms (1..5000): ", value, sizeof(value)) == 0 && value[0] != '\0') {
                float v = strtof(value, NULL);
                if (v >= 1.0f && v <= 5000.0f) {
                    app->window_ms = v;
                    app_log(app, "Scan window %.1f ms", app->window_ms);
                } else {
                    app_log(app, "Window out of range (1..5000)");
                }
            }
            break;
        }
        default: break;
    }
}

static bool execute_action_cursor(App *app) {
    switch (app->action_cursor) {
        case 0: app->show_help = !app->show_help; break;
        case 1: return true;
        case 2:
            if (app->scan_running) {
                send_scan_stop(app);
                app->adv_detail_open = false;
            } else {
                send_scan_start(app);
                app->adv_inspect_mode = false;
                app->adv_hscroll = 0;
                app->adv_follow_latest = true;
                app->adv_pause_until = 0.0;
            }
            break;
        case 3:
            app->show_gpio_panel = !app->show_gpio_panel;
            if (app->show_gpio_panel) {
                app->show_mac_panel = false;
                app->show_ble_panel = false;
                app->show_events_panel = false;
                app->show_txadv_panel = false;
                app->gpio_action_open = false;
                send_gpio_read_all(app);
                app->gpio_refresh_ts = now_sec();
            }
            break;
        case 4:
            app->show_ble_panel = !app->show_ble_panel;
            if (app->show_ble_panel) {
                app->show_mac_panel = false;
                app->show_gpio_panel = false;
                app->show_events_panel = false;
                app->show_txadv_panel = false;
                app->gpio_action_open = false;
            }
            break;
        case 5:
            app->show_mac_panel = !app->show_mac_panel;
            if (app->show_mac_panel) {
                app->show_gpio_panel = false;
                app->show_ble_panel = false;
                app->show_events_panel = false;
                app->show_txadv_panel = false;
                app->gpio_action_open = false;
            }
            break;
        case 6: {
            app->show_events_panel = !app->show_events_panel;
            if (app->show_events_panel) {
                app->show_gpio_panel = false;
                app->show_mac_panel = false;
                app->show_ble_panel = false;
                app->show_txadv_panel = false;
                app->gpio_action_open = false;
                send_gpioevt_cmd(app, GPIOEVT_OP_QUERY, 0, 0);
            }
            break;
        }
        case 7:
            app->show_txadv_panel = !app->show_txadv_panel;
            if (app->show_txadv_panel) {
                app->show_gpio_panel = false;
                app->show_mac_panel = false;
                app->show_ble_panel = false;
                app->show_events_panel = false;
                app->gpio_action_open = false;
            }
            break;
        case 8: {
            const uint8_t payload[] = {CMD_ID_INFO};
            serial_send_payload(app, payload, sizeof(payload));
            break;
        }
        case 9: {
            const uint8_t payload[] = {CMD_ID_CLRM};
            serial_send_payload(app, payload, sizeof(payload));
            break;
        }
        case 10: {
            const uint8_t payload[] = {CMD_ID_VBAT};
            serial_send_payload(app, payload, sizeof(payload));
            break;
        }
        case 11:
            send_gpio_cmd(app, GPIO_OP_WRITE, BOARD_LED_GPIO, 1, 0, 0, 0);
            app_log(app, "LED ON (GPIO%d active-low)", BOARD_LED_GPIO);
            break;
        case 12:
            send_gpio_cmd(app, GPIO_OP_WRITE, BOARD_LED_GPIO, 1, 1, 0, 0);
            app_log(app, "LED OFF (GPIO%d active-low)", BOARD_LED_GPIO);
            break;
        default: break;
    }
    return false;
}

static void draw_ui(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);
    erase();

    if (app->use_color) {
        attrset(COLOR_PAIR(1));
        bkgdset(' ' | COLOR_PAIR(1));
    }

    char last_seen[16] = "-";
    if (app->last_packet_ts > 0.0) {
        format_time(app->last_packet_ts, last_seen, sizeof(last_seen));
    }

    attron(A_BOLD);
    mvprintw(0, 0, "ADV_BLE2UART TUI");
    attroff(A_BOLD);

    mvprintw(0, 18,
             "Port:%s  Baud:%d  USB:%s  FW:0x%02X  MAC:%s",
             app->port,
             app->baud,
             app->usb_connected ? "on" : "off",
             app->fw_version,
             app->local_mac[0] ? app->local_mac : "-");

    if (app->scan_running) {
        attron(A_REVERSE | A_BOLD);
        mvprintw(0, w > 16 ? w - 16 : 0, " SCAN RUN ");
        attroff(A_REVERSE | A_BOLD);
    } else {
        attron(A_DIM);
        mvprintw(0, w > 17 ? w - 17 : 0, " scan stopped ");
        attroff(A_DIM);
    }

    mvprintw(1, 0,
             "ADV total:%llu  1M:%llu  Coded:%llu  Last:%s  Stats:%d  USB:%s",
             (unsigned long long)app->total_count,
             (unsigned long long)app->one_m_count,
             (unsigned long long)app->coded_count,
             last_seen,
             app->stats_count,
             app->usb_connected ? "on" : "off");
    /* Row 2: navigable scan config items */
    mvprintw(2, 0, "Cfg:");
    int _col = 4;
    for (int _ni = 0; _ni < NAV_COUNT && _col < w - 2; ++_ni) {
        char _val[16];
        switch (_ni) {
            case 0: snprintf(_val, sizeof(_val), "%s", on_off(app->phy_1m));          break;
            case 1: snprintf(_val, sizeof(_val), "%s", on_off(app->phy_coded));       break;
            case 2: snprintf(_val, sizeof(_val), "%s", on_off(app->active_scan));     break;
            case 3: snprintf(_val, sizeof(_val), "%s", on_off(app->duplicate_filter));break;
            case 4: snprintf(_val, sizeof(_val), "%s", on_off(app->filter_random));   break;
            case 5: snprintf(_val, sizeof(_val), "%s", on_off(app->filter_private));  break;
            default: snprintf(_val, sizeof(_val), "%.0fms", (double)app->window_ms);  break;
        }
        char _item[32];
        snprintf(_item, sizeof(_item), "[%s:%s]", nav_labels[_ni], _val);
        if (!app->scan_running && app->nav_row == 0 && _ni == app->main_cursor) { attron(A_REVERSE | A_BOLD); }
        mvprintw(2, _col, "%s", _item);
        if (!app->scan_running && app->nav_row == 0 && _ni == app->main_cursor) { attroff(A_REVERSE | A_BOLD); }
        _col += (int)strlen(_item) + 1;
    }

    if (app->show_mac_panel) {
        attron(A_REVERSE);
        mvprintw(3, 0,
                 " MAC lists: Up/Down select  Left/Right list  a add  e edit  d/Del remove  c clear active  C clear all  S sync  ESC/M close ");
        clrtoeol();
        attroff(A_REVERSE);
    } else if (app->show_events_panel) {
        attron(A_REVERSE);
        mvprintw(3, 0,
                 " Events panel: Up/Down pin  Enter toggle  e enable  d disable  g query mask  c clear all  ESC/E close ");
        clrtoeol();
        attroff(A_REVERSE);
    } else if (app->show_txadv_panel) {
        attron(A_REVERSE);
        mvprintw(3, 0,
                 " TXADV panel: Up/Down select  Left/Right PHY  Enter edit/exec  s start  x stop  i status  ESC/T close ");
        clrtoeol();
        attroff(A_REVERSE);
    } else if (app->show_ble_panel) {
        attron(A_REVERSE);
        mvprintw(3, 0,
                 " BLE panel: s status  1 connect-1M  2 connect-coded  d disconnect  c cancel  ESC close ");
        clrtoeol();
        attroff(A_REVERSE);
    } else if (app->show_gpio_panel) {
        attron(A_REVERSE);
        mvprintw(3, 0,
                 " GPIO panel: Up/Down select  Enter action  G/ESC close  j refresh  ");
        clrtoeol();
        attroff(A_REVERSE);
    } else if (app->scan_running && app->adv_detail_open) {
        attron(A_REVERSE);
        mvprintw(3, 0,
                 " Detail: ESC resume live  Up/Down previous/next ad  Left/Right pan long text ");
        clrtoeol();
        attroff(A_REVERSE);
    } else if (app->scan_running) {
        attron(A_REVERSE);
        if (app->adv_inspect_mode) {
            mvprintw(3, 0,
                     " Ads inspect: Up/Down scroll  PgUp/PgDn page  Left/Right x-scroll  Enter details  ESC resume live ");
        } else {
            mvprintw(3, 0,
                     " Ads live: Up/Down freeze+scroll  PgUp/PgDn page  Left/Right pan  Enter details  ESC pause 15s  G gpio panel ");
        }
        clrtoeol();
        attroff(A_REVERSE);
    } else if (app->adv_panel_nav) {
        attron(A_REVERSE);
        mvprintw(3, 0,
                 " Ads browse (scan stopped): Up/Down scroll  PgUp/PgDn page  Left/Right pan  Enter details  ESC back to menus ");
        clrtoeol();
        attroff(A_REVERSE);
    } else {
        mvprintw(3, 0, "Actions:");
        int _ac = 8;
        for (int _ai = 0; _ai < ACTION_COUNT && _ac < w - 2; ++_ai) {
            char _item[24];
            snprintf(_item, sizeof(_item), "[%s]", action_labels[_ai]);
            if (app->nav_row == 1 && _ai == app->action_cursor) { attron(A_REVERSE | A_BOLD); }
            mvprintw(3, _ac, "%s", _item);
            if (app->nav_row == 1 && _ai == app->action_cursor) { attroff(A_REVERSE | A_BOLD); }
            _ac += (int)strlen(_item) + 1;
        }
        clrtoeol();
    }

    if (app->show_mac_panel) {
        draw_mac_lists_panel(app);
    } else if (app->show_events_panel) {
        draw_events_panel(app);
    } else if (app->show_txadv_panel) {
        draw_txadv_panel(app);
    } else if (app->show_ble_panel) {
        draw_ble_panel(app);
    } else if (app->show_gpio_panel) {
        draw_gpio_panel(app);
    } else {
    int log_h = h >= 22 ? 8 : 5;
    int adv_top = 5;
    int adv_h = h - adv_top - log_h - 2;
    if (adv_h < 4) {
        adv_h = 4;
    }

    int panel_w = w;
    draw_box_unicode(adv_top - 1, 0, adv_h + 2, panel_w);
    attron(A_BOLD);
    mvprintw(adv_top - 1, 2, " Advertisements ");
    attroff(A_BOLD);

    if (app->adv_ring.count > 0) {
        if (app->adv_cursor < 0 || app->adv_follow_latest) {
            app->adv_cursor = app->adv_ring.count - 1;
        } else if (app->adv_cursor >= app->adv_ring.count) {
            app->adv_cursor = app->adv_ring.count - 1;
        }
        if (app->adv_cursor < 0) {
            app->adv_cursor = 0;
        }
    }

    attron(A_BOLD);
    char adv_meta[64];
    snprintf(adv_meta, sizeof(adv_meta), "[%d/%d x=%d ovf=%llu]",
             app->adv_ring.count > 0 ? app->adv_cursor + 1 : 0,
             app->adv_ring.count,
             app->adv_hscroll,
             (unsigned long long)app->adv_overflow_count);
    int adv_meta_x = panel_w - 2 - (int)strlen(adv_meta);
    if (adv_meta_x < 20) {
        adv_meta_x = 20;
    }
    mvprintw(adv_top - 1, adv_meta_x, "%s", adv_meta);
    attroff(A_BOLD);

    int adv_start = app->adv_cursor - adv_h + 1;
    if (adv_start < 0) adv_start = 0;
    if (adv_start > app->adv_ring.count - adv_h) {
        adv_start = app->adv_ring.count - adv_h;
    }
    if (adv_start < 0) adv_start = 0;

    for (int i = 0; i < adv_h; ++i) {
        int idx = adv_start + i;
        if (idx >= app->adv_ring.count) {
            break;
        }
        int adv_cap = (int)(sizeof(app->adv_ring.lines) / sizeof(app->adv_ring.lines[0]));
        int ring_idx = (app->adv_ring.head - app->adv_ring.count + idx + adv_cap) % adv_cap;
        const char *line = app->adv_ring.lines[ring_idx];
        int line_len = (int)strlen(line);
        int start_col = app->adv_hscroll;
        if (start_col < 0) start_col = 0;
        if (start_col > line_len) start_col = line_len;
        const char *view = line + start_col;
        if (idx == app->adv_cursor) {
            attron(A_REVERSE | A_BOLD);
            mvhline(adv_top + i, 1, ' ', w - 3);
            mvprintw(adv_top + i, 1, "%.*s", w - 3, view);
            attroff(A_REVERSE | A_BOLD);
        } else {
            mvhline(adv_top + i, 1, ' ', w - 3);
            draw_adv_line_soft_labels(adv_top + i, 1, w - 3, view);
        }
    }

    int log_top = adv_top + adv_h + 1;
    draw_box_custom(log_top - 1, 0, log_h + 2, panel_w,
                    ACS_LTEE, ACS_RTEE, ACS_LLCORNER, ACS_LRCORNER);
    attron(A_BOLD);
    mvprintw(log_top - 1, 2, " Log ");
    attroff(A_BOLD);
    int log_start = app->log_ring.count - log_h;
    if (log_start < 0) log_start = 0;
    for (int i = 0; i < log_h; ++i) {
        int idx = log_start + i;
        if (idx >= app->log_ring.count) {
            break;
        }
        int log_cap = (int)(sizeof(app->log_ring.lines) / sizeof(app->log_ring.lines[0]));
        int ring_idx = (app->log_ring.head - app->log_ring.count + idx + log_cap) % log_cap;
        mvprintw(log_top + i, 1, "%.*s", w - 3, app->log_ring.lines[ring_idx]);
    }
    } /* end non-panel main body */

    attron(A_DIM);
    {
        const char *footer = "h/? full help | GPIO pin: decimal or 0x | ESC cancella i prompt";
        int frame_w = w;
        int inner_w = frame_w - 4;
        if (inner_w > 0) {
            int footer_len = (int)strlen(footer);
            int shown = footer_len < inner_w ? footer_len : inner_w;
            mvprintw(h - 1, 2, "%.*s", shown, footer);
        }
    }
    attroff(A_DIM);

    if (app->show_help) {
        draw_help_overlay(app);
    }

    if (app->log_fullscreen) {
        draw_log_fullscreen_overlay(app);
    }

    if (app->adv_detail_open) {
        draw_adv_detail_overlay(app);
    }

    refresh();
}

static void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [--port /dev/ttyUSB0] [--baud 2000000] [--pulse-reset] [--connect] [--crc-debug]\n"
            "\n"
            "TUI commands:\n"
            "  h/? Toggle help\n"
            "  s Start/Stop scan\n"
            "  i INFO\n"
            "  c Clear MAC lists\n"
            "  w Add whitelist MAC/prefix\n"
            "  b Add blacklist MAC/prefix\n"
            "  v VBAT\n"
            "  g GPIOEVT query\n"
            "  e GPIOEVT enable pin\n"
            "  u GPIOEVT disable pin\n"
            "  k GPIOEVT clear all\n"
            "  U USB connect toggle on/off\n"
            "  B BLE panel quick toggle\n"
            "  E Events panel quick toggle\n"
            "  T TXADV panel quick toggle\n"
            "  --crc-debug Verbose CRC discard logs\n"
            "  x GPIO digital read\n"
            "  X Clear advertisements panel\n"
            "  y GPIO digital write\n"
            "  t GPIO digital toggle\n"
            "  f GPIO configure (in out pull)\n"
            "  z GPIO analog read\n"
            "  n/o/l LED on/off/toggle (GPIO8)\n"
            "  L Fullscreen log (ESC to return)\n"
            "  M MAC lists editor panel\n"
            "  1/2 Toggle 1M/Coded PHY\n"
            "  a Toggle active scan\n"
            "  d Toggle duplicate filter\n"
            "  r/p Toggle random/private address filters\n"
            "  m Set scan window (ms)\n"
            "  q Quit\n",
            prog);
}

static void draw_ble_panel(App *app) {
    int h, w;
    getmaxyx(stdscr, h, w);

    int panel_y = 4;
    int panel_h = h - panel_y - 1;
    if (panel_h < 8) {
        return;
    }

    for (int r = panel_y; r < panel_y + panel_h; ++r) {
        mvhline(r, 0, ' ', w);
    }

    draw_box_unicode(panel_y, 0, panel_h, w);
    attron(A_BOLD | A_REVERSE);
    mvprintw(panel_y, 2, " BLE Connect Panel ");
    attroff(A_BOLD | A_REVERSE);

    const char *state = "Idle";
    if (app->ble_conn_state == 1) {
        state = "Connecting";
    } else if (app->ble_conn_state == 2) {
        state = "Connected";
    }

    mvprintw(panel_y + 2, 2, "USB link      : %s", app->usb_connected ? "ON" : "OFF");
    mvprintw(panel_y + 3, 2, "BLE state     : %s (%u)", state, (unsigned)app->ble_conn_state);
    mvprintw(panel_y + 4, 2, "Peer addr type: %u", (unsigned)app->ble_peer_addr_type);
    mvprintw(panel_y + 5, 2, "ATT handle    : 0x%04X", (unsigned)app->ble_att_handle);
    mvprintw(panel_y + 6, 2, "Conn interval : %.2f ms", (double)app->ble_interval_units * 1.25);

    attron(A_DIM);
    mvprintw(panel_y + panel_h - 2, 2,
             "Keys: s status  1 connect 1M  2 connect coded  d disconnect  c cancel  ESC close");
    attroff(A_DIM);
}

static bool send_conn_open(App *app, bool coded) {
    char addr_type_s[16] = {0};
    char mac_s[64] = {0};
    uint8_t addr_type = 0;
    char hex[13];
    int nhex;
    int nbytes;
    uint8_t payload[8] = {0};

    draw_ui(app);
    if (prompt_input("BLE peer addr_type (0=PUBLIC,1=RANDOM): ", addr_type_s, sizeof(addr_type_s)) != 0) {
        return false;
    }
    if (!parse_u8(addr_type_s, &addr_type) || addr_type > 1) {
        app_log(app, "Invalid addr_type (use 0 or 1)");
        return false;
    }
    if (prompt_input("BLE peer MAC (AA:BB:CC:DD:EE:FF): ", mac_s, sizeof(mac_s)) != 0 || mac_s[0] == '\0') {
        return false;
    }
    nhex = normalize_mac_hex(mac_s, hex, sizeof(hex));
    if (nhex != 12) {
        app_log(app, "Invalid MAC: use exactly 6 bytes");
        return false;
    }

    nbytes = nhex / 2;
    payload[0] = CMD_ID_CONN;
    payload[1] = coded ? 2 : 1;
    payload[2] = addr_type;
    for (int i = 0; i < nbytes; ++i) {
        int src = (nbytes - 1 - i) * 2;
        payload[3 + i] = (uint8_t)parse_hex_pair(hex[src], hex[src + 1]);
    }
    strncpy(app->ble_peer_mac, mac_s, sizeof(app->ble_peer_mac) - 1);
    app->ble_peer_mac[sizeof(app->ble_peer_mac) - 1] = '\0';
    if (serial_send_payload(app, payload, 3 + (size_t)nbytes)) {
        app_log(app, "BLE connect request sent (%s)", coded ? "coded" : "1M");
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    const char *port = "/dev/ttyUSB0";
    int baud = 2000000;
    bool pulse_reset = false;
    bool cli_connect = false;
    bool crc_debug = false;

    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
            port = argv[++i];
        } else if ((strcmp(argv[i], "--baud") == 0 || strcmp(argv[i], "-b") == 0) && i + 1 < argc) {
            baud = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--pulse-reset") == 0) {
            pulse_reset = true;
        } else if (strcmp(argv[i], "--connect") == 0) {
            cli_connect = true;
        } else if (strcmp(argv[i], "--crc-debug") == 0) {
            crc_debug = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }

    static App app;
    memset(&app, 0, sizeof(app));
    app.fd = -1;
    app.port = port;
    app.baud = baud;
    app.phy_1m = true;
    app.phy_coded = true;
    app.window_ms = 30.0f;
    app.scan_start_wait_ack = false;
    app.scan_start_cmd_ts = 0.0;
    app.usb_connected = false;
    app.usb_pulse_reset = pulse_reset;
    app.crc_debug = crc_debug;
    app.adv_cursor = -1;
    app.adv_hscroll = 0;
    app.adv_follow_latest = true;
    app.adv_inspect_mode = false;
    app.adv_detail_open = false;
    app.adv_pause_until = 0.0;
    app.gpio_refresh_interval = 2.0f;
    app.gpio_cursor = 0;
    app.gpio_action_open = false;
    app.gpio_action_cursor = 0;
    app.main_cursor = 0;
    app.action_cursor = 0;
    app.nav_row = 1;
    app.show_mac_panel = false;
    app.show_ble_panel = false;
    app.show_events_panel = false;
    app.show_txadv_panel = false;
    app.events_cursor = 0;
    app.gpioevt_mask = 0;
    app.txadv_cursor = 0;
    app.txadv_phy = 0;
    app.txadv_interval_ms = 100.0f;
    snprintf(app.txadv_payload_hex, sizeof(app.txadv_payload_hex), "020106");
    app.txadv_running = false;
    app.txadv_last_status[0] = '\0';
    app.mac_active_list = 0;
    app.mac_cursor = 0;
    app.log_fullscreen = false;
    app.log_cursor = -1;
    app.log_hscroll = 0;
    for (int gi = 0; gi < GPIO_PANEL_COUNT; ++gi) {
        app.gpio_state[gi].digital = -1;
        app.gpio_state[gi].adc_raw = -1;
        app.gpio_out_known[gi] = false;
        app.gpio_out_mode[gi] = false;
    }

    if (cli_connect) {
        if (!usb_set_connected(&app, true, "CLI --connect")) {
            fprintf(stderr, "Cannot open %s: %s\n", port, strerror(errno));
            return 1;
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    notimeout(stdscr, FALSE);
    set_escdelay(120);
    timeout(30);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_WHITE, -1);
        app.use_color = true;
    }

    if (app.usb_connected) {
        app_log(&app, "USB connected to %s @ %d", port, baud);
    } else {
        app_log(&app, "USB disconnected (use U to connect, or start with --connect)");
    }
    app_log(&app, "Press 's' to start scan");

    while (!g_stop) {
        poll_serial(&app);

        if (app.scan_running && !app.adv_detail_open && app.adv_pause_until > 0.0 && now_sec() >= app.adv_pause_until) {
            app.adv_pause_until = 0.0;
            app.adv_inspect_mode = false;
            app.adv_hscroll = 0;
            if (app.adv_ring.count > 0) {
                app.adv_cursor = app.adv_ring.count - 1;
            }
            app.adv_follow_latest = true;
        }

        int ch = read_key_event();
        if (ch != ERR) {
            bool esc_key = (ch == 27);
            /* Global keys (always active) */
            if (ch == 'q' || ch == 'Q') {
                break;
            } else if (ch == 'h' || ch == 'H' || ch == '?') {
                app.show_help = !app.show_help;
            } else if (ch == 'L') {
                app.log_fullscreen = !app.log_fullscreen;
                if (app.log_fullscreen) {
                    app.show_help = false;
                    app.log_hscroll = 0;
                    if (app.log_ring.count > 0) {
                        app.log_cursor = app.log_ring.count - 1;
                    }
                }
            } else if (ch == 'B') {
                app.show_ble_panel = !app.show_ble_panel;
                if (app.show_ble_panel) {
                    app.show_mac_panel = false;
                    app.show_gpio_panel = false;
                    app.show_events_panel = false;
                    app.show_txadv_panel = false;
                    app.gpio_action_open = false;
                    app.show_help = false;
                    app.log_fullscreen = false;
                }
            } else if (ch == 'E') {
                app.show_events_panel = !app.show_events_panel;
                if (app.show_events_panel) {
                    app.show_mac_panel = false;
                    app.show_gpio_panel = false;
                    app.show_ble_panel = false;
                    app.show_txadv_panel = false;
                    app.gpio_action_open = false;
                    send_gpioevt_cmd(&app, GPIOEVT_OP_QUERY, 0, 0);
                }
            } else if (ch == 'T') {
                app.show_txadv_panel = !app.show_txadv_panel;
                if (app.show_txadv_panel) {
                    app.show_mac_panel = false;
                    app.show_gpio_panel = false;
                    app.show_ble_panel = false;
                    app.show_events_panel = false;
                    app.gpio_action_open = false;
                }
            } else if (app.show_help && esc_key) {
                app.show_help = false;
            } else if (app.show_help) {
                /* While help is visible, consume unrelated keys. */

            /* ---- Log fullscreen overlay ---- */
            } else if (app.log_fullscreen) {
                if (esc_key) {
                    app.log_fullscreen = false;
                } else if (ch == KEY_UP) {
                    if (app.log_cursor > 0) {
                        app.log_cursor--;
                    }
                } else if (ch == KEY_DOWN) {
                    if (app.log_cursor < app.log_ring.count - 1) {
                        app.log_cursor++;
                    }
                } else if (ch == KEY_PPAGE) {
                    int step = 8;
                    if (app.log_cursor >= step) {
                        app.log_cursor -= step;
                    } else {
                        app.log_cursor = 0;
                    }
                } else if (ch == KEY_NPAGE) {
                    int step = 8;
                    if (app.log_cursor < 0) {
                        app.log_cursor = 0;
                    }
                    if (app.log_cursor + step < app.log_ring.count) {
                        app.log_cursor += step;
                    } else if (app.log_ring.count > 0) {
                        app.log_cursor = app.log_ring.count - 1;
                    }
                } else if (ch == KEY_HOME) {
                    app.log_cursor = 0;
                } else if (ch == KEY_END) {
                    if (app.log_ring.count > 0) {
                        app.log_cursor = app.log_ring.count - 1;
                    }
                } else if (ch == KEY_LEFT) {
                    if (app.log_hscroll > 0) {
                        app.log_hscroll--;
                    }
                } else if (ch == KEY_RIGHT) {
                    app.log_hscroll += 4;
                }

            /* ---- MAC lists panel ---- */
            } else if (app.show_mac_panel) {
                int count = *mac_count_ptr(&app, app.mac_active_list);
                if (esc_key || ch == 'M') {
                    app.show_mac_panel = false;
                } else if (ch == KEY_LEFT) {
                    app.mac_active_list = 0;
                    app.mac_cursor = 0;
                } else if (ch == KEY_RIGHT) {
                    app.mac_active_list = 1;
                    app.mac_cursor = 0;
                } else if (ch == KEY_UP) {
                    if (app.mac_cursor > 0) app.mac_cursor--;
                } else if (ch == KEY_DOWN) {
                    if (app.mac_cursor + 1 < count) app.mac_cursor++;
                } else if (ch == 'a' || ch == 'A') {
                    char mac[64] = {0};
                    draw_ui(&app);
                    if (prompt_input("Add MAC/prefix: ", mac, sizeof(mac)) == 0 && mac[0] != '\0') {
                        (void)mac_list_add_local(&app, app.mac_active_list, mac);
                    }
                } else if (ch == 'e' || ch == 'E') {
                    if (count > 0 && app.mac_cursor >= 0 && app.mac_cursor < count) {
                        char mac[64] = {0};
                        char (*arr)[13] = mac_array_ptr(&app, app.mac_active_list);
                        snprintf(mac, sizeof(mac), "%s", arr[app.mac_cursor]);
                        draw_ui(&app);
                        if (prompt_input("Edit MAC/prefix: ", mac, sizeof(mac)) == 0 && mac[0] != '\0') {
                            (void)mac_list_edit_local(&app, app.mac_active_list, app.mac_cursor, mac);
                        }
                    }
                } else if (ch == 'd' || ch == 'D' || ch == KEY_DC || ch == KEY_BACKSPACE || ch == 127) {
                    if (count > 0 && app.mac_cursor >= 0 && app.mac_cursor < count) {
                        (void)mac_list_remove_local(&app, app.mac_active_list, app.mac_cursor);
                        if (app.mac_cursor >= *mac_count_ptr(&app, app.mac_active_list) && app.mac_cursor > 0) {
                            app.mac_cursor--;
                        }
                    }
                } else if (ch == 'c') {
                    mac_list_clear_local(&app, app.mac_active_list);
                    app.mac_cursor = 0;
                } else if (ch == 'C') {
                    app.whitelist_count = 0;
                    app.blacklist_count = 0;
                    app.mac_cursor = 0;
                    app_log(&app, "Whitelist and Blacklist cleared (local)");
                } else if (ch == 's' || ch == 'S') {
                    sync_mac_lists_to_firmware(&app);
                }

            /* ---- BLE connect panel ---- */
            } else if (app.show_ble_panel) {
                if (esc_key) {
                    app.show_ble_panel = false;
                } else if (ch == 's' || ch == 'S') {
                    const uint8_t payload[] = {CMD_ID_CONN, 0};
                    serial_send_payload(&app, payload, sizeof(payload));
                } else if (ch == '1') {
                    (void)send_conn_open(&app, false);
                } else if (ch == '2') {
                    (void)send_conn_open(&app, true);
                } else if (ch == 'd' || ch == 'D') {
                    const uint8_t payload[] = {CMD_ID_CONN, 3};
                    serial_send_payload(&app, payload, sizeof(payload));
                } else if (ch == 'c') {
                    const uint8_t payload[] = {CMD_ID_CONN, 4};
                    serial_send_payload(&app, payload, sizeof(payload));
                }

            /* ---- GPIO events panel ---- */
            } else if (app.show_events_panel) {
                if (esc_key || ch == 'E') {
                    app.show_events_panel = false;
                } else if (ch == KEY_UP) {
                    if (app.events_cursor > 0) {
                        app.events_cursor--;
                    }
                } else if (ch == KEY_DOWN) {
                    if (app.events_cursor < GPIO_PANEL_COUNT - 1) {
                        app.events_cursor++;
                    }
                } else if (ch == 'g' || ch == 'G') {
                    send_gpioevt_cmd(&app, GPIOEVT_OP_QUERY, 0, 0);
                } else if (ch == 'c' || ch == 'C') {
                    send_gpioevt_cmd(&app, GPIOEVT_OP_CLEAR, 0, 0);
                } else if (ch == 'e') {
                    send_gpioevt_cmd(&app, GPIOEVT_OP_ENABLE, 1, gpio_pins[app.events_cursor]);
                } else if (ch == 'd') {
                    send_gpioevt_cmd(&app, GPIOEVT_OP_DISABLE, 1, gpio_pins[app.events_cursor]);
                } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                    uint8_t pin = gpio_pins[app.events_cursor];
                    bool armed = (app.gpioevt_mask & (1u << pin)) != 0;
                    send_gpioevt_cmd(&app, armed ? GPIOEVT_OP_DISABLE : GPIOEVT_OP_ENABLE, 1, pin);
                }

            /* ---- TXADV panel ---- */
            } else if (app.show_txadv_panel) {
                if (esc_key || ch == 'T') {
                    app.show_txadv_panel = false;
                } else if (ch == KEY_UP) {
                    if (app.txadv_cursor > 0) {
                        app.txadv_cursor--;
                    }
                } else if (ch == KEY_DOWN) {
                    if (app.txadv_cursor < 5) {
                        app.txadv_cursor++;
                    }
                } else if (ch == KEY_LEFT && app.txadv_cursor == 0) {
                    if (app.txadv_phy > 0) {
                        app.txadv_phy--;
                    }
                } else if (ch == KEY_RIGHT && app.txadv_cursor == 0) {
                    if (app.txadv_phy < 2) {
                        app.txadv_phy++;
                    }
                } else if (ch == 's' || ch == 'S') {
                    send_txadv_start(&app);
                } else if (ch == 'x' || ch == 'X') {
                    send_txadv_stop(&app);
                } else if (ch == 'i' || ch == 'I') {
                    send_txadv_status(&app);
                } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                    if (app.txadv_cursor == 0) {
                        app.txadv_phy = (app.txadv_phy + 1) % 3;
                    } else if (app.txadv_cursor == 1) {
                        draw_ui(&app);
                        char val[16] = {0};
                        if (prompt_input("TXADV interval ms (>=20, ESC cancel): ", val, sizeof(val)) == 0) {
                            float x = strtof(val, NULL);
                            if (x >= 20.0f && x <= 10000.0f) {
                                app.txadv_interval_ms = x;
                            } else {
                                app_log(&app, "Invalid interval range (20..10000)");
                            }
                        }
                    } else if (app.txadv_cursor == 2) {
                        draw_ui(&app);
                        char hex[128] = {0};
                        snprintf(hex, sizeof(hex), "%s", app.txadv_payload_hex);
                        if (prompt_input("TXADV payload hex (ESC cancel): ", hex, sizeof(hex)) == 0) {
                            uint8_t tmp[31];
                            if (parse_hex_bytes(hex, tmp, (int)sizeof(tmp)) > 0) {
                                snprintf(app.txadv_payload_hex, sizeof(app.txadv_payload_hex), "%s", hex);
                            } else {
                                app_log(&app, "Invalid payload hex");
                            }
                        }
                    } else if (app.txadv_cursor == 3) {
                        send_txadv_start(&app);
                    } else if (app.txadv_cursor == 4) {
                        send_txadv_stop(&app);
                    } else if (app.txadv_cursor == 5) {
                        send_txadv_status(&app);
                    }
                }

            /* ---- GPIO panel: action popup ---- */
            } else if (app.show_gpio_panel && app.gpio_action_open) {
                uint8_t pin = gpio_pins[app.gpio_cursor];
                bool is_an  = gpio_analog[app.gpio_cursor];
                int action_count = is_an ? 7 : 6;
                if (app.gpio_action_cursor < 0) {
                    app.gpio_action_cursor = 0;
                }
                if (app.gpio_action_cursor >= action_count) {
                    app.gpio_action_cursor = action_count - 1;
                }

                int selected = -1;
                if (esc_key) { /* ESC */
                    app.gpio_action_open = false;
                } else if (ch == KEY_UP) {
                    if (app.gpio_action_cursor > 0) {
                        app.gpio_action_cursor--;
                    }
                } else if (ch == KEY_DOWN) {
                    if (app.gpio_action_cursor < action_count - 1) {
                        app.gpio_action_cursor++;
                    }
                } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                    selected = app.gpio_action_cursor;
                } else if (ch == 'r') {
                    selected = 0;
                } else if (ch == 'w') {
                    selected = 1;
                } else if (ch == 't') {
                    selected = 2;
                } else if (ch == 'b') {
                    selected = 3;
                } else if (ch == 'c') {
                    selected = 4;
                } else if (ch == 'a' && is_an) {
                    selected = 5;
                } else if (ch == 's') {
                    selected = is_an ? 6 : 5;
                }

                if (selected == 0) {
                    send_gpio_cmd(&app, GPIO_OP_READ, pin, 0, 0, 0, 0);
                    app.gpio_action_open = false;
                } else if (selected == 1) {
                    app.gpio_action_open = false;
                    draw_ui(&app);
                    char lvl_s[8] = {0};
                    if (prompt_input("Write level (0/1, ESC cancel): ", lvl_s, sizeof(lvl_s)) == 0) {
                        uint8_t lvl = 0;
                        if (parse_u8(lvl_s, &lvl) && (lvl == 0 || lvl == 1)) {
                            send_gpio_cmd(&app, GPIO_OP_WRITE, pin, 1, lvl, 0, 0);
                            app_log(&app, "GPIO%u write %u", pin, lvl);
                        } else {
                            app_log(&app, "Invalid level (use 0 or 1)");
                        }
                    } else {
                        app_log(&app, "GPIO write cancelled");
                    }
                } else if (selected == 2) {
                    ensure_gpio_output_for_toggle(&app, pin);
                    send_gpio_cmd(&app, GPIO_OP_TOGGLE, pin, 0, 0, 0, 0);
                    send_gpio_cmd(&app, GPIO_OP_READ, pin, 0, 0, 0, 0);
                    app_log(&app, "GPIO%u toggle", pin);
                    app.gpio_action_open = false;
                } else if (selected == 3) {
                    send_gpio_cmd(&app, GPIO_OP_WRITE, pin, 1, 1, 0, 0);
                    send_gpio_cmd(&app, GPIO_OP_WRITE, pin, 1, 0, 0, 0);
                    app_log(&app, "GPIO%u blink pulse", pin);
                    app.gpio_action_open = false;
                } else if (selected == 4) {
                    app.gpio_action_open = false;
                    draw_ui(&app);
                    char dir_s[8] = {0};
                    char pull_s[16] = {0};
                    if (prompt_input("Direction [i=input / o=output] (ESC cancel): ", dir_s, sizeof(dir_s)) != 0) {
                        app_log(&app, "GPIO config cancelled");
                    } else if (prompt_input("Pull [f=float / u=pull-up / d=pull-down] (ESC cancel): ", pull_s, sizeof(pull_s)) != 0) {
                        app_log(&app, "GPIO config cancelled");
                    } else {
                        uint8_t in = (dir_s[0] == 'i' || dir_s[0] == 'I') ? 1 : 0;
                        uint8_t out = (dir_s[0] == 'o' || dir_s[0] == 'O') ? 1 : 0;
                        uint8_t pull = 0;
                        if (pull_s[0] == 'u' || pull_s[0] == 'U') {
                            pull = 1;
                        } else if (pull_s[0] == 'd' || pull_s[0] == 'D') {
                            pull = 2;
                        } else if (!(pull_s[0] == 'f' || pull_s[0] == 'F')) {
                            app_log(&app, "Invalid pull mode (use f/u/d)");
                            continue;
                        }
                        if (in == 0 && out == 0) {
                            app_log(&app, "Invalid direction (use i or o)");
                            continue;
                        }
                        send_gpio_cmd(&app, GPIO_OP_CONFIG, pin, 3, in, out, pull);
                        int idx = gpio_pin_index(pin);
                        if (idx >= 0) {
                            app.gpio_out_known[idx] = true;
                            app.gpio_out_mode[idx] = out ? true : false;
                        }
                        app_log(&app, "GPIO%u config in=%u out=%u pull=%u", pin, in, out, pull);
                    }
                } else if (selected == 5 && is_an) {
                    send_gpio_cmd(&app, GPIO_OP_ANALOG_READ, pin, 0, 0, 0, 0);
                    app_log(&app, "GPIO%u analog read requested", pin);
                    app.gpio_action_open = false;
                } else if (selected == (is_an ? 6 : 5)) {
                    send_gpio_cmd(&app, GPIO_OP_STATUS, pin, 0, 0, 0, 0);
                    app.gpio_action_open = false;
                }

            /* ---- GPIO panel: navigation ---- */
            } else if (app.show_gpio_panel) {
                if (ch == KEY_UP) {
                    if (app.gpio_cursor > 0) { app.gpio_cursor--; }
                } else if (ch == KEY_DOWN) {
                    if (app.gpio_cursor < GPIO_PANEL_COUNT - 1) { app.gpio_cursor++; }
                } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                    app.gpio_action_open = true;
                    app.gpio_action_cursor = 0;
                } else if (esc_key || ch == 'G') {
                    app.show_gpio_panel   = false;
                    app.gpio_action_open  = false;
                } else if (ch == 'j' || ch == 'J') {
                    send_gpio_read_all(&app);
                    app.gpio_refresh_ts = now_sec();
                }

            /* ---- Advertisement view: scroll through packets while scan is running ---- */
            } else if (app.scan_running) {
                if ((app.adv_inspect_mode || app.adv_detail_open) && esc_key) {
                    /* ESC: leave inspect/detail and jump back to live tail */
                    app.adv_detail_open = false;
                    app.adv_inspect_mode = false;
                    app.adv_hscroll = 0;
                    app.adv_pause_until = 0.0;
                    if (app.adv_ring.count > 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                    app.adv_follow_latest = true;
                } else if (esc_key) {
                    /* ESC during live scan: pause auto-follow for 15 seconds. */
                    app.adv_detail_open = false;
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = now_sec() + 15.0;
                    if (app.adv_cursor < 0 && app.adv_ring.count > 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                } else if (ch == KEY_UP) {
                    if (app.adv_cursor > 0) {
                        app.adv_cursor--;
                    }
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = 0.0;
                } else if (ch == KEY_DOWN) {
                    if (app.adv_cursor < app.adv_ring.count - 1) {
                        app.adv_cursor++;
                    }
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = 0.0;
                } else if (ch == KEY_PPAGE) {
                    int step = 5;
                    if (app.adv_cursor >= step) {
                        app.adv_cursor -= step;
                    } else {
                        app.adv_cursor = 0;
                    }
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = 0.0;
                } else if (ch == KEY_NPAGE) {
                    int step = 5;
                    if (app.adv_cursor < 0) {
                        app.adv_cursor = 0;
                    }
                    if (app.adv_cursor + step < app.adv_ring.count) {
                        app.adv_cursor += step;
                    } else if (app.adv_ring.count > 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = 0.0;
                } else if (ch == KEY_LEFT) {
                    if (app.adv_hscroll > 0) {
                        app.adv_hscroll--;
                    }
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = 0.0;
                } else if (ch == KEY_RIGHT) {
                    app.adv_hscroll += 4;
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = 0.0;
                } else if (ch == KEY_HOME) {
                    app.adv_cursor = 0;
                    app.adv_inspect_mode = true;
                    app.adv_follow_latest = false;
                    app.adv_pause_until = 0.0;
                } else if (ch == KEY_END) {
                    if (app.adv_ring.count > 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                    app.adv_hscroll = 0;
                    app.adv_inspect_mode = false;
                    app.adv_follow_latest = true;
                    app.adv_pause_until = 0.0;
                } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                    if (app.adv_ring.count > 0) {
                        app.adv_detail_open = true;
                        app.adv_inspect_mode = true;
                        app.adv_pause_until = 0.0;
                    }
                } else {
                    (void)handle_command_hotkeys(&app, ch);
                }

            /* ---- Advertisement view: browse packets while scan is stopped ---- */
            } else if (app.adv_panel_nav) {
                if (esc_key) {
                    app.adv_detail_open = false;
                    app.adv_panel_nav = false;
                    app.adv_inspect_mode = false;
                    app.adv_hscroll = 0;
                } else if (ch == KEY_UP) {
                    if (app.adv_cursor > 0) {
                        app.adv_cursor--;
                    }
                    app.adv_follow_latest = false;
                } else if (ch == KEY_DOWN) {
                    if (app.adv_cursor < app.adv_ring.count - 1) {
                        app.adv_cursor++;
                    }
                    app.adv_follow_latest = false;
                } else if (ch == KEY_PPAGE) {
                    int step = 5;
                    if (app.adv_cursor >= step) {
                        app.adv_cursor -= step;
                    } else {
                        app.adv_cursor = 0;
                    }
                    app.adv_follow_latest = false;
                } else if (ch == KEY_NPAGE) {
                    int step = 5;
                    if (app.adv_cursor < 0) {
                        app.adv_cursor = 0;
                    }
                    if (app.adv_cursor + step < app.adv_ring.count) {
                        app.adv_cursor += step;
                    } else if (app.adv_ring.count > 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                    app.adv_follow_latest = false;
                } else if (ch == KEY_LEFT) {
                    if (app.adv_hscroll > 0) {
                        app.adv_hscroll--;
                    }
                    app.adv_follow_latest = false;
                } else if (ch == KEY_RIGHT) {
                    app.adv_hscroll += 4;
                    app.adv_follow_latest = false;
                } else if (ch == KEY_HOME) {
                    app.adv_cursor = 0;
                    app.adv_follow_latest = false;
                } else if (ch == KEY_END) {
                    if (app.adv_ring.count > 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                    app.adv_hscroll = 0;
                    app.adv_follow_latest = false;
                } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                    if (app.adv_ring.count > 0) {
                        app.adv_detail_open = true;
                    }
                } else {
                    (void)handle_command_hotkeys(&app, ch);
                }

            /* ---- Main view key handling (only before scan) ---- */
            } else if (ch == KEY_UP) {
                app.nav_row = 0;
            } else if (ch == KEY_DOWN) {
                if (app.nav_row == 1 && app.adv_ring.count > 0) {
                    app.adv_panel_nav = true;
                    app.adv_follow_latest = false;
                    if (app.adv_cursor < 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                } else {
                    app.nav_row = 1;
                }
            } else if (ch == KEY_PPAGE || ch == KEY_NPAGE) {
                if (app.adv_ring.count > 0) {
                    app.adv_panel_nav = true;
                    app.adv_follow_latest = false;
                    if (app.adv_cursor < 0) {
                        app.adv_cursor = app.adv_ring.count - 1;
                    }
                    if (ch == KEY_PPAGE) {
                        int step = 5;
                        if (app.adv_cursor >= step) app.adv_cursor -= step;
                        else app.adv_cursor = 0;
                    } else {
                        int step = 5;
                        if (app.adv_cursor + step < app.adv_ring.count) app.adv_cursor += step;
                        else app.adv_cursor = app.adv_ring.count - 1;
                    }
                }
            } else if (ch == KEY_LEFT) {
                if (app.nav_row == 0) {
                    if (app.main_cursor > 0) { app.main_cursor--; }
                    else { app.main_cursor = NAV_COUNT - 1; }
                } else {
                    if (app.action_cursor > 0) { app.action_cursor--; }
                    else { app.action_cursor = ACTION_COUNT - 1; }
                }
            } else if (ch == KEY_RIGHT) {
                if (app.nav_row == 0) {
                    if (app.main_cursor < NAV_COUNT - 1) { app.main_cursor++; }
                    else { app.main_cursor = 0; }
                } else {
                    if (app.action_cursor < ACTION_COUNT - 1) { app.action_cursor++; }
                    else { app.action_cursor = 0; }
                }
            } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                if (app.nav_row == 0) {
                    execute_cfg_cursor(&app);
                } else if (execute_action_cursor(&app)) {
                    break;
                }
            } else {
                (void)handle_command_hotkeys(&app, ch);
            }
        }

        draw_ui(&app);

        /* GPIO panel auto-refresh */
        if (app.show_gpio_panel) {
            double t = now_sec();
            if (t - app.gpio_refresh_ts >= (double)app.gpio_refresh_interval) {
                send_gpio_read_all(&app);
                app.gpio_refresh_ts = t;
            }
        }

        /* timeout(30) already provides loop pacing. */
    }

    if (app.scan_running) {
        send_scan_stop(&app);
        usleep(80000);
    }

    endwin();
    if (app.fd >= 0) {
        close(app.fd);
    }
    return 0;
}
