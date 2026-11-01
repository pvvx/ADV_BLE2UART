/*
 * esp-flasher -- flash an ESP32 family SoC from Linux.
 *
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 * Why this wrapper exists: esp-serial-flasher (v2.x) only knows the classic
 * auto-reset circuit -- DTR->BOOT and RTS->RESET through two transistors, as on
 * boards with a CP2102/CH340/FT232 bridge. Boards whose ESP32-C3/C6/H2/S3 is
 * wired through its *built-in* USB Serial/JTAG peripheral (/dev/ttyACM*) have no
 * such circuit: the ROM reads the DTR/RTS bits of the USB CDC interface and
 * needs the dedicated sequence esptool calls USBJTAGSerialReset. Without it the
 * chip never enters download mode on its own and has to be put in boot mode by
 * hand (BOOT+RESET), and after the flash it stays in download mode until the
 * RESET button is pressed again.
 *
 * The sequences below replicate esptool's "before" (ClassicReset /
 * USBJTAGSerialReset) and "after" (HardReset) strategies and are driven here,
 * on our own tty file descriptor, so the tool works with any esp-serial-flasher
 * version. The library is asked not to touch the lines (-m auto ->
 * LINUX_GPIO_NONE) which also avoids a second, wrong pulse in the middle of the
 * handshake.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "esp_loader.h"
#include "example_common.h"
#include "linux_port.h"
#include "md5_hash.h"

#define DEFAULT_SERIAL_DEVICE "/dev/ttyACM0"
#define DEFAULT_BAUD_RATE 115200
#define HIGHER_BAUD_RATE 460800

#define ESP32XX_UART_CLKDIV_REG 0x60000014
#define ESP32XX_UART_CLKDIV_MASK 0x000FFFFF

#define ESP32XX_EFUSE_BASE 0x60008800
#define ESP32C3_EFUSE_BLOCK1_ADDR (ESP32XX_EFUSE_BASE + 0x44)
#define ESP32C2_EFUSE_BLOCK2_ADDR (ESP32XX_EFUSE_BASE + 0x40)

/* esptool timings for the reset sequences. */
#define RESET_HOLD_MS 100
#define BOOT_HOLD_MS 100
/* A USB JTAG Serial board re-enumerates after a reset: give it time to come
 * back and to settle before the library opens the port. */
#define REENUMERATION_TIMEOUT_MS 8000
#define REENUMERATION_POLL_MS 50
#define REENUMERATION_SETTLE_MS 300

/* ------------------------------------------------------------------------- */
/* Reset strategies                                                          */
/* ------------------------------------------------------------------------- */

/* Defined below, next to the connection retry helpers. */
static double now_seconds(void);

typedef enum {
    BEFORE_DEFAULT,   /* auto: USBJTAGSerialReset on ttyACM, ClassicReset otherwise */
    BEFORE_USB_JTAG,  /* force esptool USBJTAGSerialReset */
    BEFORE_CLASSIC,   /* force esptool ClassicReset (external auto-reset circuit) */
    BEFORE_NONE,      /* the target is already in download mode */
} before_strategy_t;

typedef enum {
    AFTER_HARD_RESET, /* esptool HardReset: restart into the new application */
    AFTER_NONE,       /* leave the target in download mode */
} after_strategy_t;

static bool read_sysfs_hex(const char *path, unsigned int *out)
{
    FILE *f = fopen(path, "r");
    int rc;

    if (!f) {
        return false;
    }
    rc = fscanf(f, "%x", out);
    fclose(f);
    return rc == 1;
}

/*
 * True when the serial device is an Espressif USB JTAG Serial interface
 * (VID 0x303A, PID 0x1001), detected through sysfs. Anything that cannot be
 * classified falls back to the /dev/ttyACM* heuristic used by earlier versions.
 */
static bool is_usb_jtag_serial_device(const char *device)
{
    const char *name = strrchr(device, '/');
    char path[256];
    unsigned int vid = 0;
    unsigned int pid = 0;

    name = name ? name + 1 : device;

    snprintf(path, sizeof(path), "/sys/class/tty/%s/device/../idVendor", name);
    if (read_sysfs_hex(path, &vid)) {
        snprintf(path, sizeof(path), "/sys/class/tty/%s/device/../idProduct", name);
        if (read_sysfs_hex(path, &pid)) {
            return vid == 0x303A && pid == 0x1001;
        }
    }
    return strncmp(device, "/dev/ttyACM", strlen("/dev/ttyACM")) == 0;
}

static void sleep_ms(uint32_t ms)
{
    usleep((useconds_t)ms * 1000u);
}

/* Open the tty raw and without line discipline side effects. The kernel may
 * assert DTR/RTS when the port is opened; every sequence below starts with an
 * explicit (0,0) so the state is known. */
static int tty_open(const char *device)
{
    struct termios options;
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0) {
        return -1;
    }
    if (tcgetattr(fd, &options) == 0) {
        cfmakeraw(&options);
        options.c_cflag |= (CLOCAL | CREAD);
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 0;
        tcsetattr(fd, TCSANOW, &options);
    }
    return fd;
}

/* Both modem-control lines are written with a single TIOCMSET: toggling them
 * with two separate ioctl() calls can produce a glitch that makes the ROM miss
 * the download-mode request. */
static void tty_set_lines(int fd, bool dtr, bool rts)
{
    int status;

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

/*
 * esptool "USBJTAGSerialReset" (hold_boot = true) / "HardReset" (false) for a
 * target connected through its built-in USB Serial/JTAG: the ROM watches the
 * CDC control bits, so the order of the transitions is what selects download
 * mode versus a normal restart.
 */
static bool tty_reset_usb_jtag(const char *device, bool hold_boot)
{
    int fd = tty_open(device);

    if (fd < 0) {
        fprintf(stderr, "Cannot open %s to reset the target: %s\n", device, strerror(errno));
        return false;
    }

    tty_set_lines(fd, false, false);            /* idle */
    if (hold_boot) {
        tty_set_lines(fd, true, false);         /* BOOT asserted */
        sleep_ms(RESET_HOLD_MS);
        tty_set_lines(fd, true, true);          /* RESET asserted, BOOT still low */
        tty_set_lines(fd, false, true);         /* BOOT released through (1,1) */
        sleep_ms(RESET_HOLD_MS);
        tty_set_lines(fd, false, false);        /* RESET released -> ROM loader */
    } else {
        tty_set_lines(fd, false, true);         /* RESET asserted, BOOT high */
        sleep_ms(RESET_HOLD_MS);
        tty_set_lines(fd, false, false);        /* RESET released -> application */
    }
    close(fd);
    return true;
}

/*
 * esptool "UnixTightReset" (hold_boot = true) / "HardReset" (false) for boards
 * with an external USB-to-UART bridge and the two-transistor auto-reset
 * circuit.
 */
static bool tty_reset_classic(const char *device, bool hold_boot)
{
    int fd = tty_open(device);

    if (fd < 0) {
        fprintf(stderr, "Cannot open %s to reset the target: %s\n", device, strerror(errno));
        return false;
    }

    tty_set_lines(fd, false, false);
    if (hold_boot) {
        tty_set_lines(fd, true, true);          /* pass through (1,1), never (0,0) */
        tty_set_lines(fd, false, true);         /* RESET low, BOOT high */
        sleep_ms(RESET_HOLD_MS);
        tty_set_lines(fd, true, false);         /* RESET high with BOOT low */
        sleep_ms(BOOT_HOLD_MS);
        tty_set_lines(fd, false, false);        /* BOOT high -> ROM loader */
    } else {
        tty_set_lines(fd, false, true);         /* RESET low */
        sleep_ms(RESET_HOLD_MS);
        tty_set_lines(fd, false, false);        /* RESET high -> application */
    }
    close(fd);
    return true;
}

/*
 * After a reset of a USB JTAG Serial target the /dev/ttyACM* node disappears
 * and comes back. Wait for it without opening it (opening would assert DTR/RTS
 * again), then let it settle.
 */
static bool wait_for_reenumeration(const char *device, uint32_t timeout_ms)
{
    double deadline = now_seconds() + (double)timeout_ms / 1000.0;
    struct stat info;

    while (now_seconds() < deadline) {
        if (stat(device, &info) == 0) {
            sleep_ms(REENUMERATION_SETTLE_MS);
            return true;
        }
        sleep_ms(REENUMERATION_POLL_MS);
    }
    fprintf(stderr, "Warning: %s did not come back within %u ms\n", device, timeout_ms);
    return false;
}

/* Put the target in download mode before the library opens the port. */
static bool reset_into_download_mode(const char *device, before_strategy_t strategy)
{
    bool usb_jtag;

    if (strategy == BEFORE_NONE) {
        return true;
    }
    usb_jtag = strategy == BEFORE_USB_JTAG ||
               (strategy == BEFORE_DEFAULT && is_usb_jtag_serial_device(device));

    printf("Reset         : %s\n", usb_jtag ? "usb-jtag (esptool USBJTAGSerialReset)"
                                           : "classic (esptool UnixTightReset)");
    if (!(usb_jtag ? tty_reset_usb_jtag(device, true)
                   : tty_reset_classic(device, true))) {
        return false;
    }
    if (usb_jtag && !wait_for_reenumeration(device, REENUMERATION_TIMEOUT_MS)) {
        return false;
    }
    return true;
}

/* Restart the target into the freshly written application. */
static void reset_into_application(const char *device, after_strategy_t strategy)
{
    bool usb_jtag;

    if (strategy == AFTER_NONE) {
        printf("Target left in download mode (--after no-reset)\n");
        return;
    }
    usb_jtag = is_usb_jtag_serial_device(device);
    printf("Restarting the target (%s reset)\n", usb_jtag ? "usb-jtag" : "classic");
    if (usb_jtag ? tty_reset_usb_jtag(device, false)
                 : tty_reset_classic(device, false)) {
        if (usb_jtag) {
            wait_for_reenumeration(device, REENUMERATION_TIMEOUT_MS);
        }
        printf("Target running the new firmware\n");
    } else {
        fprintf(stderr, "Warning: could not reset the target; press RESET or replug it\n");
    }
}

static const char *loader_err_to_string(esp_loader_error_t err)
{
    switch (err) {
    case ESP_LOADER_SUCCESS: return "NONE";
    case ESP_LOADER_ERROR_FAIL: return "UNKNOWN";
    case ESP_LOADER_ERROR_TIMEOUT: return "TIMEOUT";
    case ESP_LOADER_ERROR_IMAGE_SIZE: return "IMAGE SIZE";
    case ESP_LOADER_ERROR_INVALID_MD5: return "INVALID MD5";
    case ESP_LOADER_ERROR_INVALID_PARAM: return "INVALID PARAMETER";
    case ESP_LOADER_ERROR_INVALID_TARGET: return "INVALID TARGET";
    case ESP_LOADER_ERROR_UNSUPPORTED_CHIP: return "UNSUPPORTED CHIP";
    case ESP_LOADER_ERROR_UNSUPPORTED_FUNC: return "UNSUPPORTED FUNCTION";
    case ESP_LOADER_ERROR_INVALID_RESPONSE: return "INVALID RESPONSE";
    default: return "UNKNOWN";
    }
}

static bool is_retriable_connect_err(esp_loader_error_t err)
{
    return err == ESP_LOADER_ERROR_TIMEOUT ||
           err == ESP_LOADER_ERROR_INVALID_TARGET ||
           err == ESP_LOADER_ERROR_INVALID_RESPONSE;
}

static const char *on_off(bool v)
{
    return v ? "ON" : "OFF";
}

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

static esp_loader_error_t connect_with_retries(esp_loader_t *loader,
        uint32_t current_baud,
    bool *use_stub_inout,
    uint32_t *effective_baud_out)
{
    int attempt;
    esp_loader_error_t err = ESP_LOADER_ERROR_FAIL;

    for (attempt = 1; attempt <= 3; ++attempt) {
        esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();

        if (attempt > 1) {
            connect_config.sync_timeout = 300;
            connect_config.trials = 30;
            usleep(200000);
        }

        err = *use_stub_inout
              ? esp_loader_connect_with_stub(loader, &connect_config)
              : esp_loader_connect(loader, &connect_config);
        if (err == ESP_LOADER_SUCCESS) {
            printf("Connected to target\n");
            break;
        }

        printf("Cannot connect to target. Error: %s\n", loader_err_to_string(err));

        if (!is_retriable_connect_err(err) || attempt == 3) {
            if (*use_stub_inout && is_retriable_connect_err(err)) {
                esp_loader_connect_args_t fallback_cfg = ESP_LOADER_CONNECT_DEFAULT();
                fallback_cfg.sync_timeout = 400;
                fallback_cfg.trials = 40;
                printf("Retrying in ROM bootloader mode...\n");
                err = esp_loader_connect(loader, &fallback_cfg);
                if (err == ESP_LOADER_SUCCESS) {
                    *use_stub_inout = false;
                    printf("Connected to target (ROM bootloader mode)\n");
                    return ESP_LOADER_SUCCESS;
                }
                printf("Cannot connect to target. Error: %s\n", loader_err_to_string(err));
            }
            if (err == ESP_LOADER_ERROR_TIMEOUT) {
                printf("Check if the host and the target are properly connected.\n");
            } else if (err == ESP_LOADER_ERROR_INVALID_TARGET) {
                printf("You could be using an unsupported chip, or chip revision.\n");
            } else if (err == ESP_LOADER_ERROR_INVALID_RESPONSE) {
                printf("Stub mode can fail on USB JTAG Serial; retrying in ROM mode is recommended.\n");
            }
            return err;
        }

        printf("Retrying connection (%d/3)...\n", attempt + 1);
    }

    if (err != ESP_LOADER_SUCCESS) {
        return err;
    }

    if (effective_baud_out) {
        *effective_baud_out = current_baud;
    }

    if (*use_stub_inout) {
        printf("Changing baud rate to %u...\n", (unsigned)HIGHER_BAUD_RATE);
        err = esp_loader_change_transmission_rate_stub(loader, current_baud, HIGHER_BAUD_RATE);
        if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
            printf("Interface does not support changing transmission rate via stub.\n");
        } else if (err != ESP_LOADER_SUCCESS) {
            printf("Unable to change transmission rate via stub. Error: %s\n", loader_err_to_string(err));
            if (is_retriable_connect_err(err)) {
                esp_loader_connect_args_t fallback_cfg = ESP_LOADER_CONNECT_DEFAULT();
                fallback_cfg.sync_timeout = 300;
                fallback_cfg.trials = 30;
                printf("Retrying in ROM bootloader mode...\n");
                err = esp_loader_connect(loader, &fallback_cfg);
                if (err == ESP_LOADER_SUCCESS) {
                    *use_stub_inout = false;
                    printf("Connected to target (ROM bootloader mode)\n");
                    return ESP_LOADER_SUCCESS;
                }
            }
            return err;
        }
        printf("Changed.\n");
        if (effective_baud_out) {
            *effective_baud_out = HIGHER_BAUD_RATE;
        }
    } else {
        printf("Changing baud rate to %u...\n", (unsigned)HIGHER_BAUD_RATE);
        err = esp_loader_change_transmission_rate(loader, HIGHER_BAUD_RATE);
        if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
            printf("Interface does not support changing transmission rate.\n");
        } else if (err != ESP_LOADER_SUCCESS) {
            printf("Unable to change transmission rate. Error: %s\n", loader_err_to_string(err));
            return err;
        } else {
            printf("Changed.\n");
            if (effective_baud_out) {
                *effective_baud_out = HIGHER_BAUD_RATE;
            }
        }
    }

    return ESP_LOADER_SUCCESS;
}

static const char *chip_to_string(target_chip_t chip)
{
    switch (chip) {
    case ESP8266_CHIP: return "esp8266";
    case ESP32_CHIP: return "esp32";
    case ESP32S2_CHIP: return "esp32s2";
    case ESP32C3_CHIP: return "esp32c3";
    case ESP32S3_CHIP: return "esp32s3";
    case ESP32C2_CHIP: return "esp32c2";
    case ESP32C5_CHIP: return "esp32c5";
    case ESP32H2_CHIP: return "esp32h2";
    case ESP32C6_CHIP: return "esp32c6";
    case ESP32P4_CHIP: return "esp32p4";
    case ESP32C61_CHIP: return "esp32c61";
    default: return "unknown";
    }
}

static bool parse_chip_name(const char *name, target_chip_t *out)
{
    if (strcmp(name, "esp8266") == 0) {
        *out = ESP8266_CHIP;
    } else if (strcmp(name, "esp32") == 0) {
        *out = ESP32_CHIP;
    } else if (strcmp(name, "esp32s2") == 0) {
        *out = ESP32S2_CHIP;
    } else if (strcmp(name, "esp32c3") == 0) {
        *out = ESP32C3_CHIP;
    } else if (strcmp(name, "esp32s3") == 0) {
        *out = ESP32S3_CHIP;
    } else if (strcmp(name, "esp32c2") == 0) {
        *out = ESP32C2_CHIP;
    } else if (strcmp(name, "esp32c5") == 0) {
        *out = ESP32C5_CHIP;
    } else if (strcmp(name, "esp32h2") == 0) {
        *out = ESP32H2_CHIP;
    } else if (strcmp(name, "esp32c6") == 0) {
        *out = ESP32C6_CHIP;
    } else if (strcmp(name, "esp32p4") == 0) {
        *out = ESP32P4_CHIP;
    } else if (strcmp(name, "esp32c61") == 0) {
        *out = ESP32C61_CHIP;
    } else {
        return false;
    }

    return true;
}

static esp_loader_error_t get_crystal_frequency_esp32c2(esp_loader_t *loader, uint32_t *frequency)
{
    uint32_t uart_div;
    double est_xtal_mhz;
    linux_port_t *port;
    esp_loader_error_t err;

    if (!loader || !frequency || !loader->_port) {
        return ESP_LOADER_ERROR_INVALID_PARAM;
    }

    err = esp_loader_read_register(loader, ESP32XX_UART_CLKDIV_REG, &uart_div);
    if (err != ESP_LOADER_SUCCESS) {
        return err;
    }

    uart_div &= ESP32XX_UART_CLKDIV_MASK;
    if (uart_div == 0) {
        return ESP_LOADER_ERROR_INVALID_RESPONSE;
    }

    port = container_of(loader->_port, linux_port_t, port);
    if (!port || port->baudrate == 0) {
        return ESP_LOADER_ERROR_INVALID_PARAM;
    }

    est_xtal_mhz = ((double)port->baudrate * (double)uart_div) / 1000000.0;
    if (est_xtal_mhz > 45.0) {
        *frequency = 48;
    } else if (est_xtal_mhz > 33.0) {
        *frequency = 40;
    } else {
        *frequency = 26;
    }

    return ESP_LOADER_SUCCESS;
}

static bool get_fixed_crystal_frequency(target_chip_t chip, uint32_t *frequency)
{
    if (!frequency) {
        return false;
    }

    switch (chip) {
    case ESP32S2_CHIP:
    case ESP32C3_CHIP:
    case ESP32S3_CHIP:
    case ESP32C6_CHIP:
    case ESP32P4_CHIP:
    case ESP32C61_CHIP:
        *frequency = 40;
        return true;
    case ESP32H2_CHIP:
        *frequency = 32;
        return true;
    default:
        return false;
    }
}

static const char *chip_features(target_chip_t chip)
{
    switch (chip) {
    case ESP32C3_CHIP: return "Wi-Fi, BT 5 (LE), Single Core";
    case ESP32C2_CHIP: return "Wi-Fi, BT 5 (LE), Single Core";
    case ESP32S3_CHIP: return "Wi-Fi, BT 5 (LE), Dual Core";
    case ESP32C6_CHIP: return "Wi-Fi 6, BT 5 (LE), IEEE802.15.4";
    case ESP32C61_CHIP: return "Wi-Fi 6, BT 5 (LE), IEEE802.15.4";
    case ESP32H2_CHIP: return "BT 5 (LE), IEEE802.15.4";
    case ESP32P4_CHIP: return "Dual Core + LP Core";
    case ESP32_CHIP: return "Wi-Fi, BT";
    case ESP32S2_CHIP: return "Wi-Fi";
    case ESP8266_CHIP: return "Wi-Fi";
    default: return "n/a";
    }
}

static const char *flash_vendor_from_id(uint32_t vendor_id)
{
    switch (vendor_id) {
    case 1: return "XMC";
    case 2: return "GD";
    case 3: return "FM";
    case 4: return "TT";
    case 5: return "ZBIT";
    default: return "Unknown";
    }
}

static bool get_embedded_flash_info(esp_loader_t *loader,
        target_chip_t chip,
        uint32_t *cap_code,
        const char **vendor)
{
    uint32_t reg = 0;
    uint32_t vendor_id = 0;

    if (!loader || !cap_code || !vendor) {
        return false;
    }

    if (chip == ESP32C3_CHIP) {
        if (esp_loader_read_register(loader, ESP32C3_EFUSE_BLOCK1_ADDR + (4 * 3), &reg) != ESP_LOADER_SUCCESS) {
            return false;
        }
        *cap_code = (reg >> 27) & 0x07;

        if (esp_loader_read_register(loader, ESP32C3_EFUSE_BLOCK1_ADDR + (4 * 4), &reg) != ESP_LOADER_SUCCESS) {
            return false;
        }
        vendor_id = reg & 0x07;
        *vendor = flash_vendor_from_id(vendor_id);
        return true;
    }

    if (chip == ESP32C2_CHIP) {
        if (esp_loader_read_register(loader, ESP32C2_EFUSE_BLOCK2_ADDR + (4 * 7), &reg) != ESP_LOADER_SUCCESS) {
            return false;
        }
        *cap_code = (reg >> 29) & 0x07;
        vendor_id = (reg >> 24) & 0x07;
        *vendor = flash_vendor_from_id(vendor_id);
        return true;
    }

    return false;
}

static const char *embedded_flash_cap_string(uint32_t cap_code)
{
    switch (cap_code) {
    case 1: return "Embedded Flash 4MB";
    case 2: return "Embedded Flash 2MB";
    case 3: return "Embedded Flash 1MB";
    case 4: return "Embedded Flash 8MB";
    default: return "Unknown Embedded Flash";
    }
}

static bool get_esptool_revision(esp_loader_t *loader,
        target_chip_t chip,
        uint32_t *major,
        uint32_t *minor)
{
    uint32_t word3 = 0;
    uint32_t word5 = 0;

    if (!loader || !major || !minor) {
        return false;
    }

    if (chip == ESP32C3_CHIP) {
        if (esp_loader_read_register(loader, ESP32C3_EFUSE_BLOCK1_ADDR + (4 * 3), &word3) != ESP_LOADER_SUCCESS) {
            return false;
        }
        if (esp_loader_read_register(loader, ESP32C3_EFUSE_BLOCK1_ADDR + (4 * 5), &word5) != ESP_LOADER_SUCCESS) {
            return false;
        }

        *minor = (word3 >> 18) & 0x07;
        *minor |= ((word5 >> 23) & 0x01) << 3;
        *major = (word5 >> 24) & 0x03;
        return true;
    }

    return false;
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [OPTIONS] <addr1> <file1> [<addr2> <file2> ...]\n"
            "\n"
            "Options:\n"
            "  -p, --port <device>   Serial device (default: %s)\n"
            "  -b, --baud <rate>     Baud rate     (default: %d)\n"
            "  -c, --chip <name>     Expected chip (esp32c3, esp32s3, ...)\n"
            "  -m, --mode <mode>     Who drives RESET/BOOT:\n"
            "                          auto     the flasher does it, esptool sequences (default)\n"
            "                          dtr-rts  esp-serial-flasher does it (classic circuit only)\n"
            "                          gpio     esp-serial-flasher via libgpiod lines\n"
            "                          none     nobody: the target must already be in download mode\n"
            "      --before <how>    How to enter download mode: auto | usb-jtag | classic | no-reset\n"
            "                          auto picks usb-jtag on /dev/ttyACM* (ESP32-C3/C6/H2/S3 USB),\n"
            "                          classic with an external USB-to-UART bridge\n"
            "      --after <how>     After flashing: hard-reset (default) | no-reset\n"
            "  -n, --no-stub         Use ROM bootloader instead of stub (stub is default)\n"
            "  -h, --help\n"
            "\n"
            "Examples:\n"
            "  %s 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 app.bin\n"
            "  %s -p /dev/ttyACM0 -c esp32c3 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 app.bin\n",
            prog, DEFAULT_SERIAL_DEVICE, DEFAULT_BAUD_RATE, prog, prog);
}

static uint8_t *read_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    long len;
    uint8_t *buf;

    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len <= 0) {
        fprintf(stderr, "Error: file '%s' is empty or unreadable\n", path);
        fclose(f);
        return NULL;
    }

    buf = malloc((size_t)len);
    if (!buf) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        return NULL;
    }

    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        fprintf(stderr, "Error: could not read file '%s'\n", path);
        free(buf);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *out_size = (size_t)len;
    return buf;
}

static void md5_to_hex(const uint8_t md5[16], char out[33])
{
    static const char digits[] = "0123456789abcdef";
    int i;
    for (i = 0; i < 16; i++) {
        out[i * 2] = digits[(md5[i] >> 4) & 0x0F];
        out[i * 2 + 1] = digits[md5[i] & 0x0F];
    }
    out[32] = '\0';
}

static esp_loader_error_t verify_flash_readback(esp_loader_t *loader,
        const uint8_t *expected_data,
        size_t size,
        uint32_t address)
{
    uint8_t readback[1024];
    size_t offset = 0;
    struct MD5Context expected_ctx;
    struct MD5Context actual_ctx;
    uint8_t expected_md5[16];
    uint8_t actual_md5[16];
    char expected_hex[33];
    char actual_hex[33];

    MD5Init(&expected_ctx);
    MD5Update(&expected_ctx, expected_data, (unsigned)size);
    MD5Final(expected_md5, &expected_ctx);

    MD5Init(&actual_ctx);
    while (offset < size) {
        const size_t chunk = (size - offset) < sizeof(readback) ? (size - offset) : sizeof(readback);
        esp_loader_error_t err = esp_loader_flash_read(loader, readback, address + (uint32_t)offset, (uint32_t)chunk);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Readback failed with error: %s\n", loader_err_to_string(err));
            return err;
        }
        MD5Update(&actual_ctx, readback, (unsigned)chunk);
        offset += chunk;
    }
    MD5Final(actual_md5, &actual_ctx);

    if (memcmp(expected_md5, actual_md5, sizeof(expected_md5)) != 0) {
        md5_to_hex(expected_md5, expected_hex);
        md5_to_hex(actual_md5, actual_hex);
        printf("MD5 does not match. Flash verification failed.\n");
        printf("Expected: %s\n", expected_hex);
        printf("Actual:   %s\n", actual_hex);
        return ESP_LOADER_ERROR_INVALID_MD5;
    }

    printf("Hash of data verified.\n");
    return ESP_LOADER_SUCCESS;
}

static esp_loader_error_t flash_file_raw(esp_loader_t *loader,
        const char *file_path,
        const uint8_t *bin,
        size_t size,
        uint32_t address,
        bool skip_verify)
{
    esp_loader_error_t err;
    static uint8_t payload[1024];
    const uint8_t *bin_addr = bin;
    size_t remaining = size;
    size_t written = 0;

    printf("Erasing flash (this may take a while)...\n");
    esp_loader_flash_cfg_t flash_cfg = {
        .offset = address,
        .image_size = size,
        .block_size = sizeof(payload),
        .skip_verify = skip_verify,
    };

    err = esp_loader_flash_start(loader, &flash_cfg);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Erasing flash failed with error: %s.\n", loader_err_to_string(err));
        return err;
    }

    printf("Start programming\n");
    while (remaining > 0) {
        const size_t to_write = remaining < sizeof(payload) ? remaining : sizeof(payload);
        memcpy(payload, bin_addr, to_write);

        err = esp_loader_flash_write(loader, &flash_cfg, payload, (uint32_t)to_write);
        if (err != ESP_LOADER_SUCCESS) {
            printf("\nPacket could not be written! Error %s.\n", loader_err_to_string(err));
            return err;
        }

        remaining -= to_write;
        bin_addr += to_write;
        written += to_write;
        printf("\rProgress: %d %%", (int)(((float)written / (float)size) * 100.0f));
    }
    printf("\nFinished programming\n");

    err = esp_loader_flash_finish(loader, &flash_cfg);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Flash finish failed with error: %s\n", loader_err_to_string(err));
        return err;
    }

    if (skip_verify) {
        err = verify_flash_readback(loader, bin, size, address);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Readback verification failed for '%s'\n", file_path);
            return err;
        }
    } else {
        printf("Flash verified\n");
    }

    return ESP_LOADER_SUCCESS;
}

int main(int argc, char *argv[])
{
    const char *device = DEFAULT_SERIAL_DEVICE;
    uint32_t baud_rate = DEFAULT_BAUD_RATE;
    linux_gpio_mode_t gpio_mode = LINUX_GPIO_NONE;
    bool flasher_reset = true;   /* this tool drives DTR/RTS itself */
    before_strategy_t before = BEFORE_DEFAULT;
    after_strategy_t after = AFTER_HARD_RESET;
    bool use_stub = true;
    uint32_t effective_baud = DEFAULT_BAUD_RATE;
    bool expected_chip_set = false;
    target_chip_t expected_chip = ESP_UNKNOWN_CHIP;

    enum {
        OPT_BEFORE = 1000,
        OPT_AFTER,
    };

    static const struct option long_opts[] = {
        { "port", required_argument, NULL, 'p' },
        { "baud", required_argument, NULL, 'b' },
        { "chip", required_argument, NULL, 'c' },
        { "mode", required_argument, NULL, 'm' },
        { "before", required_argument, NULL, OPT_BEFORE },
        { "after", required_argument, NULL, OPT_AFTER },
        { "no-stub", no_argument, NULL, 'n' },
        { "help", no_argument, NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:b:c:m:nh", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'p':
            device = optarg;
            break;
        case 'b':
            baud_rate = (uint32_t)strtoul(optarg, NULL, 10);
            if (baud_rate == 0) {
                fprintf(stderr, "Invalid baud rate: %s\n", optarg);
                return 1;
            }
            break;
        case 'c':
            if (!parse_chip_name(optarg, &expected_chip)) {
                fprintf(stderr, "Invalid chip '%s'\n", optarg);
                return 1;
            }
            expected_chip_set = true;
            break;
        case 'm':
            if (strcmp(optarg, "auto") == 0) {
                /* Default: the flasher owns the reset lines and the library is
                 * told not to touch them (LINUX_GPIO_NONE). */
                gpio_mode = LINUX_GPIO_NONE;
                flasher_reset = true;
            } else if (strcmp(optarg, "dtr-rts") == 0) {
                gpio_mode = LINUX_GPIO_DTR_RTS;
                flasher_reset = false;
            } else if (strcmp(optarg, "gpio") == 0) {
                gpio_mode = LINUX_GPIO_GPIOD;
                flasher_reset = false;
            } else if (strcmp(optarg, "none") == 0) {
                gpio_mode = LINUX_GPIO_NONE;
                flasher_reset = false;
            } else {
                fprintf(stderr, "Unknown mode '%s'. Use: auto | dtr-rts | gpio | none\n", optarg);
                return 1;
            }
            break;
        case OPT_BEFORE:
            if (strcmp(optarg, "auto") == 0 || strcmp(optarg, "default") == 0) {
                before = BEFORE_DEFAULT;
            } else if (strcmp(optarg, "usb-jtag") == 0) {
                before = BEFORE_USB_JTAG;
            } else if (strcmp(optarg, "classic") == 0) {
                before = BEFORE_CLASSIC;
            } else if (strcmp(optarg, "no-reset") == 0 || strcmp(optarg, "none") == 0) {
                before = BEFORE_NONE;
            } else {
                fprintf(stderr, "Unknown --before '%s'. Use: auto | usb-jtag | classic | no-reset\n", optarg);
                return 1;
            }
            break;
        case OPT_AFTER:
            if (strcmp(optarg, "hard-reset") == 0 || strcmp(optarg, "default") == 0) {
                after = AFTER_HARD_RESET;
            } else if (strcmp(optarg, "no-reset") == 0 || strcmp(optarg, "none") == 0) {
                after = AFTER_NONE;
            } else {
                fprintf(stderr, "Unknown --after '%s'. Use: hard-reset | no-reset\n", optarg);
                return 1;
            }
            break;
        case 'n':
            use_stub = false;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    {
        int remaining = argc - optind;
        if (remaining == 0) {
            fprintf(stderr, "Error: no <addr> <file> pairs given.\n\n");
            print_usage(argv[0]);
            return 1;
        }
        if ((remaining % 2) != 0) {
            fprintf(stderr, "Error: odd number of remaining arguments - expected <addr> <file> pairs.\n\n");
            print_usage(argv[0]);
            return 1;
        }
    }

    if (is_usb_jtag_serial_device(device) && use_stub) {
        printf("USB JTAG Serial detected: forcing ROM bootloader mode for reliable flashing.\n");
        use_stub = false;
    }

    if (!flasher_reset) {
        printf("Reset         : handled by esp-serial-flasher (%s)\n",
               gpio_mode == LINUX_GPIO_GPIOD ? "gpiod lines" :
               gpio_mode == LINUX_GPIO_DTR_RTS ? "dtr-rts" : "disabled");
    }

    printf("== Connection ==\n");
    printf("Port          : %s\n", device);
    printf("Baud requested: %" PRIu32 "\n", baud_rate);
    printf("GPIO mode     : %s\n",
           gpio_mode == LINUX_GPIO_DTR_RTS ? "dtr-rts" :
           gpio_mode == LINUX_GPIO_GPIOD ? "gpio" : "none");
    if (expected_chip_set) {
        printf("Expected chip : %s\n", chip_to_string(expected_chip));
    }
    printf("Connect mode  : %s\n", use_stub ? "stub" : "ROM bootloader");
    if (is_usb_jtag_serial_device(device)) {
        printf("Serial profile: USB JTAG Serial\n");
    }

    {
        esp_loader_t loader;
        esp_loader_error_t conn_err;
        linux_port_t port = {
            .port.ops = &linux_uart_ops,
            .device = device,
            .baudrate = baud_rate,
            .gpio_mode = gpio_mode,
        };

        if (gpio_mode == LINUX_GPIO_GPIOD) {
            port.gpio_chip_path = "/dev/gpiochip0";
            port.reset_pin = 2;
            port.boot_pin = 3;
        }

        if (flasher_reset && !reset_into_download_mode(device, before)) {
            return 1;
        }

        if (esp_loader_init_uart(&loader, &port.port) != ESP_LOADER_SUCCESS) {
            return 1;
        }

        conn_err = connect_with_retries(&loader, baud_rate, &use_stub, &effective_baud);

        if (conn_err != ESP_LOADER_SUCCESS) {
            esp_loader_deinit(&loader);
            return 1;
        }

        printf("Baud effective: %" PRIu32 "\n", effective_baud);

        {
            target_chip_t detected_chip = esp_loader_get_target(&loader);
            uint32_t crystal_freq = 0;
            uint32_t flash_size = 0;
            uint8_t mac[6] = {0};
            uint32_t eco_version = 0;
            uint32_t rev_major = 0;
            uint32_t rev_minor = 0;

            printf("\n== Target ==\n");
            printf("Chip          : %s\n", chip_to_string(detected_chip));
            printf("Features      : %s\n", chip_features(detected_chip));

            {
                esp_loader_target_security_info_t info;
                if (esp_loader_get_security_info(&loader, &info) == ESP_LOADER_SUCCESS &&
                    detected_chip != ESP32S2_CHIP) {
                    eco_version = info.eco_version;

                    if (get_esptool_revision(&loader, detected_chip, &rev_major, &rev_minor)) {
                        if (rev_major == 0) {
                            printf("Revision      : %" PRIu32 " (v0.%" PRIu32 ")\n", rev_minor, rev_minor);
                        } else {
                            printf("Revision      : v%" PRIu32 ".%" PRIu32 "\n", rev_major, rev_minor);
                        }
                    } else {
                        printf("Revision      : %" PRIu32 "\n", eco_version);
                    }
                    printf("Eco version   : %" PRIu32 "\n", eco_version);

                    printf("\n== Security ==\n");
                    printf("Secure boot   : %s\n", on_off(info.secure_boot_enabled));
                    printf("Secure dl mode: %s\n", on_off(info.secure_download_mode_enabled));
                    printf("Flash encrypt : %s\n", on_off(info.flash_encryption_enabled));
                    printf("JTAG soft dis.: %s\n", on_off(info.jtag_software_disabled));
                    printf("JTAG hard dis.: %s\n", on_off(info.jtag_hardware_disabled));
                    printf("USB disabled  : %s\n", on_off(info.usb_disabled));
                }
            }

            if (detected_chip == ESP32C2_CHIP) {
                if (get_crystal_frequency_esp32c2(&loader, &crystal_freq) == ESP_LOADER_SUCCESS) {
                    printf("Crystal       : %" PRIu32 "MHz\n", crystal_freq);
                }
            } else if (get_fixed_crystal_frequency(detected_chip, &crystal_freq)) {
                printf("Crystal       : %" PRIu32 "MHz\n", crystal_freq);
            }

            if (esp_loader_read_mac(&loader, mac) == ESP_LOADER_SUCCESS) {
                printf("MAC           : %02x:%02x:%02x:%02x:%02x:%02x\n",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }

            if (esp_loader_flash_detect_size(&loader, &flash_size) == ESP_LOADER_SUCCESS) {
                printf("Flash size    : %" PRIu32 " bytes (%.1f MB)\n",
                       flash_size,
                       (double)flash_size / (1024.0 * 1024.0));
            }

            {
                uint32_t cap_code = 0;
                const char *vendor = NULL;
                if (get_embedded_flash_info(&loader, detected_chip, &cap_code, &vendor)) {
                    printf("Embedded flash: %s (%s)\n", embedded_flash_cap_string(cap_code), vendor);
                }
            }

            if (expected_chip_set && detected_chip != expected_chip) {
                fprintf(stderr, "Error: detected chip '%s' differs from expected '%s'\n",
                        chip_to_string(detected_chip), chip_to_string(expected_chip));
                return 1;
            }
        }

        {
            int num_pairs = (argc - optind) / 2;
            char **pair_args = argv + optind;
            int i;
            uint64_t total_written = 0;
            double total_time = 0.0;

            printf("\n== Flash Plan ==\n");
            for (i = 0; i < num_pairs; i++) {
                const char *addr_str = pair_args[i * 2];
                const char *file_path = pair_args[i * 2 + 1];
                char *endptr;
                uint32_t addr = (uint32_t)strtoul(addr_str, &endptr, 0);
                if (*endptr == '\0') {
                    printf("[%d/%d] 0x%08" PRIx32 " <- %s\n", i + 1, num_pairs, addr, file_path);
                }
            }

            printf("\n== Flashing ==\n");

            for (i = 0; i < num_pairs; i++) {
                const char *addr_str = pair_args[i * 2];
                const char *file_path = pair_args[i * 2 + 1];
                char *endptr;
                uint32_t addr = (uint32_t)strtoul(addr_str, &endptr, 0);
                size_t size = 0;
                uint8_t *buf;
                esp_loader_error_t err;
                double t0;
                double t1;
                double elapsed;
                double kbit_s;
                const char *verify_mode = (is_usb_jtag_serial_device(device) && !use_stub)
                                          ? "readback-hash"
                                          : "md5";

                if (*endptr != '\0') {
                    fprintf(stderr, "Error: invalid address '%s'\n", addr_str);
                    esp_loader_deinit(&loader);
                    return 1;
                }

                buf = read_file(file_path, &size);
                if (!buf) {
                    esp_loader_deinit(&loader);
                    return 1;
                }

                printf("\n-- [%d/%d] Flash '%s' @ 0x%08" PRIx32 " (%zu bytes) --\n",
                       i + 1, num_pairs, file_path, addr, size);
                t0 = now_seconds();
                if (is_usb_jtag_serial_device(device) && !use_stub) {
                    err = flash_file_raw(&loader, file_path, buf, size, addr, true);
                } else {
                    err = flash_file_raw(&loader, file_path, buf, size, addr, false);
                }
                t1 = now_seconds();
                free(buf);

                elapsed = t1 - t0;
                if (elapsed <= 0.0) {
                    elapsed = 0.001;
                }
                kbit_s = (((double)size * 8.0) / 1000.0) / elapsed;

                if (err != ESP_LOADER_SUCCESS) {
                    printf("RESULT        : FAIL (%s)\n", loader_err_to_string(err));
                    fprintf(stderr, "Error: failed to flash '%s'\n", file_path);
                    esp_loader_deinit(&loader);
                    return 1;
                }

                total_written += (uint64_t)size;
                total_time += elapsed;
                printf("RESULT        : OK\n");
                printf("Bytes written : %zu\n", size);
                printf("Elapsed       : %.1f s\n", elapsed);
                printf("Throughput    : %.1f kbit/s\n", kbit_s);
                printf("Verify mode   : %s\n", verify_mode);
            }

            if (total_time <= 0.0) {
                total_time = 0.001;
            }

            printf("\n== Summary ==\n");
            printf("Total written : %" PRIu64 " bytes\n", total_written);
            printf("Total elapsed : %.1f s\n", total_time);
            printf("Avg throughput: %.1f kbit/s\n",
                   ((((double)total_written * 8.0) / 1000.0) / total_time));
        }

        if (flasher_reset) {
            esp_loader_deinit(&loader);
            printf("\nAll done!\n");
            reset_into_application(device, after);
        } else {
            printf("\nAll done! Resetting target...\n");
            esp_loader_reset_target(&loader);
            esp_loader_deinit(&loader);
        }
    }

    return 0;
}
