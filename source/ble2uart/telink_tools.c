#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef B921600
#define B921600 B115200
#endif

#define TOOL_VERSION "1.0"

#define CMD_GET_VERSION 0x00
#define CMD_WRITE_FLASH 0x01
#define CMD_READ_FLASH  0x02
#define CMD_ERASE_FLASH 0x03
#define CMD_READ_MUID   0x04
#define CMD_CHANGE_BAUD 0x05

#define RES_WRITE_FLASH "OK_01"
#define RES_READ_FLASH  "OK_02"
#define RES_ERASE_FLASH "OK_03"
#define RES_READ_MUID   "OK_04"
#define RES_CHANGE_BAUD "OK_05"

#define CLR_RED   "\033[3;31m"
#define CLR_GREEN "\033[3;32m"
#define CLR_RESET "\033[0m"

typedef struct {
    const char *port;
    const char *command;
    int argc;
    char **argv;
} cli_args_t;

static void usage(const char *prog) {
    fprintf(stderr,
        "telink_tools v%s - Telink BLE Chip Bootloader Utility\n\n"
        "Usage:\n"
        "  %s [-p /dev/ttyUSB0] <command> [args]\n\n"
        "Commands:\n"
        "  burn <firmware.bin>\n"
        "  burn_triad <productID> <MAC_HEX> <SECRET_HEX>\n"
        "  write_flash <addr> <hexdata>\n"
        "  write_flash_fill <addr> <hexdata>\n"
        "  read_flash <addr> <len>\n"
        "  erase_flash <addr> <sector_count>\n"
        "  test\n\n"
        "Examples:\n"
        "  %s -p /dev/ttyACM0 burn firmware.bin\n"
        "  %s -p /dev/ttyUSB0 erase_flash 0x4000 44\n"
        "  %s -p /dev/ttyUSB0 read_flash 0x78000 26\n",
        TOOL_VERSION, prog, prog, prog, prog);
}

static bool parse_u32(const char *s, uint32_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, 0);
    if (errno || end == s || *end != '\0' || v > 0xFFFFFFFFul) {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

static speed_t baud_to_speed(int baud) {
    switch (baud) {
        case 115200: return B115200;
        case 921600: return B921600;
        default: return 0;
    }
}

static bool set_serial_baud(int fd, int baud) {
    struct termios tio;
    speed_t spd = baud_to_speed(baud);
    if (spd == 0) {
        return false;
    }
    if (tcgetattr(fd, &tio) < 0) {
        return false;
    }

    cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CRTSCTS;

    if (cfsetispeed(&tio, spd) < 0 || cfsetospeed(&tio, spd) < 0) {
        return false;
    }

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        return false;
    }
    tcflush(fd, TCIOFLUSH);
    return true;
}

static bool set_modem_line(int fd, int flag, bool on) {
    int status;
    if (ioctl(fd, TIOCMGET, &status) < 0) {
        return false;
    }
    if (on) {
        status |= flag;
    } else {
        status &= ~flag;
    }
    return ioctl(fd, TIOCMSET, &status) == 0;
}

static bool set_rts(int fd, bool on) {
    return set_modem_line(fd, TIOCM_RTS, on);
}

static bool set_dtr(int fd, bool on) {
    return set_modem_line(fd, TIOCM_DTR, on);
}

static ssize_t write_all(int fd, const uint8_t *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t w = write(fd, buf + off, len - off);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        off += (size_t)w;
    }
    return (ssize_t)off;
}

static bool uart_write(int fd, const uint8_t *data, size_t len) {
    tcflush(fd, TCIOFLUSH);
    return write_all(fd, data, len) == (ssize_t)len;
}

static size_t uart_read_text(int fd, char *out, size_t out_cap) {
    size_t used = 0;
    while (used + 1 < out_cap) {
        int avail = 0;
        if (ioctl(fd, FIONREAD, &avail) < 0 || avail <= 0) {
            break;
        }
        if (avail > (int)(out_cap - used - 1)) {
            avail = (int)(out_cap - used - 1);
        }
        ssize_t r = read(fd, out + used, (size_t)avail);
        if (r <= 0) {
            break;
        }
        used += (size_t)r;
    }
    out[used] = '\0';
    return used;
}

static bool wait_result(int fd, const char *needle, int timeout_ticks, bool print_out) {
    char result[1024];
    size_t result_len = 0;

    for (int wait_c = 0; wait_c <= timeout_ticks; ++wait_c) {
        if (result_len + 1 >= sizeof(result)) {
            break;
        }
        char chunk[256];
        size_t got = uart_read_text(fd, chunk, sizeof(chunk));
        if (got > 0) {
            size_t copy_len = got;
            if (copy_len > sizeof(result) - result_len - 1) {
                copy_len = sizeof(result) - result_len - 1;
            }
            memcpy(result + result_len, chunk, copy_len);
            result_len += copy_len;
            result[result_len] = '\0';
            if (result_len > 5) {
                break;
            }
        }
        usleep(10000);
    }

    if (strstr(result, needle) == NULL) {
        return false;
    }
    if (print_out) {
        while (result_len > 0 &&
               (result[result_len - 1] == '\n' || result[result_len - 1] == '\r')) {
            result[--result_len] = '\0';
        }
        printf("[%s] ", result);
        fflush(stdout);
    }
    return true;
}

static void be16(uint8_t *dst, uint16_t v) {
    dst[0] = (uint8_t)((v >> 8) & 0xFF);
    dst[1] = (uint8_t)(v & 0xFF);
}

static void be32(uint8_t *dst, uint32_t v) {
    dst[0] = (uint8_t)((v >> 24) & 0xFF);
    dst[1] = (uint8_t)((v >> 16) & 0xFF);
    dst[2] = (uint8_t)((v >> 8) & 0xFF);
    dst[3] = (uint8_t)(v & 0xFF);
}

static void le32(uint8_t *dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFF);
    dst[1] = (uint8_t)((v >> 8) & 0xFF);
    dst[2] = (uint8_t)((v >> 16) & 0xFF);
    dst[3] = (uint8_t)((v >> 24) & 0xFF);
}

static bool contains_bytes(const uint8_t *buf, size_t len, const char *needle) {
    size_t nlen = strlen(needle);
    if (nlen == 0 || len < nlen) {
        return false;
    }
    for (size_t i = 0; i + nlen <= len; ++i) {
        if (memcmp(buf + i, needle, nlen) == 0) {
            return true;
        }
    }
    return false;
}

static bool telink_flash_write(int fd, uint32_t addr, const uint8_t *data, size_t len) {
    if (len == 0 || len > 256) {
        return false;
    }

    uint8_t pkt[1 + 2 + 4 + 1 + 256];
    if (addr < 0x4000) {
        addr += 0x2C000;
    }

    pkt[0] = CMD_WRITE_FLASH;
    be16(pkt + 1, (uint16_t)(len + 5));
    be32(pkt + 3, addr);
    pkt[7] = 0;
    memcpy(pkt + 8, data, len);

    for (int retry = 0; retry < 3; ++retry) {
        if (!uart_write(fd, pkt, 8 + len)) {
            return false;
        }
        if (wait_result(fd, RES_WRITE_FLASH, 200, false)) {
            return true;
        }
        usleep(500000);
    }
    return false;
}

static bool telink_flash_read(int fd, uint32_t addr, uint8_t len_b, uint8_t *data_out) {
    uint8_t pkt[8];
    pkt[0] = CMD_READ_FLASH;
    be16(pkt + 1, 5);
    be32(pkt + 3, addr);
    pkt[7] = len_b;

    if (!uart_write(fd, pkt, sizeof(pkt))) {
        return false;
    }
    usleep(10000);

    uint8_t rx[2048];
    size_t used = 0;
    int idle_ticks = 0;

    while (used <= (size_t)len_b + 5 && idle_ticks < 400) {
        int avail = 0;
        if (ioctl(fd, FIONREAD, &avail) == 0 && avail > 0) {
            if (avail > (int)(sizeof(rx) - used)) {
                avail = (int)(sizeof(rx) - used);
            }
            ssize_t r = read(fd, rx + used, (size_t)avail);
            if (r > 0) {
                used += (size_t)r;
                idle_ticks = 0;
                continue;
            }
        }
        usleep(10000);
        ++idle_ticks;
    }

    if (used < (size_t)len_b + 5) {
        return false;
    }

    if (!contains_bytes(rx + len_b, used - len_b, RES_READ_FLASH)) {
        return false;
    }

    memcpy(data_out, rx, len_b);
    return true;
}

static bool telink_flash_erase(int fd, uint32_t addr, uint8_t len_t) {
    if (addr + (uint32_t)len_t * 0x1000u > 0x80000u) {
        return false;
    }

    uint8_t pkt[8];
    pkt[0] = CMD_ERASE_FLASH;
    be16(pkt + 1, 5);
    be32(pkt + 3, addr);
    pkt[7] = len_t;

    if (!uart_write(fd, pkt, sizeof(pkt))) {
        return false;
    }

    printf("\033[?25l-");
    fflush(stdout);
    for (int i = 0; i < (int)(len_t / 3); ++i) {
        usleep(100000);
        switch (i % 4) {
            case 1: printf("\b\\"); break;
            case 2: printf("\b|"); break;
            case 3: printf("\b/"); break;
            default: printf("\b-"); break;
        }
        fflush(stdout);
    }
    printf("\b \b\033[?25h");
    fflush(stdout);

    return wait_result(fd, RES_ERASE_FLASH, 200, false);
}

static bool connect_chip(int fd) {
    if (!set_rts(fd, true) || !set_dtr(fd, true)) {
        return false;
    }
    usleep(100000);
    if (!set_rts(fd, false)) {
        return false;
    }
    usleep(150000);
    if (!set_dtr(fd, false)) {
        return false;
    }

    uint8_t pkt[3];
    pkt[0] = CMD_GET_VERSION;
    be16(pkt + 1, 0);

    if (!uart_write(fd, pkt, sizeof(pkt))) {
        return false;
    }

    return wait_result(fd, "V", 200, true);
}

static bool change_baud(int fd) {
    uint8_t pkt[3];
    pkt[0] = CMD_CHANGE_BAUD;
    be16(pkt + 1, 0);

    if (!uart_write(fd, pkt, sizeof(pkt))) {
        return false;
    }

    if (!set_serial_baud(fd, 921600)) {
        return false;
    }
    usleep(10000);

    if (wait_result(fd, RES_CHANGE_BAUD, 50, false)) {
        printf("Try the Way2 to start download the  file  to the board ... %sSuccess!%s\n", CLR_GREEN, CLR_RESET);
        return true;
    }

    printf("Try to start download the file to the board ... %sSuccess!%s\n", CLR_GREEN, CLR_RESET);
    if (!set_serial_baud(fd, 115200)) {
        return false;
    }
    return connect_chip(fd);
}

static bool parse_hex_bytes(const char *hex, uint8_t *out, size_t out_cap, size_t *out_len) {
    size_t n = strlen(hex);
    if ((n % 2) != 0) {
        return false;
    }
    size_t bytes = n / 2;
    if (bytes > out_cap) {
        return false;
    }

    for (size_t i = 0; i < bytes; ++i) {
        char tmp[3] = { hex[i * 2], hex[i * 2 + 1], '\0' };
        char *end = NULL;
        long v = strtol(tmp, &end, 16);
        if (end == tmp || *end != '\0' || v < 0 || v > 255) {
            return false;
        }
        out[i] = (uint8_t)v;
    }

    *out_len = bytes;
    return true;
}

static int op_erase_flash(int fd, const char *addr_s, const char *len_s) {
    uint32_t addr;
    uint32_t len;

    if (!parse_u32(addr_s, &addr) || !parse_u32(len_s, &len) || len > 255) {
        fprintf(stderr, "%sInvalid erase arguments.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    printf("Erase Flash at %s %s Sector ... ... ", addr_s, len_s);
    fflush(stdout);

    if (telink_flash_erase(fd, addr, (uint8_t)len)) {
        printf("%sOK!%s\n", CLR_GREEN, CLR_RESET);
        return 0;
    }

    printf("%sFail!%s\n", CLR_RED, CLR_RESET);
    return 1;
}

static int op_read_flash(int fd, const char *addr_s, const char *len_s) {
    uint32_t addr;
    uint32_t len;
    uint8_t data[255];

    if (!parse_u32(addr_s, &addr) || !parse_u32(len_s, &len)) {
        fprintf(stderr, "%sInvalid read arguments.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }
    if (len > 255) {
        fprintf(stderr, "%sThe MAX read len is 255 bytes!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    printf("Read Flash from %s %s Bytes ... ... ", addr_s, len_s);
    fflush(stdout);

    if (!telink_flash_read(fd, addr, (uint8_t)len, data)) {
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    printf("%sOK!%s\n", CLR_GREEN, CLR_RESET);
    for (uint32_t i = 0; i < len; ++i) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if ((len % 16) != 0) {
        printf("\n");
    }
    return 0;
}

static int op_write_flash_common(int fd, const char *addr_s, const char *hex_s, bool fill) {
    uint32_t addr;
    uint8_t data[256];
    size_t len = 0;

    if (!parse_u32(addr_s, &addr)) {
        fprintf(stderr, "%sInvalid address.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    if (!parse_hex_bytes(hex_s, data, sizeof(data), &len)) {
        fprintf(stderr, "%sInvalid HEX payload.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    if (len > 255) {
        fprintf(stderr, "%sThe MAX write len is 255 bytes!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    if (fill) {
        memset(data + len, 0xFF, 256 - len);
        len = 256;
    }

    if (!telink_flash_write(fd, addr, data, len)) {
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    printf("%sSuccess!%s\n", CLR_GREEN, CLR_RESET);
    return 0;
}

static int op_burn(int fd, const char *filename) {
    struct stat st;
    if (stat(filename, &st) < 0) {
        perror("stat");
        return 1;
    }

    if (st.st_size <= 0) {
        fprintf(stderr, "%sFirmware file is empty.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    if (st.st_size > 0x2c000) {
        fprintf(stderr, "%sFirmware Too BIG!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    (void)change_baud(fd);
    printf("Start erase Flash at 0x4000 len 176 KB ... ");
    fflush(stdout);

    if (!telink_flash_erase(fd, 0x4000, 44)) {
        fclose(fp);
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    printf("%sOK!%s\nBurn Firmware: %s\n", CLR_GREEN, CLR_RESET, filename);

    uint8_t chunk[256];
    uint32_t addr = 0;
    const int bar_len = 50;

    while (1) {
        size_t r = fread(chunk, 1, sizeof(chunk), fp);
        if (r == 0) {
            break;
        }

        if (!telink_flash_write(fd, addr, chunk, r)) {
            fclose(fp);
            printf("\n%sBurn firmware Fail!%s\n", CLR_RED, CLR_RESET);
            return 1;
        }

        addr += (uint32_t)r;
        int percent = (int)((addr * 100u) / (uint32_t)st.st_size);
        int done = (percent * bar_len) / 100;

        printf("\r%d%% [%s", percent, CLR_GREEN);
        for (int i = 0; i < done; ++i) {
            putchar('>');
        }
        printf("%s", CLR_RESET);
        for (int i = done; i < bar_len; ++i) {
            putchar('=');
        }
        putchar(']');
        fflush(stdout);
    }

    printf("\n");
    fclose(fp);
    return 0;
}

static int op_burn_triad(int fd, const char *product_id_s, const char *mac_hex, const char *secret_hex) {
    uint32_t product_id;
    uint8_t mac[16], secret[32];
    size_t mac_len = 0, secret_len = 0;

    if (!parse_u32(product_id_s, &product_id)) {
        fprintf(stderr, "%sInvalid productID.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    if (!parse_hex_bytes(mac_hex, mac, sizeof(mac), &mac_len)) {
        fprintf(stderr, "%sInvalid MAC hex.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    if (!parse_hex_bytes(secret_hex, secret, sizeof(secret), &secret_len)) {
        fprintf(stderr, "%sInvalid Secret hex.%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    uint8_t triad[26];
    le32(triad, product_id);
    if (4 + mac_len + secret_len != sizeof(triad)) {
        fprintf(stderr, "%sTriad Error!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }
    memcpy(triad + 4, mac, mac_len);
    memcpy(triad + 4 + mac_len, secret, secret_len);

    printf("Your productID =  %s\n", product_id_s);
    printf("Your MAC =   %s\n", mac_hex);
    printf("Your Secret =   %s\n", secret_hex);

    printf("Erase Flash at 0x78000 len 4 KB ... ... ");
    fflush(stdout);
    if (!telink_flash_erase(fd, 0x78000, 1)) {
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }
    printf("%sOK!%s\n", CLR_GREEN, CLR_RESET);

    printf("Burn Triad to 0x78000 ... ... ");
    fflush(stdout);
    if (!telink_flash_write(fd, 0x78000, triad, sizeof(triad))) {
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }
    printf("%sOK!%s\n", CLR_GREEN, CLR_RESET);
    return 0;
}

static int op_test(int fd) {
    while (1) {
        if (!set_dtr(fd, false) || !set_dtr(fd, true)) {
            fprintf(stderr, "%sFailed to toggle DTR.%s\n", CLR_RED, CLR_RESET);
            return 1;
        }
        usleep(100000);
    }
}

static int parse_cli(int argc, char **argv, cli_args_t *out) {
    out->port = "/dev/ttyUSB0";
    out->command = NULL;
    out->argc = 0;
    out->argv = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
            case 'p':
                out->port = optarg;
                break;
            case 'h':
                usage(argv[0]);
                return 1;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    if (optind >= argc) {
        usage(argv[0]);
        return -1;
    }

    out->command = argv[optind++];
    out->argc = argc - optind;
    out->argv = &argv[optind];
    return 0;
}

static int dispatch_command(int fd, const cli_args_t *cli) {
    if (strcmp(cli->command, "burn") == 0) {
        if (cli->argc != 1) {
            fprintf(stderr, "burn requires: <firmware.bin>\n");
            return 1;
        }
        return op_burn(fd, cli->argv[0]);
    }

    if (strcmp(cli->command, "burn_triad") == 0) {
        if (cli->argc != 3) {
            fprintf(stderr, "burn_triad requires: <productID> <MAC_HEX> <SECRET_HEX>\n");
            return 1;
        }
        return op_burn_triad(fd, cli->argv[0], cli->argv[1], cli->argv[2]);
    }

    if (strcmp(cli->command, "write_flash") == 0) {
        if (cli->argc != 2) {
            fprintf(stderr, "write_flash requires: <addr> <hexdata>\n");
            return 1;
        }
        return op_write_flash_common(fd, cli->argv[0], cli->argv[1], false);
    }

    if (strcmp(cli->command, "write_flash_fill") == 0) {
        if (cli->argc != 2) {
            fprintf(stderr, "write_flash_fill requires: <addr> <hexdata>\n");
            return 1;
        }
        return op_write_flash_common(fd, cli->argv[0], cli->argv[1], true);
    }

    if (strcmp(cli->command, "read_flash") == 0) {
        if (cli->argc != 2) {
            fprintf(stderr, "read_flash requires: <addr> <len>\n");
            return 1;
        }
        return op_read_flash(fd, cli->argv[0], cli->argv[1]);
    }

    if (strcmp(cli->command, "erase_flash") == 0) {
        if (cli->argc != 2) {
            fprintf(stderr, "erase_flash requires: <addr> <sector_count>\n");
            return 1;
        }
        return op_erase_flash(fd, cli->argv[0], cli->argv[1]);
    }

    if (strcmp(cli->command, "test") == 0) {
        return op_test(fd);
    }

    fprintf(stderr, "%sUnknown command: %s%s\n", CLR_RED, cli->command, CLR_RESET);
    return 1;
}

int main(int argc, char **argv) {
    cli_args_t cli;
    int parse_rc;

    printf("telink_tools v%s\n", TOOL_VERSION);

    parse_rc = parse_cli(argc, argv, &cli);
    if (parse_rc != 0) {
        return parse_rc > 0 ? 0 : 1;
    }

    printf("Open %s ... ... ", cli.port);
    fflush(stdout);

    int fd = open(cli.port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        perror("open");
        return 1;
    }

    if (!set_serial_baud(fd, 115200)) {
        close(fd);
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        fprintf(stderr, "Could not configure serial port.\n");
        return 1;
    }

    printf("%sSuccess!%s\r\nConnect Board ... ...", CLR_GREEN, CLR_RESET);
    fflush(stdout);

    if (!connect_chip(fd)) {
        close(fd);
        printf("%sFail!%s\n", CLR_RED, CLR_RESET);
        return 1;
    }

    printf("%sSuccess!%s\n", CLR_GREEN, CLR_RESET);

    int rc = dispatch_command(fd, &cli);
    close(fd);
    return rc;
}
