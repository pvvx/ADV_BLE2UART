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

#define CMD_ID_SCAN 0x01U
#define CMD_ID_VBAT 0x0FU
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
        g_battery_response_received = true;
        g_battery_status = id;
        g_battery_mv = batt_mv;
        printf(
            "VBAT status=%s(0x%02X) mv=%u\n",
            cmd_status_name(id),
            (unsigned int)id,
            batt_mv
        );
        break;
    }
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
        "Usage: %s [--battery] [serial_port [baudrate [mac_filter]]]\n"
        "mac_filter accepts a full MAC or a partial OUI/prefix, for example A4:C1:38 or A4C13812\n"
        "--battery queries the current VBAT/3V3 rail in millivolts and exits (mac_filter is not used)\n"
        "Defaults: serial_port=/dev/ttyUSB0 baudrate=2000000\n",
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
    bool synced = false;
    int fd;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--battery") == 0) {
            battery_query = true;
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
        printf("Querying VBAT/3V3 rail; press Ctrl-C to stop\n");
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
    printf("Sending INFO, CLRM and START_SCAN (BLE 1M + Coded PHY); press Ctrl-C to stop\n");
    if (mac_filter != NULL) {
        printf("Installing white-list MAC filter: %s\n", mac_filter);
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
    if (send_command_with_settle(fd, CMD_START_SCAN, sizeof(CMD_START_SCAN)) < 0) {
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

    if (send_command_with_settle(fd, CMD_STOP_SCAN, sizeof(CMD_STOP_SCAN)) < 0) {
        perror("send STOP_SCAN");
    } else {
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