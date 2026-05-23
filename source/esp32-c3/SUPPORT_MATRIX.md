# ESP32-C3 Super Mini Support Matrix

This document lists the feature status of the ESP32-C3 firmware port in
`source/esp32-c3/ble50_scan` relative to the protocol and behaviors documented
in `README2.md`.

## Supported

- CRC16-framed bidirectional transport over the ESP32-C3 USB serial interface.
- `CMD_ID_INFO (0x00)`.
- `CMD_ID_SCAN (0x01)`.
  - short scan format supported
  - extended scan format with independent 1M and Coded windows supported
  - passive/active scan flag supported
  - duplicate-filter flag supported
  - public/random/private address filter bits supported
  - concurrent 1M + Coded PHY scanning supported
- `CMD_ID_WMAC (0x02)` and `CMD_ID_BMAC (0x03)`.
  - separate white-list and black-list supported
  - 1..6-byte MAC prefix entries supported
  - total capacity: 64 white-list entries and 64 black-list entries
- `CMD_ID_CLRM (0x04)` clears both lists.
- `CMD_ID_PRNT (0x05)` framed debug-print transport implemented.
- `CMD_ID_GPIO (0x06)` with ESP32-specific pin mapping.
  - `op 0` query board mask and pin state supported
  - `op 1` read single pin supported
  - `op 4` configure GPIO direction/pull supported on exposed safe GPIOs
  - `op 2` write and `op 3` toggle supported on all exposed safe GPIOs
  - `op 7` raw analog read supported on `GPIO0..GPIO4`
- `CMD_ID_UART (0x08)`.
  - `op 0` query supported
  - `op 1` ping/echo supported
- `CMD_ID_RFSDK (0x09)`.
  - `op 0` query supported
  - `op 1` BLE TX power set/query supported through ESP-IDF power APIs
  - `op 4` Coded PHY minimum scan window supported
- `CMD_ID_VERSION (0x0A)` supported.
- `CMD_ID_TXADV (0x0B)` custom advertisement transmission supported.
  - stop/query supported
  - start supported for legacy 1M, extended 1M and extended Coded PHY modes
  - current running PHY / interval / payload length status response supported
- `CMD_ID_CONN (0x0C)` BLE central control supported.
  - `op 0` status supported
  - `op 1` connect on 1M PHY supported
  - `op 2` connect on Coded PHY supported
  - `op 3` disconnect supported
  - `op 4` cancel pending connect supported
- `CMD_ID_TXDATA (0x0D)` ATT write-without-response supported.
- `CMD_ID_RXDATA (0x0E)` ATT notifications / indications forwarded to the host.
- `adv2uart.py` default scanner workflow supported:
  - connect/open serial device
  - `INFO`
  - `CLRM`
  - white/black list programming
  - start scan
  - stop scan
  - `INFO` during active scan

## Partially Supported

- LED monitoring is adapted for ESP32-C3 Super Mini hardware.
  - the original TLSR board had multiple LEDs with separate meanings
  - the ESP32-C3 port only has a single board LED activity indication
  - the LED emits a short activity pulse on forwarded advertisements and on host commands that do not directly target the board LED GPIO, then restores the requested steady state
- GPIO identifiers are not TLSR `PA/PB/PC` port codes.
  - on ESP32-C3, `pin_code` is the raw GPIO number
  - default board LED assumption is `GPIO8`
  - if your board uses a different LED pin, change `BOARD_LED_GPIO` in
    `ble50_scan/main/ble50_scan.c`
- `CMD_ID_UART (0x08)` baud-rate change is not applied to the native USB serial
  transport.
  - `op 2` is intentionally denied
  - query/ping remain supported
  - `BOOT / GPIO9` can be used as an application input, but it cannot switch the real USB Serial/JTAG baud because that transport does not use a programmable UART clock
- `CMD_ID_RFSDK (0x09)` is partial.
  - TX power is supported
  - Coded PHY minimum scan window is supported
  - crystal capacitance override is not supported on ESP32-C3
  - custom scan-channel selection is not exposed by the current ESP-IDF scan API
- `CMD_ID_TXADV (0x0B)` is limited to 31-byte advertising payloads in the current host/UI flow.
  - this matches the legacy/GUI payload cap used by the restored TX advertising tab
- `CMD_ID_CONN (0x0C)` central mode is limited to the legacy BLE2UART service layout.
  - service discovery is filtered to UUID `0x00FF`
  - automatic notify registration is filtered to characteristic UUID `0xFF01`
  - the reported handle is the discovered notify characteristic handle when available
- `CMD_ID_TXDATA (0x0D)` currently uses ATT Write Without Response only.
  - the restored GUI caps payloads to 20 bytes
  - no reliable ATT write-complete callback is expected for this mode

## Not Supported

- `CMD_ID_VBAT (0x0F)`.
  - the ESP32-C3 Super Mini board does not expose the `3.3V` rail to a known ADC
    input in this project
  - the firmware returns `CMD_STATUS_DENIED`
  - `adv2uart.py --battery` therefore fails gracefully by design
- TB-03F-KIT-specific hardware semantics.
  - RGB LED color meanings
  - dedicated key button behavior for bitrate switching
  - TLSR-specific GPIO numbering
  - TLSR `VBAT` sensing path
- Telink-specific flashing and SDK flows described in `README2.md`.
  - `Telink_Tools.py`
  - `TlsrPgm.py`
  - Telink SDK installation/update steps

## Not Implemented By Design In This Port

- The original TLSR software FIFO design is not replicated exactly.
  - the ESP32-C3 port currently forwards framed packets directly through the USB
    serial path instead of maintaining the same 4-packet FIFO structure
- The Telink persistent user-data layout described in `README2.md` is not used.
  - this ESP32-C3 port relies on runtime configuration only for the currently
    supported command set

## Safe GPIO Set

The current firmware accepts these raw ESP32-C3 GPIO numbers through
`CMD_ID_GPIO`:

- `0`
- `1`
- `2`
- `3`
- `4`
- `5`
- `6`
- `7`
- `8`
- `9`
- `10`
- `20`
- `21`

Pins used by native USB or otherwise avoided by this port are intentionally not
accepted.

## Validation Status

- The firmware builds successfully with `./build.sh` on ESP-IDF `v6.0.1` for the validated GPIO/ADC/TXADV slice.
- The latest BLE central changes were additionally validated by forced rebuild of:
  - `ble50_scan/build/esp-idf/main/CMakeFiles/__idf_main.dir/ble50_scan.c.obj`
  - `ble50_scan/build/esp-idf/main/libmain.a`
- Generated artifacts verified:
  - `ble50_scan/build/ble50_scan.bin`
  - `ble50_scan/build/ble50_scan.elf`
  - `ble50_scan/build/bootloader.bin`
  - `ble50_scan/build/partition-table.bin`