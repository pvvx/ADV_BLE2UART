#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define RX_BUFFER_SIZE 8192
#define COMMAND_SETTLE_US 50000U

#define CMD_ID_INFO 0x00U
#define CMD_ID_SCAN 0x01U
#define CMD_ID_GPIO 0x06U
#define CMD_ID_LED 0x07U
#define CMD_ID_UART 0x08U
#define CMD_ID_RFSDK 0x09U
#define CMD_ID_VERSION 0x0AU
#define CMD_ID_TXADV 0x0BU
#define CMD_ID_CONN 0x0CU
#define CMD_ID_TXDATA 0x0DU
#define CMD_ID_RXDATA 0x0EU
#define CMD_ID_VBAT 0x0FU
#define CMD_ID_GPIOEVT 0x10U

/* CMD_ID_GPIO sub-operations */
#define GPIO_OP_STATUS 0U
#define GPIO_OP_READ 1U
#define GPIO_OP_WRITE 2U
#define GPIO_OP_TOGGLE 3U
#define GPIO_OP_CONFIG 4U
#define GPIO_OP_PWM 5U
#define GPIO_OP_PWM_OFF 6U
#define GPIO_OP_ANALOG 7U
#define GPIO_OP_RGB 8U

/* CMD_ID_GPIOEVT sub-operations */
#define GPIOEVT_OP_QUERY 0U
#define GPIOEVT_OP_ENABLE 1U
#define GPIOEVT_OP_DISABLE 2U
#define GPIOEVT_OP_CLEAR 3U

/* High bit of byte[2] marking a spontaneous GPIOEVT event */
#define GPIOEVT_EVENT_FLAG 0x80U

/* TB-03F-KIT pin codes */
#define PIN_RGB_BLUE 0x22U
#define PIN_RGB_RED 0x23U
#define PIN_RGB_GREEN 0x24U
#define PIN_ADC_FREE 0x16U
#define PIN_KEY_PA7 0x07U

#define SCAN_PHY_1M 0x01U
#define SCAN_PHY_CODED 0x02U

static volatile sig_atomic_t g_stop_requested = 0;
static bool g_battery_response_received = false;
static uint8_t g_battery_status = 0xFFU;
static unsigned int g_battery_mv = 0U;

static const uint8_t CMD_INFO[] = {0x00};
static const uint8_t CMD_CLEAR_MAC_LIST[] = {0x04};
static const uint8_t CMD_BATTERY[] = {CMD_ID_VBAT};
static const uint8_t CMD_START_SCAN[] = {CMD_ID_SCAN, SCAN_PHY_1M | SCAN_PHY_CODED, 0x30, 0x00};
static const uint8_t CMD_STOP_SCAN[] = {CMD_ID_SCAN, 0x00, 0x00, 0x00};

static int hex_nibble(int ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return -1;
}

static int build_white_list_command(const char *mac_filter, uint8_t *cmd, size_t *cmd_len)
{
    uint8_t mac_be[6];
    size_t hex_len = 0;
    size_t mac_len;
    char normalized[12];

    for (const char *cursor = mac_filter; *cursor != '\0'; ++cursor) {
        if (*cursor == ':' || *cursor == '-' || *cursor == '.' || isspace((unsigned char)*cursor)) {
            continue;
        }

        if (!isxdigit((unsigned char)*cursor) || hex_len >= sizeof(normalized)) {
            errno = EINVAL;
            return -1;
        }

        normalized[hex_len++] = *cursor;
    }

    if (hex_len == 0U || (hex_len % 2U) != 0U) {
        errno = EINVAL;
        return -1;
    }

    mac_len = hex_len / 2U;
    if (mac_len == 0U || mac_len > sizeof(mac_be)) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < mac_len; ++i) {
        int hi = hex_nibble(normalized[i * 2U]);
        int lo = hex_nibble(normalized[i * 2U + 1U]);
        if (hi < 0 || lo < 0) {
            errno = EINVAL;
            return -1;
        }
        mac_be[i] = (uint8_t)((hi << 4) | lo);
    }

    cmd[0] = 0x02;
    for (size_t i = 0; i < mac_len; ++i) {
        cmd[1U + i] = mac_be[mac_len - 1U - i];
    }
    *cmd_len = mac_len + 1U;

    return 0;
}

/* ---------------------------------------------------------------------------
 * Command actions: the command-line options are collected while parsing argv
 * and sent in order once the serial port is open.
 * ------------------------------------------------------------------------- */
#define MAX_ACTIONS     32
#define MAX_ACTION_LEN  40

static uint8_t g_actions[MAX_ACTIONS][MAX_ACTION_LEN];
static size_t  g_action_len[MAX_ACTIONS];
static int     g_action_count;
static uint8_t g_last_gpioevt_op;   /* 0xFF = none, used to decode the ack layout */

static int add_action(const uint8_t *cmd, size_t len)
{
    if (g_action_count >= MAX_ACTIONS || len == 0U || len > MAX_ACTION_LEN) {
        return -1;
    }
    memcpy(g_actions[g_action_count], cmd, len);
    g_action_len[g_action_count] = len;
    ++g_action_count;
    return 0;
}

static int parse_hex_bytes(const char *spec, uint8_t *out, size_t cap, size_t *out_len)
{
    size_t n = 0;
    int hi = -1;

    for (const char *p = spec; *p != '\0'; ++p) {
        int nib;

        if (*p == ' ' || *p == ',' || *p == ':' || *p == '-') {
            continue;
        }
        nib = hex_nibble((unsigned char)*p);
        if (nib < 0) {
            return -1;
        }
        if (hi < 0) {
            hi = nib;
        } else {
            if (n >= cap) {
                return -1;
            }
            out[n++] = (uint8_t)((hi << 4) | nib);
            hi = -1;
        }
    }
    if (hi >= 0) {
        return -1;      /* odd number of hex digits */
    }
    *out_len = n;
    return 0;
}

/* Full MAC address (12 hex digits) converted to the little-endian wire order. */
static int parse_mac_le(const char *text, uint8_t out[6])
{
    uint8_t be[6];
    char norm[12];
    size_t n = 0;

    for (const char *p = text; *p != '\0'; ++p) {
        if (*p == ':' || *p == '-' || *p == '.' || isspace((unsigned char)*p)) {
            continue;
        }
        if (!isxdigit((unsigned char)*p) || n >= sizeof(norm)) {
            return -1;
        }
        norm[n++] = *p;
    }
    if (n != 12U) {
        return -1;
    }
    for (size_t i = 0; i < 6U; ++i) {
        int hi = hex_nibble((unsigned char)norm[i * 2U]);
        int lo = hex_nibble((unsigned char)norm[i * 2U + 1U]);
        if (hi < 0 || lo < 0) {
            return -1;
        }
        be[i] = (uint8_t)((hi << 4) | lo);
    }
    for (size_t i = 0; i < 6U; ++i) {
        out[i] = be[5U - i];        /* little-endian on the wire */
    }
    return 0;
}

/* Reads the next integer of a "a:b:c" specification and advances the cursor. */
static int parse_next_int(const char **cursor, long *value)
{
    char *end = NULL;
    long v = strtol(*cursor, &end, 0);

    if (end == *cursor) {
        return -1;
    }
    *value = v;
    if (*end == ':' || *end == ',' || *end == ' ') {
        *cursor = end + 1;
    } else {
        *cursor = end;
    }
    return 0;
}

static const uint8_t CMD_VERSION_REQ[] = {CMD_ID_VERSION};
static const uint8_t CMD_RFSDK_STATUS_REQ[] = {CMD_ID_RFSDK, 0x00U};
static const uint8_t CMD_UART_STATUS_REQ[] = {CMD_ID_UART, 0x00U};
static const uint8_t CMD_GPIOEVT_QUERY_REQ[] = {CMD_ID_GPIOEVT, GPIOEVT_OP_QUERY};
static const uint8_t CMD_GPIOEVT_CLEAR_REQ[] = {CMD_ID_GPIOEVT, GPIOEVT_OP_CLEAR};
static const uint8_t CMD_TXADV_STOP_REQ[] = {CMD_ID_TXADV, 0x00U};
static const uint8_t CMD_TXADV_STATUS_REQ[] = {CMD_ID_TXADV, 0x0FU};
static const uint8_t CMD_CONN_STATUS_REQ[] = {CMD_ID_CONN, 0x00U};
static const uint8_t CMD_CONN_CLOSE_REQ[] = {CMD_ID_CONN, 0x03U};
static const uint8_t CMD_CONN_DISCOVER_REQ[] = {CMD_ID_CONN, 0x05U};

static void on_signal(int signo)
{
    (void)signo;
    g_stop_requested = 1;
}

static uint16_t crc16_modbus(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 1U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

static int write_all(int fd, const uint8_t *data, size_t len)
{
    while (len > 0) {
        ssize_t written = write(fd, data, len);

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        data += (size_t)written;
        len -= (size_t)written;
    }

    return 0;
}

static int send_command(int fd, const uint8_t *cmd, size_t cmd_len)
{
    uint8_t frame[64];
    uint16_t crc;

    if (cmd_len + 2U > sizeof(frame)) {
        errno = EMSGSIZE;
        return -1;
    }

    memcpy(frame, cmd, cmd_len);
    crc = crc16_modbus(cmd, cmd_len);
    frame[cmd_len] = (uint8_t)(crc & 0xFFU);
    frame[cmd_len + 1U] = (uint8_t)(crc >> 8U);

    if (write_all(fd, frame, cmd_len + 2U) < 0) {
        return -1;
    }

    return 0;
}

static int send_command_with_settle(int fd, const uint8_t *cmd, size_t cmd_len)
{
    if (send_command(fd, cmd, cmd_len) < 0) {
        return -1;
    }

    if (ioctl(fd, TCSBRK, 1) < 0) {
        return -1;
    }

    usleep(COMMAND_SETTLE_US);
    return 0;
}

static int get_termios_speed(unsigned int baudrate, speed_t *speed)
{
    switch (baudrate) {
    case 115200U:
        *speed = B115200;
        return 0;
    case 921600U:
        *speed = B921600;
        return 0;
    case 2000000U:
        *speed = B2000000;
        return 0;
    default:
        errno = EINVAL;
        return -1;
    }
}

static int set_serial_speed_linux(int fd, unsigned int baudrate)
{
    struct termios tio;
    speed_t speed;

    if (get_termios_speed(baudrate, &speed) < 0) {
        return -1;
    }

    if (tcgetattr(fd, &tio) < 0) {
        return -1;
    }

    cfmakeraw(&tio);
    tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
    tio.c_cflag |= CS8 | CLOCAL | CREAD;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if (cfsetispeed(&tio, speed) < 0 || cfsetospeed(&tio, speed) < 0) {
        return -1;
    }

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        return -1;
    }

    return 0;
}

static void pulse_dtr_rts(int fd)
{
    int modem_bits;

    if (ioctl(fd, TIOCMGET, &modem_bits) < 0) {
        return;
    }

    modem_bits |= TIOCM_DTR | TIOCM_RTS;
    if (ioctl(fd, TIOCMSET, &modem_bits) < 0) {
        return;
    }

    usleep(50000);

    modem_bits &= ~(TIOCM_DTR | TIOCM_RTS);
    if (ioctl(fd, TIOCMSET, &modem_bits) < 0) {
        return;
    }

    usleep(50000);
}

static void drain_input(int fd)
{
    uint8_t scratch[512];

    for (;;) {
        struct pollfd pfd = {
            .fd = fd,
            .events = POLLIN,
            .revents = 0,
        };

        int poll_rc = poll(&pfd, 1, 50);
        if (poll_rc <= 0) {
            return;
        }

        if ((pfd.revents & POLLIN) == 0) {
            return;
        }

        ssize_t rd = read(fd, scratch, sizeof(scratch));
        if (rd <= 0) {
            return;
        }
    }
}

static void format_mac_le(const uint8_t *data, size_t data_len, char *out, size_t out_len)
{
    size_t used_len = data_len;
    size_t offset = 0;

    if (used_len == 0U || used_len > 6U) {
        used_len = 6U;
    }

    for (size_t i = 0; i < used_len; ++i) {
        int written = snprintf(
            out + offset,
            out_len > offset ? out_len - offset : 0U,
            "%s%02X",
            i == 0U ? "" : ":",
            data[used_len - 1U - i]
        );

        if (written < 0) {
            break;
        }

        if ((size_t)written >= out_len - offset) {
            offset = out_len > 0U ? out_len - 1U : 0U;
            break;
        }

        offset += (size_t)written;
    }
}

static void print_hex(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        printf("%02X", data[i]);
    }
}

static const char *event_type_name(uint8_t event_type)
{
    switch (event_type) {
    case 0x00:
        return "ADV_IND";
    case 0x01:
        return "DIRECT_IND";
    case 0x02:
        return "SCAN_IND";
    case 0x03:
        return "NONCONN_IND";
    case 0x04:
        return "SCAN_RSP";
    default:
        return "UNKNOWN";
    }
}

static const char *address_type_name(uint8_t address_type)
{
    switch (address_type & 0x0FU) {
    case 0x00:
        return "PUBLIC";
    case 0x01:
        return "RANDOM";
    case 0x02:
        return "RPA_PUBLIC";
    case 0x03:
        return "RPA_RANDOM";
    default:
        return "UNKNOWN";
    }
}

static const char *phys_name(uint8_t phys)
{
    switch (phys) {
    case 0x00:
        return "BT4.2";
    case 0x01:
        return "1M";
    case 0x02:
        return "2M";
    case 0x03:
        return "CODED";
    default:
        return "MIXED";
    }
}

static const char *cmd_status_name(uint8_t status)
{
    switch (status) {
    case 0x00:
        return "OK";
    case 0x01:
        return "ARGS";
    case 0x02:
        return "PIN";
    case 0x03:
        return "DENIED";
    case 0x04:
        return "VALUE";
    default:
        return "UNKNOWN";
    }
}

static void print_timestamp(void)
{
    struct timespec ts;
    struct tm tm_now;
    char buffer[32];

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return;
    }

    if (localtime_r(&ts.tv_sec, &tm_now) == NULL) {
        return;
    }

    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_now) == 0) {
        return;
    }

    printf("[%s.%03ld] ", buffer, ts.tv_nsec / 1000000L);
}

static void handle_command_response(const uint8_t *frame, size_t payload_len)
{
    uint8_t cmd_id = frame[1];
    uint8_t id = frame[2];
    uint8_t data_len = frame[3];
    const uint8_t *data = &frame[5];
    char mac_text[18];

    if (data_len > 6U) {
        data_len = 6U;
    }

    print_timestamp();

    if (cmd_id == 0x05 && frame[2] == 0xFF && frame[3] == 0xFF && payload_len > 0U) {
        printf("DBG ");
        fwrite(&frame[11], 1, payload_len, stdout);
        putchar('\n');
        fflush(stdout);
        return;
    }

    switch (cmd_id) {
    case 0x00:
        format_mac_le(data, 6U, mac_text, sizeof(mac_text));
        printf(
            "INFO version=%u.%u local_mac=%s\n",
            (unsigned int)((id >> 4U) & 0x0FU),
            (unsigned int)(id & 0x0FU),
            mac_text
        );
        break;
    case 0x01:
        if (data_len >= 3U && data[0] != 0U) {
            unsigned int window_units = (unsigned int)data[1] | ((unsigned int)data[2] << 8U);
            double window_ms = (double)window_units * 0.625;
            printf(
                "SCAN enabled filters=%u mode=0x%02X window_ms=%.3f\n",
                (unsigned int)id,
                (unsigned int)data[0],
                window_ms
            );
        } else {
            printf("SCAN disabled filters=%u\n", (unsigned int)id);
        }
        break;
    case 0x02:
    case 0x03:
        format_mac_le(data, data_len, mac_text, sizeof(mac_text));
        printf(
            "%s position=%u %s=%s\n",
            cmd_id == 0x02 ? "WHITE_LIST" : "BLACK_LIST",
            (unsigned int)id,
            data_len == 6U ? "mac" : "prefix",
            mac_text
        );
        break;
    case 0x04:
        printf("CLEAR_MAC_LIST max_entries=%u\n", (unsigned int)id);
        break;
    case CMD_ID_VBAT: {
        unsigned int batt_mv = data_len >= 2U
            ? (unsigned int)data[0] | ((unsigned int)data[1] << 8U)
            : 0U;
        int temp_c = 0;
        bool have_temp = false;

        if (data_len >= 4U) {
            temp_c = (int)((unsigned int)data[2] | ((unsigned int)data[3] << 8U));
            if ((temp_c & 0x8000) != 0) {
                temp_c -= 0x10000;
            }
            if (temp_c != -32768) {
                have_temp = true;
            }
        }

        g_battery_response_received = true;
        g_battery_status = id;
        g_battery_mv = batt_mv;
        if (have_temp) {
            printf(
                "VBAT status=%s(0x%02X) mv=%u temp_c=%d\n",
                cmd_status_name(id),
                (unsigned int)id,
                batt_mv,
                temp_c
            );
        } else if (data_len >= 4U) {
            printf(
                "VBAT status=%s(0x%02X) mv=%u temp_c=unavailable\n",
                cmd_status_name(id),
                (unsigned int)id,
                batt_mv
            );
        } else {
            printf(
                "VBAT status=%s(0x%02X) mv=%u\n",
                cmd_status_name(id),
                (unsigned int)id,
                batt_mv
            );
        }
        break;
    }
    case CMD_ID_GPIO:
        if (data_len >= 4U) {
            printf(
                "GPIO status=%s(0x%02X) op=%u pin=0x%02X value=%u caps=0x%02X\n",
                cmd_status_name(id),
                (unsigned int)id,
                (unsigned int)data[0],
                (unsigned int)data[1],
                (unsigned int)data[2],
                (unsigned int)data[3]
            );
        } else {
            printf("GPIO status=%s(0x%02X)\n", cmd_status_name(id), (unsigned int)id);
        }
        break;
    case CMD_ID_LED:
        printf("LED status=%s(0x%02X) data=", cmd_status_name(id), (unsigned int)id);
        print_hex(data, data_len);
        putchar('\n');
        break;
    case CMD_ID_UART:
        if (data_len >= 6U) {
            unsigned int baud = (unsigned int)data[3] | ((unsigned int)data[4] << 8U)
                              | ((unsigned int)data[5] << 16U);
            printf(
                "UART status=%s(0x%02X) op=%u index=%u count=%u baud=%u\n",
                cmd_status_name(id),
                (unsigned int)id,
                (unsigned int)data[0],
                (unsigned int)data[1],
                (unsigned int)data[2],
                baud
            );
        } else {
            printf("UART status=%s(0x%02X)\n", cmd_status_name(id), (unsigned int)id);
        }
        break;
    case CMD_ID_RFSDK:
        if (data_len >= 6U) {
            printf(
                "RFSDK status=%s(0x%02X) power=0x%02X cap=0x%02X ch=%u,%u,%u coded_min=%u x10ms\n",
                cmd_status_name(id),
                (unsigned int)id,
                (unsigned int)data[0],
                (unsigned int)data[1],
                (unsigned int)data[2],
                (unsigned int)data[3],
                (unsigned int)data[4],
                (unsigned int)data[5]
            );
        } else {
            printf("RFSDK status=%s(0x%02X)\n", cmd_status_name(id), (unsigned int)id);
        }
        break;
    case CMD_ID_VERSION:
        if (data_len >= 6U) {
            printf(
                "VERSION fw=%u.%u hw=0x%02X cert=%u struct=%u sdk=%u.%u.%u\n",
                (unsigned int)((id >> 4U) & 0x0FU),
                (unsigned int)(id & 0x0FU),
                (unsigned int)data[0],
                (unsigned int)data[1],
                (unsigned int)data[2],
                (unsigned int)data[3],
                (unsigned int)data[4],
                (unsigned int)data[5]
            );
        } else {
            printf(
                "VERSION fw=%u.%u\n",
                (unsigned int)((id >> 4U) & 0x0FU),
                (unsigned int)(id & 0x0FU)
            );
        }
        break;
    case CMD_ID_TXADV:
        if (data_len >= 4U) {
            unsigned int interval = (unsigned int)data[1] | ((unsigned int)data[2] << 8U);
            printf(
                "TXADV status=%s(0x%02X) running=%u interval=%u adv_len=%u\n",
                cmd_status_name(id),
                (unsigned int)id,
                (unsigned int)data[0],
                interval,
                (unsigned int)data[3]
            );
        } else {
            printf("TXADV status=%s(0x%02X)\n", cmd_status_name(id), (unsigned int)id);
        }
        break;
    case CMD_ID_CONN:
        if ((id & 0x80U) != 0U) {
            /* spontaneous discovery / read result */
            printf("CONN event sub=%u data=", (unsigned int)(id & 0x7FU));
            print_hex(data, data_len);
            if (payload_len > 0U) {
                print_hex(&frame[11], payload_len);
            }
            putchar('\n');
        } else if (data_len >= 4U) {
            static const char *const states[] = {"idle", "connecting", "connected"};
            unsigned int state = (unsigned int)data[0];
            printf(
                "CONN status=%s(0x%02X) state=%s peer_type=%u\n",
                cmd_status_name(id),
                (unsigned int)id,
                state < 3U ? states[state] : "?",
                (unsigned int)data[1]
            );
        } else {
            printf("CONN status=%s(0x%02X)\n", cmd_status_name(id), (unsigned int)id);
        }
        break;
    case CMD_ID_TXDATA:
        printf("TXDATA status=%s(0x%02X)\n", cmd_status_name(id), (unsigned int)id);
        break;
    case CMD_ID_RXDATA: {
        unsigned int handle = data_len >= 2U
            ? (unsigned int)data[0] | ((unsigned int)data[1] << 8U) : 0U;
        unsigned int vlen = data_len >= 3U ? (unsigned int)data[2] : 0U;
        printf(
            "RXDATA %s handle=0x%04X len=%u data=",
            frame[2] == 0x1DU ? "indication" : "notification",
            handle,
            vlen
        );
        if (data_len > 3U) {
            print_hex(&data[3], data_len - 3U);
        }
        if (payload_len > 0U) {
            print_hex(&frame[11], payload_len);
        }
        putchar('\n');
        break;
    }
    case CMD_ID_GPIOEVT:
        if ((id & GPIOEVT_EVENT_FLAG) != 0U && data_len >= 6U) {
            unsigned int pin_id = data[0];
            unsigned int level = data[1];
            unsigned int ts = (unsigned int)data[2] | ((unsigned int)data[3] << 8U)
                            | ((unsigned int)data[4] << 16U) | ((unsigned int)data[5] << 24U);
            printf(
                "GPIOEVT pin=0x%02X %s level=%u t=%u ms\n",
                pin_id,
                level ? "rising" : "falling",
                level,
                ts
            );
        } else {
            unsigned int mask = 0U;
            if (g_last_gpioevt_op == GPIOEVT_OP_QUERY || g_last_gpioevt_op == GPIOEVT_OP_CLEAR) {
                if (data_len >= 4U) {
                    mask = (unsigned int)data[0] | ((unsigned int)data[1] << 8U)
                         | ((unsigned int)data[2] << 16U) | ((unsigned int)data[3] << 24U);
                }
            } else if (data_len >= 5U) {
                mask = (unsigned int)data[1] | ((unsigned int)data[2] << 8U)
                     | ((unsigned int)data[3] << 16U) | ((unsigned int)data[4] << 24U);
            }
            printf(
                "GPIOEVT status=%s(0x%02X) armed=0x%08X\n",
                cmd_status_name(id),
                (unsigned int)id,
                mask
            );
        }
        break;
    default:
        printf(
            "CMD cmd=0x%02X id=0x%02X data=",
            (unsigned int)cmd_id,
            (unsigned int)id
        );
        print_hex(data, data_len);
        putchar('\n');
        break;
    }

    fflush(stdout);
}

static void handle_advertisement(const uint8_t *frame, size_t payload_len)
{
    int8_t rssi = (int8_t)frame[1];
    uint8_t event_type = frame[2];
    uint8_t address_type = frame[3];
    uint8_t phys = frame[4];
    char mac_text[18];

    format_mac_le(&frame[5], 6U, mac_text, sizeof(mac_text));

    print_timestamp();
    printf(
        "ADV rssi=%d event=%s(0x%02X) addr=%s(0x%02X) phys=%s(0x%02X) mac=%s payload=",
        rssi,
        event_type_name(event_type),
        (unsigned int)event_type,
        address_type_name(address_type),
        (unsigned int)address_type,
        phys_name(phys),
        (unsigned int)phys,
        mac_text
    );
    print_hex(&frame[11], payload_len);
    putchar('\n');
    fflush(stdout);
}

static void process_rx_buffer(uint8_t *buffer, size_t *used, bool *synced)
{
    while (*used >= 13U) {
        size_t payload_len = buffer[0];
        size_t frame_len = payload_len + 13U;

        if (frame_len > *used) {
            return;
        }

        if (crc16_modbus(buffer, frame_len) == 0U) {
            *synced = true;
            if (buffer[4] == 0xFFU) {
                handle_command_response(buffer, payload_len);
            } else {
                handle_advertisement(buffer, payload_len);
            }

            memmove(buffer, buffer + frame_len, *used - frame_len);
            *used -= frame_len;
            continue;
        }

        if (*synced) {
            fprintf(stderr, "CRC error, discarded byte 0x%02X\n", buffer[0]);
        }

        memmove(buffer, buffer + 1, *used - 1U);
        --(*used);
    }
}

static int open_serial_port(const char *port, unsigned int baudrate)
{
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        return -1;
    }

    if (set_serial_speed_linux(fd, baudrate) < 0) {
        close(fd);
        return -1;
    }

    pulse_dtr_rts(fd);
    usleep(1000000);
    drain_input(fd);

    return fd;
}

static int install_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        return -1;
    }

    return 0;
}

static void usage(const char *progname)
{
    fprintf(
        stderr,
        "Usage: %s [options] [serial_port [baudrate [mac_filter]]]\n"
        "mac_filter accepts a full MAC or a partial OUI/prefix, for example A4:C1:38 or A4C13812\n"
        "Defaults: serial_port=/dev/ttyUSB0 baudrate=2000000\n"
        "\n"
        "Scanning:\n"
        "  --no-scan                 do not start the BLE scan (use with the commands below)\n"
        "\n"
        "Generic:\n"
        "  --cmd HEX                 send a raw command, e.g. --cmd 060700 (repeatable)\n"
        "  --version                 read the HW/FW/SDK versions\n"
        "  --rfsdk                   read the RF/SDK status\n"
        "  --uart-status             read the UART status\n"
        "  --led OP:MASK:VALUE       board LED mask test (CMD_ID_LED)\n"
        "  --battery                 query the VBAT/3V3 rail and the chip temperature, then exit\n"
        "\n"
        "GPIO (CMD_ID_GPIO):\n"
        "  --gpio-read PIN           read a pin level\n"
        "  --gpio-write PIN:VAL      write a LED pin\n"
        "  --gpio-toggle PIN         toggle a LED pin\n"
        "  --gpio-analog PIN         12-bit ADC read (TB-03F-KIT: PB6 = 0x16)\n"
        "  --gpio-rgb R:G:B          set RGB1 (R = PC3, G = PC4, B = PC2)\n"
        "  --gpio-pwm PIN:DUTY       LED brightness through the hardware PWM\n"
        "  --gpio-pwm-off PIN        stop the PWM\n"
        "\n"
        "GPIO edge events (CMD_ID_GPIOEVT):\n"
        "  --gpioevt PIN             arm the both-edge interrupt on a pin and print the events\n"
        "  --gpioevt-off PIN         disarm a pin\n"
        "  --gpioevt-clear           disarm every pin\n"
        "  --gpioevt-query           print the armed-pin bitmap\n"
        "\n"
        "BLE central / GATT (CMD_ID_CONN, CMD_ID_TXDATA):\n"
        "  --txadv PHY:INT:HEX       transmit a custom advertisement\n"
        "                            (PHY 0 = legacy 1M, 1 = extended 1M, 2 = extended Coded)\n"
        "  --txadv-stop              stop the custom advertisement\n"
        "  --txadv-status            query the custom advertisement state\n"
        "  --conn MAC[:TYPE[:PHY]]   open a connection (TYPE 0 = public, 1 = random; PHY 0 = 1M, 1 = Coded)\n"
        "  --conn-close              disconnect / cancel a pending connection\n"
        "  --conn-status             query the connection state\n"
        "  --conn-discover           start the GATT service discovery\n"
        "  --conn-read HANDLE        read a characteristic\n"
        "  --conn-write H:HEX        write a characteristic with response\n"
        "  --txdata H:HEX            ATT Write Command (Write Without Response)\n"
        "\n"
        "  -h, --help                this help\n",
        progname
    );
}

int main(int argc, char **argv)
{
    const char *port = "/dev/ttyUSB0";
    const char *mac_filter = NULL;
    const char *positionals[3] = {NULL, NULL, NULL};
    unsigned int baudrate = 2000000U;
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    uint8_t white_list_cmd[7];
    size_t used = 0;
    size_t white_list_cmd_len = 0;
    size_t positional_count = 0;
    bool battery_query = false;
    bool no_scan = false;
    bool synced = false;
    int fd;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        uint8_t payload[MAX_ACTION_LEN];
        size_t payload_len = 0;
        long v1 = 0;
        long v2 = 0;
        const char *cursor = NULL;

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(arg, "--battery") == 0) {
            battery_query = true;
            continue;
        }
        if (strcmp(arg, "--no-scan") == 0) {
            no_scan = true;
            continue;
        }
        if (strcmp(arg, "--cmd") == 0) {
            if (i + 1 >= argc
                    || parse_hex_bytes(argv[i + 1], payload, sizeof(payload), &payload_len) < 0
                    || add_action(payload, payload_len) < 0) {
                fprintf(stderr, "Invalid --cmd payload\n");
                return 1;
            }
            ++i;
            continue;
        }
        if (strcmp(arg, "--version") == 0) {
            if (add_action(CMD_VERSION_REQ, sizeof(CMD_VERSION_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--rfsdk") == 0) {
            if (add_action(CMD_RFSDK_STATUS_REQ, sizeof(CMD_RFSDK_STATUS_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--uart-status") == 0) {
            if (add_action(CMD_UART_STATUS_REQ, sizeof(CMD_UART_STATUS_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--led") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0 || parse_next_int(&cursor, &v2) < 0) {
                fprintf(stderr, "Invalid --led OP:MASK:VALUE\n");
                return 1;
            }
            {
                long v3 = 0;
                if (parse_next_int(&cursor, &v3) < 0) {
                    v3 = 0;
                }
                payload[0] = CMD_ID_LED;
                payload[1] = (uint8_t)v1;
                payload[2] = (uint8_t)v2;
                payload[3] = (uint8_t)v3;
                if (add_action(payload, 4U) < 0) {
                    return 1;
                }
            }
            continue;
        }
        if (strcmp(arg, "--gpio-read") == 0 || strcmp(arg, "--gpio-toggle") == 0
                || strcmp(arg, "--gpio-analog") == 0 || strcmp(arg, "--gpio-pwm-off") == 0) {
            uint8_t op = (strcmp(arg, "--gpio-read") == 0) ? GPIO_OP_READ
                       : (strcmp(arg, "--gpio-toggle") == 0) ? GPIO_OP_TOGGLE
                       : (strcmp(arg, "--gpio-analog") == 0) ? GPIO_OP_ANALOG
                       : GPIO_OP_PWM_OFF;
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0) {
                fprintf(stderr, "Invalid pin for %s\n", arg);
                return 1;
            }
            payload[0] = CMD_ID_GPIO;
            payload[1] = op;
            payload[2] = (uint8_t)v1;
            if (add_action(payload, 3U) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--gpio-write") == 0 || strcmp(arg, "--gpio-pwm") == 0) {
            uint8_t op = (strcmp(arg, "--gpio-write") == 0) ? GPIO_OP_WRITE : GPIO_OP_PWM;
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0 || parse_next_int(&cursor, &v2) < 0) {
                fprintf(stderr, "Invalid PIN:VALUE for %s\n", arg);
                return 1;
            }
            payload[0] = CMD_ID_GPIO;
            payload[1] = op;
            payload[2] = (uint8_t)v1;
            payload[3] = (uint8_t)v2;
            if (op == GPIO_OP_WRITE) {
                if (add_action(payload, 4U) < 0) {
                    return 1;
                }
            } else {
                payload[4] = 0xE8U;     /* period 1000 us */
                payload[5] = 0x03U;
                if (add_action(payload, 6U) < 0) {
                    return 1;
                }
            }
            continue;
        }
        if (strcmp(arg, "--gpio-rgb") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0 || parse_next_int(&cursor, &v2) < 0) {
                fprintf(stderr, "Invalid R:G:B for --gpio-rgb\n");
                return 1;
            }
            {
                long b = 0;
                if (parse_next_int(&cursor, &b) < 0) {
                    b = 0;
                }
                payload[0] = CMD_ID_GPIO;
                payload[1] = GPIO_OP_RGB;
                payload[2] = PIN_RGB_GREEN;
                payload[3] = (uint8_t)v1;   /* R */
                payload[4] = (uint8_t)v2;   /* G */
                payload[5] = (uint8_t)b;    /* B */
                if (add_action(payload, 6U) < 0) {
                    return 1;
                }
            }
            continue;
        }
        if (strcmp(arg, "--gpioevt") == 0 || strcmp(arg, "--gpioevt-off") == 0) {
            uint8_t op = (strcmp(arg, "--gpioevt") == 0) ? GPIOEVT_OP_ENABLE : GPIOEVT_OP_DISABLE;
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0) {
                fprintf(stderr, "Invalid pin for %s\n", arg);
                return 1;
            }
            payload[0] = CMD_ID_GPIOEVT;
            payload[1] = op;
            payload[2] = (uint8_t)v1;
            g_last_gpioevt_op = op;
            if (add_action(payload, 3U) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--gpioevt-query") == 0) {
            g_last_gpioevt_op = GPIOEVT_OP_QUERY;
            if (add_action(CMD_GPIOEVT_QUERY_REQ, sizeof(CMD_GPIOEVT_QUERY_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--gpioevt-clear") == 0) {
            g_last_gpioevt_op = GPIOEVT_OP_CLEAR;
            if (add_action(CMD_GPIOEVT_CLEAR_REQ, sizeof(CMD_GPIOEVT_CLEAR_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--txadv") == 0) {
            size_t data_len = 0;
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0 || parse_next_int(&cursor, &v2) < 0) {
                fprintf(stderr, "Invalid PHY:INTERVAL:HEXDATA for --txadv\n");
                return 1;
            }
            if (parse_hex_bytes(cursor, payload, sizeof(payload), &data_len) < 0
                    || data_len > 31U) {
                fprintf(stderr, "Invalid advertisement data for --txadv\n");
                return 1;
            }
            payload[0] = CMD_ID_TXADV;
            payload[1] = 0x01U;
            payload[2] = (uint8_t)v1;           /* phy */
            payload[3] = (uint8_t)v2;           /* interval lo */
            payload[4] = (uint8_t)(v2 >> 8);    /* interval hi */
            payload[5] = (uint8_t)data_len;
            /* payload[6..] already holds the data parsed at the head of the buffer */
            memmove(&payload[6], payload, data_len);
            if (add_action(payload, 6U + data_len) < 0) {
                fprintf(stderr, "--txadv payload too long\n");
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--txadv-stop") == 0) {
            if (add_action(CMD_TXADV_STOP_REQ, sizeof(CMD_TXADV_STOP_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--txadv-status") == 0) {
            if (add_action(CMD_TXADV_STATUS_REQ, sizeof(CMD_TXADV_STATUS_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--conn") == 0) {
            uint8_t mac[6];
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            {
                /* the MAC comes first, then optional :TYPE and :PHY */
                char mac_text[20];
                size_t mac_len = 0;
                while (*cursor != '\0' && *cursor != ':' && mac_len < sizeof(mac_text) - 1U) {
                    mac_text[mac_len++] = *cursor++;
                }
                mac_text[mac_len] = '\0';
                if (parse_mac_le(mac_text, mac) < 0) {
                    fprintf(stderr, "Invalid MAC for --conn: %s\n", mac_text);
                    return 1;
                }
                v1 = 0;
                v2 = 0;
                if (*cursor == ':') {
                    ++cursor;
                    if (parse_next_int(&cursor, &v1) < 0) {
                        v1 = 0;
                    }
                    if (*cursor == ':') {
                        ++cursor;
                        if (parse_next_int(&cursor, &v2) < 0) {
                            v2 = 0;
                        }
                    }
                }
            }
            payload[0] = CMD_ID_CONN;
            payload[1] = (v2 != 0) ? 0x02U : 0x01U;     /* op 2 = Coded, op 1 = 1M */
            payload[2] = (uint8_t)v1;                   /* address type */
            memcpy(&payload[3], mac, 6);
            if (add_action(payload, 9U) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--conn-close") == 0) {
            if (add_action(CMD_CONN_CLOSE_REQ, sizeof(CMD_CONN_CLOSE_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--conn-status") == 0) {
            if (add_action(CMD_CONN_STATUS_REQ, sizeof(CMD_CONN_STATUS_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--conn-discover") == 0) {
            if (add_action(CMD_CONN_DISCOVER_REQ, sizeof(CMD_CONN_DISCOVER_REQ)) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--conn-read") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0) {
                fprintf(stderr, "Invalid handle for --conn-read\n");
                return 1;
            }
            payload[0] = CMD_ID_CONN;
            payload[1] = 0x06U;
            payload[2] = 0;
            payload[3] = (uint8_t)v1;
            payload[4] = (uint8_t)(v1 >> 8);
            if (add_action(payload, 5U) < 0) {
                return 1;
            }
            continue;
        }
        if (strcmp(arg, "--conn-write") == 0 || strcmp(arg, "--txdata") == 0) {
            size_t data_len = 0;
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            cursor = argv[++i];
            if (parse_next_int(&cursor, &v1) < 0) {
                fprintf(stderr, "Invalid HANDLE:HEXDATA for %s\n", arg);
                return 1;
            }
            if (*cursor == ':') {
                ++cursor;
            }
            if (parse_hex_bytes(cursor, payload, sizeof(payload), &data_len) < 0 || data_len > 20U) {
                fprintf(stderr, "Invalid data for %s\n", arg);
                return 1;
            }
            memmove(&payload[4], payload, data_len);
            payload[0] = (strcmp(arg, "--txdata") == 0) ? CMD_ID_TXDATA : CMD_ID_CONN;
            if (payload[0] == CMD_ID_CONN) {
                /* op 7 = write with response: [op, 0, handle_lo, handle_hi, len, data] */
                memmove(&payload[6], &payload[4], data_len);
                payload[1] = 0x07U;
                payload[2] = 0;
                payload[3] = (uint8_t)v1;
                payload[4] = (uint8_t)(v1 >> 8);
                payload[5] = (uint8_t)data_len;
                if (add_action(payload, 6U + data_len) < 0) {
                    return 1;
                }
            } else {
                /* [handle_lo, handle_hi, len, data] */
                payload[1] = (uint8_t)v1;
                payload[2] = (uint8_t)(v1 >> 8);
                payload[3] = (uint8_t)data_len;
                if (add_action(payload, 4U + data_len) < 0) {
                    return 1;
                }
            }
            continue;
        }
        if (positional_count >= sizeof(positionals) / sizeof(positionals[0])) {
            usage(argv[0]);
            return 1;
        }
        positionals[positional_count++] = argv[i];
    }

    if (battery_query && positional_count > 2U) {
        fprintf(stderr, "--battery does not accept mac_filter\n");
        return 1;
    }

    if (positional_count >= 1U) {
        port = positionals[0];
    }

    if (positional_count >= 2U) {
        baudrate = (unsigned int)strtoul(positionals[1], NULL, 10);
        if (baudrate == 0U) {
            fprintf(stderr, "Invalid baudrate: %s\n", positionals[1]);
            return 1;
        }
    }

    if (positional_count >= 3U) {
        mac_filter = positionals[2];
        if (build_white_list_command(mac_filter, white_list_cmd, &white_list_cmd_len) < 0) {
            fprintf(stderr, "Invalid MAC filter: %s\n", mac_filter);
            return 1;
        }
    }

    if (install_signal_handlers() < 0) {
        perror("sigaction");
        return 1;
    }

    fd = open_serial_port(port, baudrate);
    if (fd < 0) {
        perror(port);
        return 1;
    }

    g_battery_response_received = false;
    g_battery_status = 0xFFU;
    g_battery_mv = 0U;

    if (battery_query) {
        printf("Listening on %s at %u baud\n", port, baudrate);
        printf("Querying VBAT/3V3 rail and chip temperature; press Ctrl-C to stop\n");
        fflush(stdout);

        if (send_command_with_settle(fd, CMD_BATTERY, sizeof(CMD_BATTERY)) < 0) {
            perror("send VBAT");
            close(fd);
            return 1;
        }

        for (int i = 0; i < 20 && !g_stop_requested && !g_battery_response_received; ++i) {
            struct pollfd pfd = {
                .fd = fd,
                .events = POLLIN,
                .revents = 0,
            };

            int poll_rc = poll(&pfd, 1, 100);
            if (poll_rc < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("poll");
                close(fd);
                return 1;
            }
            if (poll_rc == 0) {
                continue;
            }
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                fprintf(stderr, "Serial line closed or in error state\n");
                close(fd);
                return 1;
            }
            if (pfd.revents & POLLIN) {
                ssize_t rd = read(fd, rx_buffer + used, sizeof(rx_buffer) - used);
                if (rd < 0) {
                    if (errno == EINTR || errno == EAGAIN) {
                        continue;
                    }
                    perror("read");
                    close(fd);
                    return 1;
                }
                if (rd == 0) {
                    continue;
                }
                used += (size_t)rd;
                process_rx_buffer(rx_buffer, &used, &synced);
            }
        }

        if (!g_battery_response_received) {
            fprintf(stderr, "VBAT query timed out\n");
            close(fd);
            return 1;
        }

        close(fd);
        return g_battery_status == 0x00U && g_battery_mv > 0U ? 0 : 1;
    }

    printf("Listening on %s at %u baud\n", port, baudrate);
    if (no_scan) {
        printf("Scan not started (--no-scan); press Ctrl-C to stop\n");
    } else {
        printf("Sending INFO, CLRM and START_SCAN (BLE 1M + Coded PHY); press Ctrl-C to stop\n");
    }
    if (mac_filter != NULL) {
        printf("Installing white-list MAC filter: %s\n", mac_filter);
    }
    if (g_action_count > 0) {
        printf("Sending %d command(s) requested on the command line\n", g_action_count);
    }
    fflush(stdout);

    if (send_command_with_settle(fd, CMD_INFO, sizeof(CMD_INFO)) < 0) {
        perror("send INFO");
        close(fd);
        return 1;
    }
    if (send_command_with_settle(fd, CMD_CLEAR_MAC_LIST, sizeof(CMD_CLEAR_MAC_LIST)) < 0) {
        perror("send CLRM");
        close(fd);
        return 1;
    }
    if (mac_filter != NULL && send_command_with_settle(fd, white_list_cmd, white_list_cmd_len) < 0) {
        perror("send WMAC");
        close(fd);
        return 1;
    }
    for (int i = 0; i < g_action_count; ++i) {
        if (send_command_with_settle(fd, g_actions[i], g_action_len[i]) < 0) {
            fprintf(stderr, "send command %d failed\n", i);
            close(fd);
            return 1;
        }
    }
    if (!no_scan && send_command_with_settle(fd, CMD_START_SCAN, sizeof(CMD_START_SCAN)) < 0) {
        perror("send START_SCAN");
        close(fd);
        return 1;
    }

    while (!g_stop_requested) {
        struct pollfd pfd = {
            .fd = fd,
            .events = POLLIN,
            .revents = 0,
        };

        int poll_rc = poll(&pfd, 1, 250);
        if (poll_rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            break;
        }

        if (poll_rc == 0) {
            continue;
        }

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "Serial line closed or in error state\n");
            break;
        }

        if (pfd.revents & POLLIN) {
            ssize_t rd = read(fd, rx_buffer + used, sizeof(rx_buffer) - used);
            if (rd < 0) {
                if (errno == EINTR || errno == EAGAIN) {
                    continue;
                }
                perror("read");
                break;
            }

            if (rd == 0) {
                continue;
            }

            used += (size_t)rd;
            process_rx_buffer(rx_buffer, &used, &synced);

            if (used == sizeof(rx_buffer)) {
                fprintf(stderr, "RX buffer full, discarding one byte to resync\n");
                memmove(rx_buffer, rx_buffer + 1, used - 1U);
                --used;
                synced = false;
            }
        }
    }

    if (!no_scan && send_command_with_settle(fd, CMD_STOP_SCAN, sizeof(CMD_STOP_SCAN)) < 0) {
        perror("send STOP_SCAN");
    } else if (!no_scan) {
        for (int i = 0; i < 4; ++i) {
            struct pollfd pfd = {
                .fd = fd,
                .events = POLLIN,
                .revents = 0,
            };

            int poll_rc = poll(&pfd, 1, 100);
            if (poll_rc <= 0) {
                break;
            }

            if (pfd.revents & POLLIN) {
                ssize_t rd = read(fd, rx_buffer + used, sizeof(rx_buffer) - used);
                if (rd > 0) {
                    used += (size_t)rd;
                    process_rx_buffer(rx_buffer, &used, &synced);
                }
            }
        }
    }

    close(fd);
    return 0;
}