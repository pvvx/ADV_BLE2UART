#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import queue
import re
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path

try:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
except ImportError as exc:
    raise SystemExit("Tkinter is required. Install the Python Tcl/Tk package for your OS.") from exc

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


CMD_ID_INFO = 0x00
CMD_ID_SCAN = 0x01
CMD_ID_WMAC = 0x02
CMD_ID_BMAC = 0x03
CMD_ID_CLRM = 0x04
CMD_ID_PRNT = 0x05
CMD_ID_GPIO = 0x06
CMD_ID_UART = 0x08
CMD_ID_RFSDK = 0x09
CMD_ID_VERSION = 0x0A
CMD_ID_TXADV = 0x0B
CMD_ID_CONN   = 0x0C
CMD_ID_TXDATA = 0x0D
CMD_ID_RXDATA = 0x0E
CMD_ID_VBAT = 0x0F
CMD_ID_GPIOEVT = 0x10

# CMD_ID_GPIOEVT sub-operations matching the firmware enum.
GPIOEVT_OP_QUERY = 0
GPIOEVT_OP_ENABLE = 1
GPIOEVT_OP_DISABLE = 2
GPIOEVT_OP_CLEAR = 3
GPIOEVT_EVENT_FLAG = 0x80  # high bit of the response 'index' field marks a spontaneous event

GUI_BOOTSTRAP_DELAY_MS = 1000
GUI_SERIAL_TIMEOUT_S = 0.3

@dataclass(frozen=True)
class DeviceProfile:
    profile_id: str
    label: str
    board_frame_title: str
    board_help_text: str
    rf_frame_title: str
    rf_note: str
    status_pin_label: str
    led_status_label: str
    board_mask_label: str
    direct_blink_label: str
    vbat_note: str
    uart_note: str
    default_gpio_pin: str
    default_rf_power_label: str
    board_led_pin_id: int
    board_led_active_low: bool
    status_pin_id: int
    status_pin_active_low: bool
    status_pin_active_label: str
    status_pin_inactive_label: str
    supports_vbat: bool
    uart_set_baud_supported: bool
    rf_cap_supported: bool
    rf_channels_supported: bool
    supports_txadv: bool
    supports_conn: bool
    supports_analog: bool
    supports_gpio_events: bool
    allow_general_gpio_write: bool
    led_mask_mode: bool
    legacy_reset_default: bool
    board_pins: dict[str, int]
    gpio_pulls: dict[str, int]
    led_bits: dict[str, int]
    led_pin_codes: dict[int, int]
    hw_version_labels: dict[int, str]
    firmware_bauds: tuple[int, ...]
    rf_power_options: tuple[tuple[str, int], ...]


ESP32_C3_PROFILE = DeviceProfile(
    profile_id="esp32-c3",
    label="ESP32-C3",
    board_frame_title="ESP32-C3 GPIO / Board LED",
    board_help_text="Read/write/configure works on safe ESP32-C3 GPIOs; analog reads are available on GPIO0-GPIO4.",
    rf_frame_title="ESP32-C3 BLE Scan / RF",
    rf_note="XTAL cap and custom scan channels are intentionally unavailable in the current ESP32-C3 firmware.",
    status_pin_label="GPIO7",
    led_status_label="LED",
    board_mask_label="Board feature mask",
    direct_blink_label="Blink board LED",
    vbat_note="VBAT command is not supported by the current ESP32-C3 firmware",
    uart_note="Baud-rate change is intentionally denied on the native ESP32-C3 USB Serial/JTAG transport.",
    default_gpio_pin="Board LED GPIO8",
    default_rf_power_label="ESP_PWR_LVL_P3 (+3 dBm)",
    board_led_pin_id=0x08,
    board_led_active_low=True,
    status_pin_id=0x07,
    status_pin_active_low=False,
    status_pin_active_label="High",
    status_pin_inactive_label="Low",
    supports_vbat=False,
    uart_set_baud_supported=False,
    rf_cap_supported=False,
    rf_channels_supported=False,
    supports_txadv=True,
    supports_conn=True,
    supports_analog=True,
    supports_gpio_events=True,
    allow_general_gpio_write=True,
    led_mask_mode=False,
    legacy_reset_default=False,
    board_pins={
        "Board LED GPIO8": 0x08,
        "GPIO0": 0x00,
        "GPIO1": 0x01,
        "GPIO2": 0x02,
        "GPIO3": 0x03,
        "GPIO4": 0x04,
        "GPIO5": 0x05,
        "GPIO6": 0x06,
        "GPIO7": 0x07,
        "BOOT / GPIO9": 0x09,
        "GPIO10": 0x0A,
        "GPIO20": 0x14,
        "GPIO21": 0x15,
    },
    gpio_pulls={
        "Float": 0,
        "Pull-up": 1,
        "Pull-down": 2,
    },
    led_bits={
        "Board LED GPIO8": 0x01,
    },
    led_pin_codes={
        0x01: 0x08,
    },
    hw_version_labels={
        0xC3: "ESP32-C3 Super Mini",
    },
    firmware_bauds=(2000000, 921600, 115200),
    rf_power_options=(
        ("ESP_PWR_LVL_N24 (-24 dBm)", 0),
        ("ESP_PWR_LVL_N21 (-21 dBm)", 1),
        ("ESP_PWR_LVL_N18 (-18 dBm)", 2),
        ("ESP_PWR_LVL_N15 (-15 dBm)", 3),
        ("ESP_PWR_LVL_N12 (-12 dBm)", 4),
        ("ESP_PWR_LVL_N9 (-9 dBm)", 5),
        ("ESP_PWR_LVL_N6 (-6 dBm)", 6),
        ("ESP_PWR_LVL_N3 (-3 dBm)", 7),
        ("ESP_PWR_LVL_N0 (0 dBm)", 8),
        ("ESP_PWR_LVL_P3 (+3 dBm)", 9),
        ("ESP_PWR_LVL_P6 (+6 dBm)", 10),
        ("ESP_PWR_LVL_P9 (+9 dBm)", 11),
        ("ESP_PWR_LVL_P12 (+12 dBm)", 12),
        ("ESP_PWR_LVL_P15 (+15 dBm)", 13),
        ("ESP_PWR_LVL_P18 (+18 dBm)", 14),
        ("ESP_PWR_LVL_P20 (+20 dBm)", 15),
    ),
)

TB_03F_KIT_PROFILE = DeviceProfile(
    profile_id="tb-03f-kit",
    label="TB-03F-KIT",
    board_frame_title="TB-03F-KIT GPIO / LEDs",
    board_help_text="TB-03F-KIT firmware supports direct GPIO read/write/toggle on the exposed board pins.",
    rf_frame_title="TB-03F-KIT RF",
    rf_note="XTAL cap and primary channel controls are available on the TB-03F-KIT firmware.",
    status_pin_label="Key",
    led_status_label="LED mask",
    board_mask_label="Board mask",
    direct_blink_label="Blink all LEDs",
    vbat_note="chip internal sensor; Supply = device 3.3V / VBAT rail",
    uart_note="Firmware baud selection is available on the TB-03F-KIT UART transport.",
    default_gpio_pin="RGB blue PC2",
    default_rf_power_label="+10.46 dBm VBAT",
    board_led_pin_id=0x22,
    board_led_active_low=False,
    status_pin_id=0x07,
    status_pin_active_low=True,
    status_pin_active_label="Pressed",
    status_pin_inactive_label="Released",
    supports_vbat=True,
    uart_set_baud_supported=True,
    rf_cap_supported=True,
    rf_channels_supported=True,
    supports_txadv=False,
    supports_conn=False,
    supports_analog=False,
    supports_gpio_events=False,
    allow_general_gpio_write=True,
    led_mask_mode=True,
    legacy_reset_default=True,
    board_pins={
        "Key/SWS PA7": 0x07,
        "Side yellow PB4": 0x14,
        "Side white PB5": 0x15,
        "RGB blue PC2": 0x22,
        "RGB red PC3": 0x23,
        "RGB green PC4": 0x24,
    },
    gpio_pulls={
        "Float": 0,
        "Pull-up 1M": 1,
        "Pull-down 100K": 2,
        "Pull-up 10K": 3,
    },
    led_bits={
        "Blue PC2": 0x01,
        "Red PC3": 0x02,
        "Green PC4": 0x04,
        "Yellow PB4": 0x08,
        "White PB5": 0x10,
    },
    led_pin_codes={
        0x01: 0x22,
        0x02: 0x23,
        0x04: 0x24,
        0x08: 0x14,
        0x10: 0x15,
    },
    hw_version_labels={
        0x10: "0x10 TB-03F KIT",
    },
    firmware_bauds=(2000000, 921600, 115200),
    rf_power_options=(
        ("+10.46 dBm VBAT", 0x3F),
        ("+10.01 dBm VBAT", 0x3A),
        ("+7.79 dBm VBAT", 0x29),
        ("+5.13 dBm VBAT", 0x1D),
        ("+3.01 dBm VANT", 0xBF),
        ("+0.04 dBm VANT", 0xA9),
        ("-5.03 dBm VANT", 0x96),
        ("-15.88 dBm VANT", 0x86),
        ("-30 dBm", 0xFF),
    ),
)

DEVICE_PROFILES = {
    ESP32_C3_PROFILE.profile_id: ESP32_C3_PROFILE,
    TB_03F_KIT_PROFILE.profile_id: TB_03F_KIT_PROFILE,
}
DEFAULT_DEVICE_PROFILE_ID = ESP32_C3_PROFILE.profile_id

DEVICE_PROFILE = ESP32_C3_PROFILE
DEVICE_LABEL = DEVICE_PROFILE.label
DEVICE_BOARD_FRAME_TITLE = DEVICE_PROFILE.board_frame_title
DEVICE_BOARD_HELP_TEXT = DEVICE_PROFILE.board_help_text
DEVICE_RF_FRAME_TITLE = DEVICE_PROFILE.rf_frame_title
DEVICE_RF_NOTE = DEVICE_PROFILE.rf_note
DEVICE_STATUS_PIN_LABEL = DEVICE_PROFILE.status_pin_label
DEVICE_LED_STATUS_LABEL = DEVICE_PROFILE.led_status_label
DEVICE_BOARD_MASK_LABEL = DEVICE_PROFILE.board_mask_label
DEVICE_DIRECT_BLINK_LABEL = DEVICE_PROFILE.direct_blink_label
DEVICE_VBAT_NOTE = DEVICE_PROFILE.vbat_note
DEVICE_UART_NOTE = DEVICE_PROFILE.uart_note
DEVICE_DEFAULT_GPIO_PIN = DEVICE_PROFILE.default_gpio_pin
DEVICE_DEFAULT_RF_POWER_LABEL = DEVICE_PROFILE.default_rf_power_label
DEVICE_SUPPORTS_VBAT = DEVICE_PROFILE.supports_vbat
UART_SET_BAUD_SUPPORTED = DEVICE_PROFILE.uart_set_baud_supported
RF_CAP_SUPPORTED = DEVICE_PROFILE.rf_cap_supported
RF_CHANNELS_SUPPORTED = DEVICE_PROFILE.rf_channels_supported
DEVICE_SUPPORTS_TXADV = DEVICE_PROFILE.supports_txadv
DEVICE_SUPPORTS_CONN = DEVICE_PROFILE.supports_conn
DEVICE_SUPPORTS_ANALOG = DEVICE_PROFILE.supports_analog
DEVICE_SUPPORTS_GPIO_EVENTS = DEVICE_PROFILE.supports_gpio_events
DEVICE_ALLOW_GENERAL_GPIO_WRITE = DEVICE_PROFILE.allow_general_gpio_write
DEVICE_LED_MASK_MODE = DEVICE_PROFILE.led_mask_mode
DEVICE_LEGACY_RESET_DEFAULT = DEVICE_PROFILE.legacy_reset_default
DEVICE_STATUS_PIN_ACTIVE_LOW = DEVICE_PROFILE.status_pin_active_low
DEVICE_STATUS_PIN_ACTIVE_LABEL = DEVICE_PROFILE.status_pin_active_label
DEVICE_STATUS_PIN_INACTIVE_LABEL = DEVICE_PROFILE.status_pin_inactive_label
DEVICE_BOARD_LED_ACTIVE_LOW = DEVICE_PROFILE.board_led_active_low
BOARD_LED_PIN_ID = DEVICE_PROFILE.board_led_pin_id
STATUS_GPIO_PIN_ID = DEVICE_PROFILE.status_pin_id
BOOT_BUTTON_PIN_ID = 0x09


def apply_device_profile(profile_id: str):
    global DEVICE_PROFILE, DEVICE_LABEL, DEVICE_BOARD_FRAME_TITLE, DEVICE_BOARD_HELP_TEXT
    global DEVICE_RF_FRAME_TITLE, DEVICE_RF_NOTE, DEVICE_STATUS_PIN_LABEL, DEVICE_LED_STATUS_LABEL
    global DEVICE_BOARD_MASK_LABEL, DEVICE_DIRECT_BLINK_LABEL, DEVICE_VBAT_NOTE, DEVICE_UART_NOTE
    global DEVICE_DEFAULT_GPIO_PIN, DEVICE_DEFAULT_RF_POWER_LABEL, DEVICE_SUPPORTS_VBAT
    global UART_SET_BAUD_SUPPORTED, RF_CAP_SUPPORTED, RF_CHANNELS_SUPPORTED
    global DEVICE_SUPPORTS_TXADV, DEVICE_SUPPORTS_CONN, DEVICE_SUPPORTS_ANALOG
    global DEVICE_SUPPORTS_GPIO_EVENTS
    global DEVICE_ALLOW_GENERAL_GPIO_WRITE, DEVICE_LED_MASK_MODE, DEVICE_LEGACY_RESET_DEFAULT
    global DEVICE_STATUS_PIN_ACTIVE_LOW, DEVICE_STATUS_PIN_ACTIVE_LABEL, DEVICE_STATUS_PIN_INACTIVE_LABEL
    global DEVICE_BOARD_LED_ACTIVE_LOW, BOARD_LED_PIN_ID, STATUS_GPIO_PIN_ID
    global BOARD_PINS, BOARD_PIN_NAMES, BOARD_PIN_MASK_INDEX, GPIO_PULLS
    global LED_BITS, LED_ALL_MASK, LED_PIN_CODES, LED_PIN_IDS, HW_VERSION_LABELS
    global FIRMWARE_BAUDS, RF_POWER_OPTIONS, RF_POWER_LABEL_BY_VALUE, RF_POWER_VALUE_BY_LABEL

    DEVICE_PROFILE = DEVICE_PROFILES[profile_id]
    DEVICE_LABEL = DEVICE_PROFILE.label
    DEVICE_BOARD_FRAME_TITLE = DEVICE_PROFILE.board_frame_title
    DEVICE_BOARD_HELP_TEXT = DEVICE_PROFILE.board_help_text
    DEVICE_RF_FRAME_TITLE = DEVICE_PROFILE.rf_frame_title
    DEVICE_RF_NOTE = DEVICE_PROFILE.rf_note
    DEVICE_STATUS_PIN_LABEL = DEVICE_PROFILE.status_pin_label
    DEVICE_LED_STATUS_LABEL = DEVICE_PROFILE.led_status_label
    DEVICE_BOARD_MASK_LABEL = DEVICE_PROFILE.board_mask_label
    DEVICE_DIRECT_BLINK_LABEL = DEVICE_PROFILE.direct_blink_label
    DEVICE_VBAT_NOTE = DEVICE_PROFILE.vbat_note
    DEVICE_UART_NOTE = DEVICE_PROFILE.uart_note
    DEVICE_DEFAULT_GPIO_PIN = DEVICE_PROFILE.default_gpio_pin
    DEVICE_DEFAULT_RF_POWER_LABEL = DEVICE_PROFILE.default_rf_power_label
    DEVICE_SUPPORTS_VBAT = DEVICE_PROFILE.supports_vbat
    UART_SET_BAUD_SUPPORTED = DEVICE_PROFILE.uart_set_baud_supported
    RF_CAP_SUPPORTED = DEVICE_PROFILE.rf_cap_supported
    RF_CHANNELS_SUPPORTED = DEVICE_PROFILE.rf_channels_supported
    DEVICE_SUPPORTS_TXADV = DEVICE_PROFILE.supports_txadv
    DEVICE_SUPPORTS_CONN = DEVICE_PROFILE.supports_conn
    DEVICE_SUPPORTS_ANALOG = DEVICE_PROFILE.supports_analog
    DEVICE_SUPPORTS_GPIO_EVENTS = DEVICE_PROFILE.supports_gpio_events
    DEVICE_ALLOW_GENERAL_GPIO_WRITE = DEVICE_PROFILE.allow_general_gpio_write
    DEVICE_LED_MASK_MODE = DEVICE_PROFILE.led_mask_mode
    DEVICE_LEGACY_RESET_DEFAULT = DEVICE_PROFILE.legacy_reset_default
    DEVICE_STATUS_PIN_ACTIVE_LOW = DEVICE_PROFILE.status_pin_active_low
    DEVICE_STATUS_PIN_ACTIVE_LABEL = DEVICE_PROFILE.status_pin_active_label
    DEVICE_STATUS_PIN_INACTIVE_LABEL = DEVICE_PROFILE.status_pin_inactive_label
    DEVICE_BOARD_LED_ACTIVE_LOW = DEVICE_PROFILE.board_led_active_low
    BOARD_LED_PIN_ID = DEVICE_PROFILE.board_led_pin_id
    STATUS_GPIO_PIN_ID = DEVICE_PROFILE.status_pin_id

    BOARD_PINS = dict(DEVICE_PROFILE.board_pins)
    BOARD_PIN_NAMES = {value: key for key, value in BOARD_PINS.items()}
    BOARD_PIN_MASK_INDEX = {pin_id: index for index, pin_id in enumerate(sorted(BOARD_PIN_NAMES))}
    GPIO_PULLS = dict(DEVICE_PROFILE.gpio_pulls)
    LED_BITS = dict(DEVICE_PROFILE.led_bits)
    LED_ALL_MASK = 0
    for mask in LED_BITS.values():
        LED_ALL_MASK |= mask
    LED_PIN_CODES = dict(DEVICE_PROFILE.led_pin_codes)
    LED_PIN_IDS = tuple(LED_PIN_CODES[bit] for bit in LED_PIN_CODES)
    HW_VERSION_LABELS = dict(DEVICE_PROFILE.hw_version_labels)
    FIRMWARE_BAUDS = tuple(DEVICE_PROFILE.firmware_bauds)
    RF_POWER_OPTIONS = tuple(DEVICE_PROFILE.rf_power_options)
    RF_POWER_LABEL_BY_VALUE = {value: label for label, value in RF_POWER_OPTIONS}
    RF_POWER_VALUE_BY_LABEL = {label: value for label, value in RF_POWER_OPTIONS}


apply_device_profile(DEFAULT_DEVICE_PROFILE_ID)

TXADV_PHY_OPTIONS = [
    ("Legacy 1M (ADV_NONCONN_IND)",  0),
    ("Extended 1M",                  1),
    ("Extended Coded PHY",           2),
]
TXADV_PHY_NAMES = {v: k for k, v in [(lbl, val) for lbl, val in TXADV_PHY_OPTIONS]}

CONN_PHY_OPTIONS = [
    ("1M PHY",    0),
    ("Coded PHY", 1),
]

HEAD_CRC_ADD_LEN = 13
MAX_ADV_PAYLOAD = 229

COMMAND_STATUS = {
    0: "OK",
    1: "Bad arguments",
    2: "Unknown pin",
    3: "Denied",
    4: "Bad value",
}

COMMAND_NAMES = {
    CMD_ID_INFO: "INFO",
    CMD_ID_SCAN: "SCAN",
    CMD_ID_WMAC: "WMAC",
    CMD_ID_BMAC: "BMAC",
    CMD_ID_CLRM: "CLRM",
    CMD_ID_PRNT: "PRNT",
    CMD_ID_GPIO: "GPIO",
    CMD_ID_UART: "UART",
    CMD_ID_RFSDK: "RFSDK",
    CMD_ID_VERSION: "VERSION",
    CMD_ID_TXADV: "TXADV",
    CMD_ID_CONN: "CONN",
    CMD_ID_TXDATA: "TXDATA",
    CMD_ID_RXDATA: "RXDATA",
    CMD_ID_VBAT: "VBAT",
    CMD_ID_GPIOEVT: "GPIOEVT",
}

RF_POWER_LABEL_BY_VALUE = {value: label for label, value in RF_POWER_OPTIONS}
RF_POWER_VALUE_BY_LABEL = {label: value for label, value in RF_POWER_OPTIONS}

ESP32_C3_ANALOG_PIN_IDS = (0x00, 0x01, 0x02, 0x03, 0x04)


def analog_capable_pin_names() -> tuple[str, ...]:
    return tuple(name for name, pin_id in BOARD_PINS.items() if pin_id in ESP32_C3_ANALOG_PIN_IDS)


def default_analog_pin_name() -> str:
    names = analog_capable_pin_names()
    return names[0] if names else DEVICE_DEFAULT_GPIO_PIN

OWN_ADDRESS_TYPES = {
    0: "PUBLIC",
    1: "RANDOM",
    2: "RESOLVE_PRIVATE_PUBLIC",
    3: "RESOLVE_PRIVATE_RANDOM",
}

ADDRESS_TYPE_NAMES = {
    0: "Public",
    1: "Random",
    2: "Public identity",
    3: "Random identity",
}

PHY_NAMES = {
    0: "None",
    1: "1M",
    2: "2M",
    3: "Coded",
}


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for value in data:
        crc ^= value
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
            crc &= 0xFFFF
    return crc


def append_crc(payload: bytes) -> bytes:
    checksum = crc16(payload)
    return payload + bytes((checksum & 0xFF, checksum >> 8))


def signed_u8(value: int) -> int:
    return value - 256 if value & 0x80 else value


def normalize_mac(text: str) -> str:
    normalized = re.sub(r"[^0-9a-fA-F]", "", text).upper()
    n = len(normalized)
    if n == 0 or n % 2 != 0 or n > 12:
        raise ValueError("MAC address must contain 2–12 even-count hexadecimal digits (1–6 bytes)")
    return normalized


def mac_to_wire(text: str) -> bytes:
    return bytes.fromhex(normalize_mac(text))[::-1]


def mac_from_wire(data: bytes) -> str:
    return data[::-1].hex().upper()


def bytes_to_hex(data: bytes) -> str:
    return data.hex().upper()


def command_name(command: int) -> str:
    return COMMAND_NAMES.get(command, f"0x{command:02X}")


def describe_tx_payload(payload: bytes) -> str:
    if not payload:
        return "empty payload"
    command = payload[0]
    if command == CMD_ID_INFO and len(payload) == 1:
        return "INFO request"
    if command == CMD_ID_VERSION and len(payload) == 1:
        return "VERSION request"
    if command == CMD_ID_VBAT and len(payload) == 1:
        return "VBAT request"
    if command == CMD_ID_SCAN:
        if payload == bytes((CMD_ID_SCAN, 0, 0, 0)):
            return "SCAN stop"
        if len(payload) in (4, 6):
            try:
                return f"SCAN start {ScanConfig.from_payload(payload[1:]).describe()}"
            except ValueError:
                return "SCAN request"
    if command in (CMD_ID_WMAC, CMD_ID_BMAC) and 2 <= len(payload) <= 7:
        mode = "white" if command == CMD_ID_WMAC else "black"
        mac_bytes = payload[1:]
        mac_str = mac_from_wire(mac_bytes)
        label = "MAC" if len(mac_bytes) == 6 else f"MAC prefix ({len(mac_bytes)}B)"
        return f"{command_name(command)} add {mode} {label} {mac_str}"
    if command == CMD_ID_CLRM and len(payload) == 1:
        return "CLRM clear MAC list"
    if command == CMD_ID_GPIO:
        if len(payload) < 2:
            return "GPIO request"
        op = payload[1]
        pin_id = payload[2] if len(payload) > 2 else None
        pin_name = BOARD_PIN_NAMES.get(pin_id, f"pin 0x{pin_id:02X}") if pin_id is not None else "pin ?"
        if op == 0:
            return f"GPIO status {pin_name}"
        if op == 1 and len(payload) >= 3:
            return f"GPIO read {pin_name}"
        if op == 2 and len(payload) >= 4:
            return f"GPIO write {pin_name}={1 if payload[3] else 0}"
        if op == 3 and len(payload) >= 3:
            return f"GPIO toggle {pin_name}"
        if op == 4 and len(payload) >= 6:
            pull_name = next((name for name, value in GPIO_PULLS.items() if value == payload[5]), f"0x{payload[5]:02X}")
            return (
                f"GPIO config {pin_name} in={1 if payload[3] else 0} "
                f"out={1 if payload[4] else 0} pull={pull_name}"
            )
        if op == 5 and len(payload) >= 6:
            period = payload[4] | (payload[5] << 8)
            return f"GPIO PWM {pin_name} duty={payload[3]}% period={period} us"
        if op == 6 and len(payload) >= 3:
            return f"GPIO PWM off {pin_name}"
        if op == 7 and len(payload) >= 3:
            return f"GPIO analog read {pin_name}"
        return f"GPIO op={op}"
    return command_name(command)


def format_tx_log(payload: bytes) -> str:
    return f"TX {bytes_to_hex(payload)} | {describe_tx_payload(payload)}"


def payload_ascii(data: bytes) -> str:
    return "".join(chr(value) if 32 <= value <= 126 else "." for value in data)


def scan_units_from_ms(window_ms: float) -> int:
    return max(0, min(0x7FFF, int(float(window_ms) / 0.625)))


def scan_ms_from_units(units: int) -> float:
    return units * 0.625


@dataclass
class ScanConfig:
    own_address_type: int = 0
    filter_random_addresses: bool = False
    filter_private_addresses: bool = False
    duplicate_filter: bool = False
    active_scan: bool = False
    phy_1m: bool = True
    phy_coded: bool = True
    window_ms: float = 30.0
    window_coded_ms: float = 0.0  # 0 = same as window_ms (legacy behaviour)

    def to_payload(self) -> bytes:
        flag = (self.own_address_type & 0x03) << 6
        if self.filter_random_addresses:
            flag |= 1 << 5
        if self.filter_private_addresses:
            flag |= 1 << 4
        if self.duplicate_filter:
            flag |= 1 << 3
        if self.active_scan:
            flag |= 1 << 2
        if self.phy_coded:
            flag |= 1 << 1
        if self.phy_1m:
            flag |= 1
        units_1m = scan_units_from_ms(self.window_ms)
        # Send extended 5-byte payload only when coded window differs from 1M window
        effective_coded = self.window_coded_ms if self.window_coded_ms > 0.0 else self.window_ms
        if self.phy_coded and abs(effective_coded - self.window_ms) > 0.1:
            units_coded = scan_units_from_ms(effective_coded)
            return bytes((flag,
                          units_1m & 0xFF, (units_1m >> 8) & 0xFF,
                          units_coded & 0xFF, (units_coded >> 8) & 0xFF))
        return bytes((flag, units_1m & 0xFF, (units_1m >> 8) & 0xFF))

    @classmethod
    def from_payload(cls, payload: bytes) -> "ScanConfig":
        if len(payload) not in (3, 5):
            raise ValueError("scan payload must be 3 or 5 bytes")
        flag = payload[0]
        units_1m = int.from_bytes(payload[1:3], byteorder="little", signed=False)
        window_coded_ms = 0.0
        if len(payload) == 5:
            units_coded = int.from_bytes(payload[3:5], byteorder="little", signed=False)
            window_coded_ms = scan_ms_from_units(units_coded)
        return cls(
            own_address_type=(flag >> 6) & 0x03,
            filter_random_addresses=bool(flag & (1 << 5)),
            filter_private_addresses=bool(flag & (1 << 4)),
            duplicate_filter=bool(flag & (1 << 3)),
            active_scan=bool(flag & (1 << 2)),
            phy_coded=bool(flag & (1 << 1)),
            phy_1m=bool(flag & 1),
            window_ms=scan_ms_from_units(units_1m),
            window_coded_ms=window_coded_ms,
        )

    def describe(self) -> str:
        phys = []
        if self.phy_1m:
            phys.append("1M")
        if self.phy_coded:
            phys.append("Coded")
        if not phys:
            phys.append("disabled")
        effective_coded = self.window_coded_ms if self.window_coded_ms > 0.0 else self.window_ms
        window_str = f"{self.window_ms:g} ms"
        if self.phy_coded and abs(effective_coded - self.window_ms) > 0.1:
            window_str += f" / Coded {effective_coded:g} ms"
        return (
            f"PHY={'+'.join(phys)}, window={window_str}, "
            f"scan={'active' if self.active_scan else 'passive'}, "
            f"dup={'on' if self.duplicate_filter else 'off'}, "
            f"own={OWN_ADDRESS_TYPES.get(self.own_address_type, self.own_address_type)}"
        )


@dataclass
class AdvPacket:
    timestamp: float
    payload_len: int
    rssi: int
    event_type: int
    address_type: int
    phys: int
    mac: str
    payload: bytes

    @property
    def primary_phy(self) -> int:
        return self.phys & 0x0F

    @property
    def secondary_phy(self) -> int:
        return (self.phys >> 4) & 0x0F

    @property
    def is_coded(self) -> bool:
        return self.primary_phy == 3 or self.secondary_phy == 3

    @property
    def is_1m(self) -> bool:
        return self.phys == 0 or self.primary_phy == 1 or self.secondary_phy == 1

    def phy_label(self) -> str:
        return phy_label(self.phys)

    def addr_label(self) -> str:
        base = self.address_type & 0x0F
        direct = (self.address_type >> 4) & 0x0F
        label = ADDRESS_TYPE_NAMES.get(base, f"0x{base:X}")
        if direct:
            label += f" / direct 0x{direct:X}"
        return label


@dataclass
class CommandResponse:
    timestamp: float
    command: int
    index: int
    data_len: int
    data: bytes

    @property
    def command_name(self) -> str:
        return command_name(self.command)


def phy_label(value: int) -> str:
    if value == 0:
        return "Legacy/1M"
    primary = value & 0x0F
    secondary = (value >> 4) & 0x0F
    primary_name = PHY_NAMES.get(primary, f"0x{primary:X}")
    if secondary:
        secondary_name = PHY_NAMES.get(secondary, f"0x{secondary:X}")
        return f"P:{primary_name} S:{secondary_name}"
    return primary_name


def firmware_version_label(value: int) -> str:
    return f"{(value >> 4) & 0x0F}.{value & 0x0F} (0x{value:02X})"


def sdk_version_label(cert: int, structure: int, major: int, minor: int, patch: int) -> str:
    return f"{cert}.{structure}.{major}.{minor} patch {patch}"


class FrameParser:
    def __init__(self):
        self.buffer = bytearray()
        self.synced = False

    def feed(self, data: bytes):
        events = []
        if not data:
            return events
        self.buffer.extend(data)
        while len(self.buffer) >= HEAD_CRC_ADD_LEN:
            payload_len = self.buffer[0]
            total_len = payload_len + HEAD_CRC_ADD_LEN
            if payload_len > MAX_ADV_PAYLOAD:
                events.append(("crc_error", self.buffer[0]))
                del self.buffer[0]
                continue
            if len(self.buffer) < total_len:
                break
            packet = bytes(self.buffer[:total_len])
            if crc16(packet) == 0:
                events.append(self._parse_packet(packet))
                self.synced = True
                del self.buffer[:total_len]
            else:
                events.append(("crc_error", self.buffer[0]))
                del self.buffer[0]
        return events

    def _parse_packet(self, packet: bytes):
        payload_len = packet[0]
        rssi_or_command = signed_u8(packet[1])
        event_or_index = packet[2]
        address_or_len = packet[3]
        phys = packet[4]
        mac = mac_from_wire(packet[5:11])
        payload = packet[11:11 + payload_len]
        if phys == 0xFF:
            return CommandResponse(
                timestamp=time.time(),
                command=packet[1],
                index=event_or_index,
                data_len=address_or_len,
                data=packet[5:5 + address_or_len],
            )
        return AdvPacket(
            timestamp=time.time(),
            payload_len=payload_len,
            rssi=rssi_or_command,
            event_type=event_or_index,
            address_type=address_or_len,
            phys=phys,
            mac=mac,
            payload=payload,
        )


class SerialClient:
    def __init__(self, event_queue: queue.Queue):
        self.event_queue = event_queue
        self.serial = None
        self.reader = None
        self.running = threading.Event()
        self.lock = threading.Lock()
        self.parser = FrameParser()

    @property
    def is_open(self) -> bool:
        return bool(self.serial and self.serial.is_open)

    def open(self, port: str, baudrate: int, serial_activation: bool = True):
        if serial is None:
            raise RuntimeError("pyserial is required. Install it with: pip install pyserial")
        self.close()
        self.parser = FrameParser()
        # IMPORTANT (ESP32-C3): on Windows pyserial asserts DTR=1 by default at
        # port open. The C3 native USB Serial/JTAG interprets DTR=1+RTS=1 as a
        # hardware reset (rst:0x15 USB_UART_CHIP_RESET) which dumps the ROM
        # bootloader log onto the protocol stream. To avoid that we open the
        # port with `do_not_open=True`, force DTR/RTS low BEFORE the OS opens
        # the device, then open. With this sequence the initial line transitions
        # land on DTR=0/RTS=0 which the C3 maps to "release / running" — no reset.
        self.serial = serial.serial_for_url(port, do_not_open=True, baudrate=baudrate, timeout=GUI_SERIAL_TIMEOUT_S)
        try:
            self.serial.dtr = False
            self.serial.rts = False
        except (AttributeError, IOError):
            pass
        self.serial.open()
        if serial_activation:
            time.sleep(0.05)
            self.serial.setDTR(True)
            self.serial.setRTS(True)
            time.sleep(0.05)
        self.serial.setDTR(False)
        self.serial.setRTS(False)
        if hasattr(self.serial, "reset_input_buffer"):
            self.serial.reset_input_buffer()
        if hasattr(self.serial, "reset_output_buffer"):
            self.serial.reset_output_buffer()
        if serial_activation:
            # Drain ROM bootloader / firmware boot log that follows a DTR/RTS pulse-reset.
            # Without this, those ASCII bytes feed the binary parser as one CRC discard per byte.
            deadline = time.monotonic() + 0.6
            while time.monotonic() < deadline:
                try:
                    waiting = self.serial.in_waiting
                except Exception:
                    waiting = 0
                if waiting:
                    try:
                        self.serial.read(waiting)
                    except Exception:
                        break
                    deadline = time.monotonic() + 0.15
                else:
                    time.sleep(0.02)
            if hasattr(self.serial, "reset_input_buffer"):
                self.serial.reset_input_buffer()
        self.running.set()
        self.reader = threading.Thread(target=self._read_loop, name="adv2uart-reader", daemon=True)
        self.reader.start()
        self.event_queue.put(("log", f"Connected to {port} at {baudrate} baud"))

    def close(self):
        self.running.clear()
        if self.reader and self.reader.is_alive():
            self.reader.join(timeout=0.8)
        self.reader = None
        if self.serial:
            try:
                self.serial.close()
            except Exception:
                pass
        self.serial = None

    def send(self, payload: bytes):
        with self.lock:
            if not self.is_open:
                raise RuntimeError("serial port is not connected")
            framed = append_crc(payload)
            self.serial.write(framed)
            self.serial.flush()
        self.event_queue.put(("tx", bytes(payload)))

    def command_info(self):
        self.send(bytes((CMD_ID_INFO,)))

    def command_version_info(self):
        self.send(bytes((CMD_ID_VERSION,)))

    def command_vbat(self):
        self.send(bytes((CMD_ID_VBAT,)))

    def command_clear_list(self):
        self.send(bytes((CMD_ID_CLRM,)))

    def command_add_white(self, mac: str):
        self.send(bytes((CMD_ID_WMAC,)) + mac_to_wire(mac))

    def command_add_black(self, mac: str):
        self.send(bytes((CMD_ID_BMAC,)) + mac_to_wire(mac))

    def command_start_scan(self, config: ScanConfig):
        self.send(bytes((CMD_ID_SCAN,)) + config.to_payload())

    def command_stop_scan(self):
        self.send(bytes((CMD_ID_SCAN, 0, 0, 0)))

    def command_gpio_read(self, pin_id: int = BOARD_LED_PIN_ID):
        self.send(bytes((CMD_ID_GPIO, 1, pin_id & 0xFF)))

    def command_gpio_read_all(self):
        """Read all known board GPIO pins sequentially."""
        for pin_id in sorted(BOARD_PIN_NAMES.keys()):
            self.send(bytes((CMD_ID_GPIO, 1, pin_id & 0xFF)))

    def command_gpio_write(self, pin_id: int, value: int):
        self.send(bytes((CMD_ID_GPIO, 2, pin_id & 0xFF, 1 if value else 0)))

    def command_gpio_toggle(self, pin_id: int):
        self.send(bytes((CMD_ID_GPIO, 3, pin_id & 0xFF)))

    def command_gpio_analog_read(self, pin_id: int):
        self.send(bytes((CMD_ID_GPIO, 7, pin_id & 0xFF)))

    def command_gpio_config(self, pin_id: int, input_enabled: bool, output_enabled: bool, pull: int):
        self.send(
            bytes(
                (
                    CMD_ID_GPIO,
                    4,
                    pin_id & 0xFF,
                    1 if input_enabled else 0,
                    1 if output_enabled else 0,
                    pull & 0xFF,
                )
            )
        )

    def command_gpio_pwm(self, pin_id: int, duty: int, period: int = 1000):
        self.send(
            bytes(
                (
                    CMD_ID_GPIO,
                    5,
                    pin_id & 0xFF,
                    max(0, min(100, duty)) & 0xFF,
                    period & 0xFF,
                    (period >> 8) & 0xFF,
                )
            )
        )

    def command_gpio_pwm_off(self, pin_id: int):
        self.send(bytes((CMD_ID_GPIO, 6, pin_id & 0xFF)))

    def command_uart_status(self):
        self.send(bytes((CMD_ID_UART, 0)))

    def command_uart_ping(self, sequence: int, pattern: int = 0x5A):
        self.send(bytes((CMD_ID_UART, 1, sequence & 0xFF, (sequence >> 8) & 0xFF, pattern & 0xFF)))

    def command_uart_set_baud(self, index: int):
        self.send(bytes((CMD_ID_UART, 2, index & 0xFF)))

    def command_rfsdk_status(self):
        self.send(bytes((CMD_ID_RFSDK, 0)))

    def command_rf_power(self, value: int):
        self.send(bytes((CMD_ID_RFSDK, 1, value & 0xFF)))

    def command_rf_cap(self, value: int):
        self.send(bytes((CMD_ID_RFSDK, 2, value & 0xFF)))

    def command_rf_channels(self, ch0: int, ch1: int, ch2: int):
        self.send(bytes((CMD_ID_RFSDK, 3, ch0 & 0xFF, ch1 & 0xFF, ch2 & 0xFF)))

    def command_coded_min_window(self, min_window_10ms: int):
        self.send(bytes((CMD_ID_RFSDK, 4, min_window_10ms & 0xFF)))

    def command_txadv_stop(self):
        self.send(bytes((CMD_ID_TXADV, 0)))

    def command_txadv_status(self):
        # op=2 → firmware returns CMD_STATUS_OK with current TX-adv state.
        self.send(bytes((CMD_ID_TXADV, 2)))

    def command_txadv_start(self, phy: int, interval_units: int, adv_data: bytes):
        adv_len = len(adv_data)
        if adv_len > 31:
            adv_data = adv_data[:31]
            adv_len = 31
        payload = bytes((
            CMD_ID_TXADV,
            1,
            phy & 0xFF,
            interval_units & 0xFF,
            (interval_units >> 8) & 0xFF,
            adv_len,
        )) + adv_data
        self.send(payload)

    # --- CMD_ID_CONN (0x0C) ---
    def command_conn_status(self):
        """Query current connection state."""
        self.send(bytes((CMD_ID_CONN, 0)))

    def command_conn_connect(self, peer_addr: bytes, peer_addr_type: int, phy: int):
        """Initiate connection. phy: 0=1M, 1=Coded."""
        op = 2 if phy else 1
        payload = bytes((CMD_ID_CONN, op, peer_addr_type & 0xFF)) + peer_addr[:6]
        self.send(payload)

    def command_conn_disconnect(self):
        """Disconnect the current ACL connection."""
        self.send(bytes((CMD_ID_CONN, 3)))

    def command_conn_cancel(self):
        """Cancel a pending connection attempt."""
        self.send(bytes((CMD_ID_CONN, 4)))

    # --- CMD_ID_GPIOEVT (0x10) ---
    def command_gpioevt_query(self):
        """Ask the firmware which GPIO event arms are currently active."""
        self.send(bytes((CMD_ID_GPIOEVT, GPIOEVT_OP_QUERY)))

    def command_gpioevt_enable(self, pin_id: int):
        """Arm both-edge interrupt notifications for the given GPIO."""
        self.send(bytes((CMD_ID_GPIOEVT, GPIOEVT_OP_ENABLE, pin_id & 0xFF)))

    def command_gpioevt_disable(self, pin_id: int):
        """Stop event notifications for the given GPIO."""
        self.send(bytes((CMD_ID_GPIOEVT, GPIOEVT_OP_DISABLE, pin_id & 0xFF)))

    def command_gpioevt_clear(self):
        """Disarm every GPIO event source."""
        self.send(bytes((CMD_ID_GPIOEVT, GPIOEVT_OP_CLEAR)))

    # --- CMD_ID_TXDATA (0x0D) ---
    def command_txdata(self, att_handle: int, data: bytes):
        """Write ATT attribute on the connected peer (Write Without Response)."""
        data = data[:20]
        payload = bytes((
            CMD_ID_TXDATA,
            att_handle & 0xFF,
            (att_handle >> 8) & 0xFF,
            len(data),
        )) + data
        self.send(payload)

    def _read_loop(self):
        while self.running.is_set():
            try:
                data = self.serial.read(256)
            except Exception as exc:
                self.event_queue.put(("error", f"Serial read failed: {exc}"))
                self.running.clear()
                break
            for event in self.parser.feed(data):
                self.event_queue.put(("rx", event))


class ScrollableFrame(ttk.Frame):
    """ttk.Frame that hosts a vertically scrollable inner container.

    Build the actual content as children of ``self.body``. The frame wires
    mouse-wheel scrolling on enter/leave and keeps the body width matched
    to the canvas viewport so children using sticky=ew expand naturally.
    """

    def __init__(self, parent, padding=0, **kwargs):
        super().__init__(parent, padding=padding, **kwargs)
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        self._canvas = tk.Canvas(self, borderwidth=0, highlightthickness=0)
        self._canvas.grid(row=0, column=0, sticky="nsew")
        self._vbar = ttk.Scrollbar(self, orient=tk.VERTICAL, command=self._canvas.yview)
        self._vbar.grid(row=0, column=1, sticky="ns")
        self._canvas.configure(yscrollcommand=self._vbar.set)
        self.body = ttk.Frame(self._canvas)
        self._window = self._canvas.create_window((0, 0), window=self.body, anchor="nw")
        self.body.bind("<Configure>", self._on_body_configure)
        self._canvas.bind("<Configure>", self._on_canvas_configure)
        self._canvas.bind("<Enter>", self._bind_wheel)
        self._canvas.bind("<Leave>", self._unbind_wheel)

    def _on_body_configure(self, _event):
        self._canvas.configure(scrollregion=self._canvas.bbox("all"))

    def _on_canvas_configure(self, event):
        self._canvas.itemconfigure(self._window, width=event.width)

    def _bind_wheel(self, _event):
        self._canvas.bind_all("<MouseWheel>", self._on_wheel_windows_mac)
        self._canvas.bind_all("<Button-4>", lambda _e: self._canvas.yview_scroll(-3, "units"))
        self._canvas.bind_all("<Button-5>", lambda _e: self._canvas.yview_scroll(3, "units"))

    def _unbind_wheel(self, _event):
        self._canvas.unbind_all("<MouseWheel>")
        self._canvas.unbind_all("<Button-4>")
        self._canvas.unbind_all("<Button-5>")

    def _on_wheel_windows_mac(self, event):
        # Windows: event.delta multiple of 120; macOS: smaller integers
        step = -1 if event.delta > 0 else 1
        self._canvas.yview_scroll(step * 3, "units")


class StatusBar(ttk.Frame):
    """Persistent footer with labelled fields. Update via ``set(key, value)``."""

    def __init__(self, parent, fields):
        super().__init__(parent, relief=tk.SUNKEN, padding=(8, 3))
        self._vars: dict[str, tk.StringVar] = {}
        for index, (key, label, width) in enumerate(fields):
            if index > 0:
                ttk.Separator(self, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=8)
            ttk.Label(self, text=f"{label}:", foreground="gray").pack(side=tk.LEFT)
            var = tk.StringVar(value="-")
            ttk.Label(self, textvariable=var, width=width, anchor="w").pack(side=tk.LEFT, padx=(4, 0))
            self._vars[key] = var

    def set(self, key, value):
        var = self._vars.get(key)
        if var is not None:
            var.set(str(value))

    def get_var(self, key):
        return self._vars.get(key)


def _scrollable_body(parent):
    """Helper: wrap parent in a ScrollableFrame and return the inner body frame."""
    scroller = ScrollableFrame(parent)
    scroller.pack(fill=tk.BOTH, expand=True)
    return scroller.body


class AdvBle2UartGui(tk.Tk):
    def __init__(self, initial_port: str | None = None, start_maximized: bool = False, auto_connect: bool = False):
        super().__init__()
        self.title(f"ADV_BLE2UART Test Console - {DEVICE_LABEL}")
        self.geometry("1180x760")
        self.minsize(880, 560)
        self.status_bar = None  # populated in _build_ui
        self.initial_port = (initial_port or "").strip()
        self.start_maximized = start_maximized
        self.auto_connect = auto_connect

        self.events = queue.Queue()
        self.client = SerialClient(self.events)

        self.white_list = []
        self.black_list = []
        self.advertisements = []
        self.stats_by_mac = {}
        self.total_count = 0
        self.legacy_count = 0
        self.one_m_count = 0
        self.coded_count = 0
        self.last_packet_time = None

        # CRC-discard coalescing state — keep parser noise out of the log.
        self._crc_discard_buf = bytearray()
        self._crc_discard_flush_after = None

        # Scan response watchdog: if the firmware does not reply within 2.5 s
        # of a Start/Stop scan command, we force-resync the buttons so the
        # user is never locked out by a missed response or a chip reset.
        self._scan_response_watchdog = None

        self._build_variables()
        self._build_ui()
        self.refresh_ports()
        self.apply_long_range_preset()
        self.after_idle(self.apply_startup_window_mode)
        if self.auto_connect:
            self.after(150, self.connect)
        self.after(50, self.process_events)
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def _build_variables(self):
        self.port_var = tk.StringVar(value=self.initial_port)
        self.baud_var = tk.StringVar(value="2000000")
        self.status_var = tk.StringVar(value="Disconnected")
        self.local_mac_var = tk.StringVar(value="-")
        self.version_var = tk.StringVar(value="-")
        self.hw_version_var = tk.StringVar(value="-")
        self.sdk_version_var = tk.StringVar(value="-")
        self.vbat_var = tk.StringVar(value="Unsupported" if not DEVICE_SUPPORTS_VBAT else "-")
        self.temp_var = tk.StringVar(value="-")
        self.list_capacity_var = tk.StringVar(value="-")
        self.scan_state_var = tk.StringVar(value="Stopped")

        self.pulse_reset_var = tk.BooleanVar(value=DEVICE_LEGACY_RESET_DEFAULT)
        self.own_addr_var = tk.StringVar(value=OWN_ADDRESS_TYPES[0])
        self.phy_1m_var = tk.BooleanVar(value=True)
        self.phy_coded_var = tk.BooleanVar(value=True)
        self.active_scan_var = tk.BooleanVar(value=False)
        self.duplicate_filter_var = tk.BooleanVar(value=False)
        self.filter_random_var = tk.BooleanVar(value=False)
        self.filter_private_var = tk.BooleanVar(value=False)
        self.window_ms_var = tk.DoubleVar(value=30.0)
        self.window_coded_ms_var = tk.DoubleVar(value=0.0)  # 0 = same as 1M window
        self.reapply_lists_var = tk.BooleanVar(value=True)

        self.mac_entry_var = tk.StringVar(value="A4C1383406CE")
        self.monitor_filter_var = tk.StringVar(value="")
        self.pause_table_var = tk.BooleanVar(value=False)
        self.autoscroll_var = tk.BooleanVar(value=True)

        self.total_var = tk.StringVar(value="0")
        self.legacy_var = tk.StringVar(value="0")
        self.one_m_var = tk.StringVar(value="0")
        self.coded_var = tk.StringVar(value="0")
        self.last_seen_var = tk.StringVar(value="-")

        self.led_state_var = tk.StringVar(value="-")
        self.board_mask_var = tk.StringVar(value="-")
        self.key_state_var = tk.StringVar(value="-")
        self.gpio_pin_var = tk.StringVar(value=DEVICE_DEFAULT_GPIO_PIN)
        self.analog_pin_var = tk.StringVar(value=default_analog_pin_name())
        self.analog_value_var = tk.StringVar(value="-")
        self.gpio_value_var = tk.BooleanVar(value=True)
        self.gpio_input_var = tk.BooleanVar(value=True)
        self.gpio_output_var = tk.BooleanVar(value=True)
        self.gpio_pull_var = tk.StringVar(value="Float")
        self.gpio_pwm_duty_var = tk.IntVar(value=50)    # kept for method compatibility
        self.gpio_pwm_period_var = tk.IntVar(value=1000)
        self.gpio_pin_state_vars = {pin_id: tk.StringVar(value="?") for pin_id in BOARD_PIN_NAMES}
        self.gpio_read_generation = 0
        self.pending_gpio_read_ids = []
        self.pending_gpio_read_pin = None
        self.led_blink_count_var = tk.IntVar(value=3)
        self.led_blink_delay_var = tk.IntVar(value=50)
        self.led_blink_generation = 0

        self.uart_status_var = tk.StringVar(value="-")
        self.uart_baud_index_var = tk.StringVar(value="0 - 2000000")
        self.uart_ping_seq_var = tk.IntVar(value=1)
        self.pending_gui_baud = None

        self.rf_power_var = tk.StringVar(value=DEVICE_DEFAULT_RF_POWER_LABEL)
        self.rf_cap_var = tk.IntVar(value=0)
        self.rf_cap_status_var = tk.StringVar(value="-" if RF_CAP_SUPPORTED else f"unsupported on {DEVICE_LABEL}")
        self.rf_ch0_var = tk.IntVar(value=37)
        self.rf_ch1_var = tk.IntVar(value=38)
        self.rf_ch2_var = tk.IntVar(value=39)
        self.coded_min_ms_var = tk.IntVar(value=100)
        self.rf_status_var = tk.StringVar(value="-")

        self.txadv_phy_var = tk.StringVar(value=TXADV_PHY_OPTIONS[0][0])
        self.txadv_interval_ms_var = tk.DoubleVar(value=100.0)
        self.txadv_data_var = tk.StringVar(value="02 01 06 0A 08 54 42 30 33 46 2D 4B 49 54")
        self.txadv_status_var = tk.StringVar(value="-")

        # BLE Connection (CMD_ID_CONN/TXDATA/RXDATA)
        self.conn_state_var = tk.StringVar(value="Idle")
        self.conn_last_result_var = tk.StringVar(value="-")
        self.conn_peer_var = tk.StringVar(value="")
        # Connection timeout (seconds) — host-side. The BLE controller has its
        # own connect timeout (~30 s); this UI-side timer fires earlier so we
        # can auto-cancel and surface the result.
        self.conn_timeout_s_var = tk.IntVar(value=10)
        self._conn_timeout_after = None
        self.conn_peer_type_var = tk.StringVar(value="Random")
        self.conn_phy_var = tk.StringVar(value=CONN_PHY_OPTIONS[0][0])
        self.conn_interval_var = tk.StringVar(value="-")
        self.conn_handle_var = tk.StringVar(value="-")
        self.txdata_handle_var = tk.StringVar(value="0012")
        self.txdata_data_var = tk.StringVar(value="48 65 6C 6C 6F")
        self.conn_rxdata_log = None  # set when tab is built
        self.last_started_scan_config = None

        # GPIO event subsystem (CMD_ID_GPIOEVT)
        self.gpioevt_pin_var = tk.StringVar(value="BOOT / GPIO9" if "BOOT / GPIO9" in BOARD_PINS else DEVICE_DEFAULT_GPIO_PIN)
        self.gpioevt_enabled_mask_var = tk.StringVar(value="0x00000000")
        self.gpioevt_log = None  # tk.Text, set when tab is built

    def _build_ui(self):
        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)

        self._build_toolbar()

        main = ttk.Panedwindow(self, orient=tk.HORIZONTAL)
        main.grid(row=1, column=0, sticky="nsew", padx=6, pady=(0, 0))

        left = ttk.Notebook(main)
        right = ttk.Frame(main)
        main.add(left, weight=0)
        main.add(right, weight=1)

        right.columnconfigure(0, weight=1)
        right.rowconfigure(0, weight=1)

        scan_tab = ttk.Frame(left)
        mac_tab = ttk.Frame(left)
        counters_tab = ttk.Frame(left)
        for tab in (scan_tab, mac_tab, counters_tab):
            tab.columnconfigure(0, weight=1)
            tab.rowconfigure(0, weight=1)
        left.add(scan_tab, text="Scan")
        left.add(mac_tab, text="MAC")
        left.add(counters_tab, text="Stats")

        self._build_scan_controls(_scrollable_body(scan_tab))
        self._build_mac_controls(mac_tab)
        self._build_counters(counters_tab)
        self._build_monitor(right)

        # Bottom status bar — replaces the old verbose Device labels frame.
        self.status_bar = StatusBar(
            self,
            fields=(
                ("device", "Device", 14),
                ("port", "Port", 14),
                ("conn", "Status", 16),
                ("mac", "MAC", 14),
                ("fw", "FW", 8),
                ("hw", "HW", 18),
                # Scan needs room for the full config description, e.g.
                # "PHY=1M+Coded, window=30 ms / Coded 150 ms, scan=active, dup=on, own=PUBLIC"
                ("scan", "Scan", 70),
                ("vbat", "Supply", 12),
            ),
        )
        self.status_bar.grid(row=2, column=0, sticky="ew")
        self.status_bar.set("device", DEVICE_LABEL)
        self.status_bar.set("port", self.port_var.get() or "-")
        self.status_bar.set("conn", self.status_var.get())
        self.status_bar.set("scan", self.scan_state_var.get())
        self._wire_status_bar()

    def _wire_status_bar(self):
        """Mirror existing StringVars into the bottom status bar via traces."""
        pairs = (
            ("port", self.port_var),
            ("conn", self.status_var),
            ("mac", self.local_mac_var),
            ("fw", self.version_var),
            ("hw", self.hw_version_var),
            ("scan", self.scan_state_var),
            ("vbat", self.vbat_var),
        )
        for key, var in pairs:
            var.trace_add("write", lambda *_a, k=key, v=var: self.status_bar.set(k, v.get()))

    def _enhance_treeview(self, tree: "ttk.Treeview", numeric_cols: tuple[str, ...] = ()):
        """Add header-click sorting and a right-click 'Copy' context menu.

        - Each column header toggles ascending/descending sort on click.
        - Right-click on a row opens a menu to copy the cell under the cursor,
          the whole row, or the entire table.
        - Numeric columns (passed in ``numeric_cols``) are sorted as numbers
          when possible, falling back to string compare on parse failure.
        """
        # --- Sortable headers ---
        sort_state: dict[str, bool] = {}

        def sort_by(col: str):
            ascending = not sort_state.get(col, False)
            sort_state[col] = ascending
            try:
                items = [(tree.set(iid, col), iid) for iid in tree.get_children("")]
            except tk.TclError:
                return
            if col in numeric_cols:
                def keyfn(pair):
                    raw = pair[0]
                    try:
                        return (0, float(raw))
                    except (TypeError, ValueError):
                        return (1, str(raw))
                items.sort(key=keyfn, reverse=not ascending)
            else:
                items.sort(key=lambda pair: str(pair[0]).casefold(), reverse=not ascending)
            for index, (_value, iid) in enumerate(items):
                tree.move(iid, "", index)
            # Visual hint on the header.
            for c in tree["columns"]:
                tree.heading(c, text=tree.heading(c, "text").rstrip(" ▲▼"))
            arrow = " ▲" if ascending else " ▼"
            tree.heading(col, text=tree.heading(col, "text") + arrow)

        for col in tree["columns"]:
            current = tree.heading(col, "text")
            tree.heading(col, text=current, command=lambda c=col: sort_by(c))

        # --- Right-click context menu ---
        menu = tk.Menu(tree, tearoff=0)

        def column_under(event):
            col_id = tree.identify_column(event.x)
            if not col_id or not col_id.startswith("#"):
                return None
            try:
                idx = int(col_id[1:]) - 1
            except ValueError:
                return None
            cols = tree["columns"]
            if 0 <= idx < len(cols):
                return cols[idx]
            return None

        def copy_cell(iid, col):
            if not iid or not col:
                return
            value = tree.set(iid, col)
            self.clipboard_clear()
            self.clipboard_append(str(value))

        def copy_row(iid):
            if not iid:
                return
            cols = tree["columns"]
            values = [tree.set(iid, c) for c in cols]
            self.clipboard_clear()
            self.clipboard_append("\t".join(str(v) for v in values))

        def copy_all():
            cols = tree["columns"]
            lines = ["\t".join(cols)]
            for iid in tree.get_children(""):
                lines.append("\t".join(str(tree.set(iid, c)) for c in cols))
            self.clipboard_clear()
            self.clipboard_append("\n".join(lines))

        def copy_mac(iid):
            """Copy just the MAC value from a row that has a 'mac' column."""
            if not iid or "mac" not in tree["columns"]:
                return
            value = tree.set(iid, "mac")
            self.clipboard_clear()
            self.clipboard_append(str(value))

        def copy_advertisement(iid):
            if not iid or "payload" not in tree["columns"]:
                return
            value = tree.set(iid, "payload")
            if not value:
                return
            self.clipboard_clear()
            self.clipboard_append(str(value))

        def on_right_click(event):
            iid = tree.identify_row(event.y)
            col = column_under(event)
            if iid:
                tree.selection_set(iid)
                tree.focus(iid)
            menu.delete(0, tk.END)
            if iid and col:
                menu.add_command(label=f"Copy cell  ({col})", command=lambda: copy_cell(iid, col))
            if iid:
                menu.add_command(label="Copy row", command=lambda: copy_row(iid))
            # Copy MAC: shown only when the row has a 'mac' column and it's not empty.
            if iid and "mac" in tree["columns"]:
                mac_val = tree.set(iid, "mac")
                if mac_val:
                    menu.add_command(label=f"Copy MAC  ({mac_val})", command=lambda: copy_mac(iid))
            if iid and "payload" in tree["columns"]:
                payload_val = tree.set(iid, "payload")
                if payload_val:
                    menu.add_command(label="Copy Advertisement", command=lambda: copy_advertisement(iid))
            if iid:
                menu.add_separator()
            menu.add_command(label="Copy all rows", command=copy_all)
            try:
                menu.tk_popup(event.x_root, event.y_root)
            finally:
                menu.grab_release()

        # Bind both X11 (Button-3) and macOS (Button-2 / control-click).
        tree.bind("<Button-3>", on_right_click, add="+")
        tree.bind("<Button-2>", on_right_click, add="+")
        tree.bind("<Control-Button-1>", on_right_click, add="+")
        # Ctrl-C on focused row copies the row.
        tree.bind("<Control-c>", lambda _e: copy_row(tree.focus()), add="+")

    def _build_toolbar(self):
        bar = ttk.Frame(self, padding=(8, 6))
        bar.grid(row=0, column=0, sticky="ew")
        bar.columnconfigure(99, weight=1)

        ttk.Label(bar, text="Port").grid(row=0, column=0, padx=(0, 4))
        self.port_combo = ttk.Combobox(bar, textvariable=self.port_var, width=14)
        self.port_combo.grid(row=0, column=1, padx=(0, 2))
        ttk.Button(bar, text="↻", width=2, command=self.refresh_ports).grid(row=0, column=2, padx=(0, 8))

        ttk.Label(bar, text="Baud").grid(row=0, column=3, padx=(0, 4))
        self.baud_combo = ttk.Combobox(
            bar,
            textvariable=self.baud_var,
            values=("2000000", "1500000", "921600", "460800", "230400", "115200"),
            width=9,
        )
        self.baud_combo.grid(row=0, column=4, padx=(0, 8))

        ttk.Checkbutton(bar, text="DTR/RTS reset", variable=self.pulse_reset_var).grid(row=0, column=5, padx=(0, 8))

        self.connect_button = ttk.Button(bar, text="Connect", command=self.connect)
        self.connect_button.grid(row=0, column=6, padx=(0, 2))
        self.disconnect_button = ttk.Button(bar, text="Disconnect", command=self.disconnect)
        self.disconnect_button.grid(row=0, column=7, padx=(0, 12))
        # Initial state: not connected
        self.disconnect_button.state(["disabled"])

        ttk.Separator(bar, orient=tk.VERTICAL).grid(row=0, column=8, sticky="ns", padx=(0, 8))

        ttk.Button(bar, text="Info", command=self.send_info, width=7).grid(row=0, column=9, padx=(0, 2))
        ttk.Button(bar, text="Version", command=self.send_version_info, width=8).grid(row=0, column=10, padx=(0, 2))
        ttk.Button(bar, text="Clear MAC", command=self.clear_mac_list, width=12).grid(row=0, column=11, padx=(0, 2))
        self.vbat_button = ttk.Button(bar, text="VBAT", command=self.send_vbat, width=6)
        self.vbat_button.grid(row=0, column=12, padx=(0, 2))
        if not DEVICE_SUPPORTS_VBAT:
            self.vbat_button.state(["disabled"])

        # spacer column 99 stretches

    def _build_scan_controls(self, parent):
        frame = ttk.LabelFrame(parent, text="Scan Configuration")
        frame.grid(row=0, column=0, sticky="ew", padx=(0, 8), pady=(0, 8))
        frame.columnconfigure(1, weight=1)

        ttk.Label(frame, text="Own address").grid(row=0, column=0, sticky="w", padx=8, pady=(8, 4))
        ttk.Combobox(
            frame,
            textvariable=self.own_addr_var,
            values=tuple(OWN_ADDRESS_TYPES.values()),
            state="readonly",
            width=24,
        ).grid(row=0, column=1, sticky="ew", padx=8, pady=(8, 4))

        ttk.Label(frame, text="PHY").grid(row=1, column=0, sticky="w", padx=8, pady=4)
        phy_frame = ttk.Frame(frame)
        phy_frame.grid(row=1, column=1, sticky="ew", padx=8, pady=4)
        ttk.Checkbutton(phy_frame, text="1M", variable=self.phy_1m_var).pack(side=tk.LEFT)
        ttk.Checkbutton(phy_frame, text="Coded", variable=self.phy_coded_var).pack(side=tk.LEFT, padx=(12, 0))

        ttk.Label(frame, text="1M window ms").grid(row=2, column=0, sticky="w", padx=8, pady=4)
        ttk.Spinbox(frame, textvariable=self.window_ms_var, from_=10, to=1000, increment=10, width=12).grid(
            row=2, column=1, sticky="w", padx=8, pady=4
        )

        ttk.Label(frame, text="Coded window ms").grid(row=3, column=0, sticky="w", padx=8, pady=4)
        coded_frame = ttk.Frame(frame)
        coded_frame.grid(row=3, column=1, sticky="w", padx=8, pady=4)
        ttk.Spinbox(coded_frame, textvariable=self.window_coded_ms_var, from_=0, to=10000, increment=50, width=12).pack(side=tk.LEFT)
        ttk.Label(coded_frame, text="(0 = same as 1M)", foreground="gray").pack(side=tk.LEFT, padx=(6, 0))

        options = ttk.Frame(frame)
        options.grid(row=4, column=0, columnspan=2, sticky="ew", padx=8, pady=4)
        ttk.Checkbutton(options, text="Active scan", variable=self.active_scan_var).grid(row=0, column=0, sticky="w")
        ttk.Checkbutton(options, text="Duplicate filter", variable=self.duplicate_filter_var).grid(row=0, column=1, sticky="w", padx=(16, 0))
        ttk.Checkbutton(options, text="Filter random addresses", variable=self.filter_random_var).grid(row=1, column=0, sticky="w", pady=(4, 0))
        ttk.Checkbutton(options, text="Filter private/identity addresses", variable=self.filter_private_var).grid(row=1, column=1, sticky="w", padx=(16, 0), pady=(4, 0))

        presets = ttk.Frame(frame)
        presets.grid(row=5, column=0, columnspan=2, sticky="ew", padx=8, pady=(8, 4))
        ttk.Button(presets, text="Long Range Coded", command=self.apply_long_range_preset).pack(side=tk.LEFT)
        ttk.Button(presets, text="Mixed Fast", command=self.apply_mixed_preset).pack(side=tk.LEFT, padx=6)
        ttk.Button(presets, text="1M Only", command=self.apply_1m_preset).pack(side=tk.LEFT)

        actions = ttk.Frame(frame)
        actions.grid(row=6, column=0, columnspan=2, sticky="ew", padx=8, pady=(4, 8))
        ttk.Checkbutton(actions, text="Reapply lists", variable=self.reapply_lists_var).pack(side=tk.LEFT)
        self.start_scan_button = ttk.Button(actions, text="Start Scan", command=self.start_scan)
        self.start_scan_button.pack(side=tk.LEFT, padx=(12, 4))
        self.stop_scan_button = ttk.Button(actions, text="Stop Scan", command=self.stop_scan)
        self.stop_scan_button.pack(side=tk.LEFT, padx=4)
        # Bind buttons to scan state: Start enabled only when scan is Stopped,
        # Stop enabled only when scan is running.
        self.scan_state_var.trace_add("write", lambda *_: self._sync_scan_buttons())
        self._sync_scan_buttons()

    def _build_mac_controls(self, parent):
        frame = ttk.LabelFrame(parent, text="Firmware MAC List")
        frame.grid(row=0, column=0, sticky="nsew", padx=6, pady=6)
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(2, weight=1)

        entry = ttk.Entry(frame, textvariable=self.mac_entry_var)
        entry.grid(row=0, column=0, columnspan=3, sticky="ew", padx=8, pady=(8, 4))

        ttk.Button(frame, text="Add White", command=self.add_white_mac).grid(row=1, column=0, sticky="ew", padx=(8, 4), pady=4)
        ttk.Button(frame, text="Add Black", command=self.add_black_mac).grid(row=1, column=1, sticky="ew", padx=4, pady=4)
        ttk.Button(frame, text="Remove Local", command=self.remove_selected_mac).grid(row=1, column=2, sticky="ew", padx=(4, 8), pady=4)

        self.mac_tree = ttk.Treeview(frame, columns=("mode", "mac"), show="headings", height=7)
        self.mac_tree.heading("mode", text="Mode")
        self.mac_tree.heading("mac", text="MAC")
        self.mac_tree.column("mode", width=70, anchor=tk.CENTER)
        self.mac_tree.column("mac", width=150, anchor=tk.CENTER)
        self.mac_tree.grid(row=2, column=0, columnspan=3, sticky="nsew", padx=8, pady=4)
        self._enhance_treeview(self.mac_tree)

        ttk.Button(frame, text="Clear Firmware and Local Lists", command=self.clear_mac_list).grid(
            row=3, column=0, columnspan=3, sticky="ew", padx=8, pady=(4, 8)
        )

    def _build_counters(self, parent):
        frame = ttk.LabelFrame(parent, text="Counters")
        frame.grid(row=0, column=0, sticky="ew", padx=6, pady=6)
        rows = (
            ("Total", self.total_var),
            ("Legacy/1M", self.legacy_var),
            ("1M ext", self.one_m_var),
            ("Coded", self.coded_var),
            ("Last seen", self.last_seen_var),
        )
        for index, (label, variable) in enumerate(rows):
            ttk.Label(frame, text=label).grid(row=index, column=0, sticky="w", padx=8, pady=2)
            ttk.Label(frame, textvariable=variable).grid(row=index, column=1, sticky="e", padx=8, pady=2)
        ttk.Button(frame, text="Reset Counters", command=self.reset_monitor).grid(row=len(rows), column=0, columnspan=2, sticky="ew", padx=8, pady=(8, 8))

    def _build_monitor(self, parent):
        notebook = ttk.Notebook(parent)
        notebook.grid(row=0, column=0, sticky="nsew")

        adv_tab = ttk.Frame(notebook)
        stats_tab = ttk.Frame(notebook)
        gpio_tab = ttk.Frame(notebook)
        rf_tab = ttk.Frame(notebook)
        uart_tab = ttk.Frame(notebook)
        txadv_tab = ttk.Frame(notebook)
        conn_tab = ttk.Frame(notebook)
        log_tab = ttk.Frame(notebook)
        notebook.add(adv_tab, text="Advertisements")
        notebook.add(stats_tab, text="MAC Stats")
        notebook.add(gpio_tab, text="GPIO")
        notebook.add(rf_tab, text="RF")
        # Label is profile-aware: ESP32-C3 uses native USB Serial/JTAG, TB-03F-KIT uses CH340 UART.
        notebook.add(uart_tab, text="Console")
        if DEVICE_SUPPORTS_TXADV:
            notebook.add(txadv_tab, text="TX Adv")
        if DEVICE_SUPPORTS_CONN:
            notebook.add(conn_tab, text="BLE Conn")
        notebook.add(log_tab, text="Log")

        # Tabs containing fixed tables/log keep raw parent (they fill naturally).
        # Tabs that are forms get a scrollable inner body for small-display safety.
        self._build_adv_tab(adv_tab)
        self._build_stats_tab(stats_tab)
        self._build_gpio_tab(gpio_tab)
        self._build_rf_tab(_scrollable_body(rf_tab))
        self._build_uart_tab(_scrollable_body(uart_tab))
        if DEVICE_SUPPORTS_TXADV:
            self._build_txadv_tab(_scrollable_body(txadv_tab))
        if DEVICE_SUPPORTS_CONN:
            self._build_conn_tab(conn_tab)
        self._build_log_tab(log_tab)

    def _build_adv_tab(self, tab):
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(tab)
        toolbar.grid(row=0, column=0, sticky="ew", padx=6, pady=6)
        ttk.Label(toolbar, text="Display filter MAC/payload").pack(side=tk.LEFT)
        ttk.Entry(toolbar, textvariable=self.monitor_filter_var, width=24).pack(side=tk.LEFT, padx=6)
        ttk.Checkbutton(toolbar, text="Pause", variable=self.pause_table_var).pack(side=tk.LEFT, padx=6)
        ttk.Checkbutton(toolbar, text="Autoscroll", variable=self.autoscroll_var).pack(side=tk.LEFT, padx=6)
        ttk.Button(toolbar, text="Clear", command=self.clear_adv_table).pack(side=tk.LEFT, padx=6)
        ttk.Button(toolbar, text="Export CSV", command=self.export_csv).pack(side=tk.LEFT, padx=6)

        columns = ("time", "rssi", "event", "addr", "phy", "mac", "len", "payload", "ascii")
        self.adv_tree = ttk.Treeview(tab, columns=columns, show="headings")
        headings = {
            "time": "Time",
            "rssi": "RSSI",
            "event": "Event",
            "addr": "Addr type",
            "phy": "PHY",
            "mac": "MAC",
            "len": "Len",
            "payload": "Payload hex",
            "ascii": "ASCII",
        }
        widths = {
            "time": 90,
            "rssi": 60,
            "event": 70,
            "addr": 150,
            "phy": 120,
            "mac": 130,
            "len": 50,
            "payload": 360,
            "ascii": 180,
        }
        for name in columns:
            self.adv_tree.heading(name, text=headings[name])
            self.adv_tree.column(name, width=widths[name], anchor=tk.CENTER if name != "payload" else tk.W)
        yscroll = ttk.Scrollbar(tab, orient=tk.VERTICAL, command=self.adv_tree.yview)
        xscroll = ttk.Scrollbar(tab, orient=tk.HORIZONTAL, command=self.adv_tree.xview)
        self.adv_tree.configure(yscrollcommand=yscroll.set, xscrollcommand=xscroll.set)
        self.adv_tree.grid(row=1, column=0, sticky="nsew", padx=(6, 0), pady=(0, 6))
        yscroll.grid(row=1, column=1, sticky="ns", pady=(0, 6))
        xscroll.grid(row=2, column=0, sticky="ew", padx=(6, 0), pady=(0, 6))
        self._enhance_treeview(self.adv_tree, numeric_cols=("rssi", "len"))

    def _build_stats_tab(self, tab):
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(0, weight=1)
        columns = ("mac", "count", "last_rssi", "min_rssi", "max_rssi", "last_phy", "last_seen")
        self.stats_tree = ttk.Treeview(tab, columns=columns, show="headings")
        headings = {
            "mac": "MAC",
            "count": "Count",
            "last_rssi": "Last RSSI",
            "min_rssi": "Min RSSI",
            "max_rssi": "Max RSSI",
            "last_phy": "Last PHY",
            "last_seen": "Last seen",
        }
        for name in columns:
            self.stats_tree.heading(name, text=headings[name])
            self.stats_tree.column(name, width=130, anchor=tk.CENTER)
        self.stats_tree.grid(row=0, column=0, sticky="nsew", padx=6, pady=6)
        self._enhance_treeview(self.stats_tree, numeric_cols=("count", "last_rssi", "min_rssi", "max_rssi"))

    def _build_gpio_tab(self, tab):
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(0, weight=1)

        inner = ttk.Notebook(tab)
        inner.grid(row=0, column=0, sticky="nsew", padx=4, pady=4)

        led_tab = ttk.Frame(inner)
        pinio_tab = ttk.Frame(inner)
        events_tab = ttk.Frame(inner)
        states_tab = ttk.Frame(inner)
        inner.add(led_tab, text="LED & Status")
        inner.add(pinio_tab, text="Pin I/O")
        inner.add(
            events_tab,
            text="Events",
            state=tk.NORMAL if DEVICE_SUPPORTS_GPIO_EVENTS else tk.DISABLED,
        )
        if DEVICE_SUPPORTS_ANALOG:
            analog_tab = ttk.Frame(inner)
            inner.add(analog_tab, text="Analog")
        inner.add(states_tab, text="Pin states")

        self._build_gpio_led_tab(_scrollable_body(led_tab))
        self._build_gpio_pinio_tab(_scrollable_body(pinio_tab))
        if DEVICE_SUPPORTS_GPIO_EVENTS:
            self._build_gpio_events_tab(events_tab)  # contains its own scroll for the log
        if DEVICE_SUPPORTS_ANALOG:
            self._build_gpio_analog_tab(_scrollable_body(analog_tab))
        self._build_gpio_states_tab(_scrollable_body(states_tab))

    def _build_gpio_led_tab(self, parent):
        parent.columnconfigure(0, weight=1)

        leds_frame = ttk.LabelFrame(parent, text=DEVICE_BOARD_FRAME_TITLE)
        leds_frame.grid(row=0, column=0, sticky="ew", padx=6, pady=6)
        leds_frame.columnconfigure(0, weight=1)

        direct_leds = ttk.Frame(leds_frame)
        direct_leds.grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 4))
        direct_leds.columnconfigure(0, weight=1)
        for row, (name, bit) in enumerate(LED_BITS.items()):
            ttk.Label(direct_leds, text=name).grid(row=row, column=0, sticky="w", pady=2)
            ttk.Button(direct_leds, text="On", width=6, command=lambda value=bit: self.led_set_mask(value, True)).grid(row=row, column=1, padx=2, pady=2)
            ttk.Button(direct_leds, text="Off", width=6, command=lambda value=bit: self.led_set_mask(value, False)).grid(row=row, column=2, padx=2, pady=2)
            ttk.Button(direct_leds, text="Toggle", width=7, command=lambda value=bit: self.led_toggle_mask(value)).grid(row=row, column=3, padx=2, pady=2)
            ttk.Button(direct_leds, text="Blink", width=6, command=lambda value=bit: self.led_blink_mask(value)).grid(row=row, column=4, padx=2, pady=2)

        ttk.Label(
            leds_frame,
            text=DEVICE_BOARD_HELP_TEXT,
            foreground="gray",
            wraplength=560,
            justify="left",
        ).grid(row=1, column=0, sticky="w", padx=8, pady=(0, 4))

        blink_frame = ttk.Frame(leds_frame)
        blink_frame.grid(row=2, column=0, sticky="ew", padx=8, pady=(4, 8))
        ttk.Label(blink_frame, text="Blink count").pack(side=tk.LEFT)
        ttk.Spinbox(blink_frame, textvariable=self.led_blink_count_var, from_=1, to=5, width=4).pack(side=tk.LEFT, padx=(4, 10))
        ttk.Label(blink_frame, text="delay ms").pack(side=tk.LEFT)
        ttk.Spinbox(blink_frame, textvariable=self.led_blink_delay_var, from_=10, to=200, increment=10, width=5).pack(side=tk.LEFT, padx=(4, 10))
        ttk.Button(blink_frame, text=DEVICE_DIRECT_BLINK_LABEL, command=lambda: self.led_blink_mask(LED_ALL_MASK)).pack(side=tk.LEFT)
        ttk.Button(blink_frame, text="All off", command=self.led_all_off).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Button(blink_frame, text="Query state", command=self.led_query_state).pack(side=tk.LEFT, padx=(6, 0))

        status_frame = ttk.LabelFrame(parent, text="Status pins")
        status_frame.grid(row=1, column=0, sticky="ew", padx=6, pady=(0, 6))
        status_frame.columnconfigure(5, weight=1)
        ttk.Label(status_frame, text=DEVICE_STATUS_PIN_LABEL).grid(row=0, column=0, sticky="w", padx=(8, 4), pady=8)
        ttk.Label(status_frame, textvariable=self.key_state_var, width=12).grid(row=0, column=1, sticky="w", padx=(4, 14), pady=8)
        ttk.Label(status_frame, text=DEVICE_LED_STATUS_LABEL).grid(row=0, column=2, sticky="w", padx=(0, 4), pady=8)
        ttk.Label(status_frame, textvariable=self.led_state_var, width=8).grid(row=0, column=3, sticky="w", padx=(4, 14), pady=8)
        ttk.Label(status_frame, text=DEVICE_BOARD_MASK_LABEL).grid(row=0, column=4, sticky="w", padx=(0, 4), pady=8)
        ttk.Label(status_frame, textvariable=self.board_mask_var, width=8).grid(row=0, column=5, sticky="w", padx=(4, 8), pady=8)

    def _build_gpio_pinio_tab(self, parent):
        parent.columnconfigure(0, weight=1)

        pin_frame = ttk.LabelFrame(parent, text="Pin selection")
        pin_frame.grid(row=0, column=0, sticky="ew", padx=6, pady=6)
        pin_frame.columnconfigure(1, weight=1)

        ttk.Label(pin_frame, text="GPIO pin").grid(row=0, column=0, sticky="w", padx=8, pady=(8, 4))
        ttk.Combobox(
            pin_frame,
            textvariable=self.gpio_pin_var,
            values=tuple(BOARD_PINS.keys()),
            state="readonly",
            width=22,
        ).grid(row=0, column=1, sticky="ew", padx=8, pady=(8, 4))
        read_frame = ttk.Frame(pin_frame)
        read_frame.grid(row=0, column=2, sticky="e", padx=(0, 8), pady=(8, 4))
        ttk.Button(read_frame, text="Read", command=self.gpio_read_selected).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(read_frame, text="Read all", command=self.gpio_read_all).pack(side=tk.LEFT)

        write_frame = ttk.LabelFrame(parent, text="Write / Toggle")
        write_frame.grid(row=1, column=0, sticky="ew", padx=6, pady=6)
        write_frame.columnconfigure(1, weight=1)
        ttk.Checkbutton(write_frame, text="Output high", variable=self.gpio_value_var).grid(
            row=0, column=0, sticky="w", padx=8, pady=(8, 8)
        )
        ttk.Button(write_frame, text="Write pin", command=self.gpio_write_selected, width=14).grid(
            row=0, column=1, sticky="w", padx=8, pady=(8, 8)
        )
        ttk.Button(write_frame, text="Toggle pin", command=self.gpio_toggle_selected, width=14).grid(
            row=0, column=2, sticky="w", padx=(0, 8), pady=(8, 8)
        )

        cfg_frame = ttk.LabelFrame(parent, text="Configure")
        cfg_frame.grid(row=2, column=0, sticky="ew", padx=6, pady=6)
        cfg_inner = ttk.Frame(cfg_frame)
        cfg_inner.pack(fill=tk.X, padx=8, pady=8)
        ttk.Checkbutton(cfg_inner, text="Input", variable=self.gpio_input_var).pack(side=tk.LEFT)
        ttk.Checkbutton(cfg_inner, text="Output", variable=self.gpio_output_var).pack(side=tk.LEFT, padx=(10, 0))
        ttk.Label(cfg_inner, text="Pull").pack(side=tk.LEFT, padx=(14, 4))
        ttk.Combobox(cfg_inner, textvariable=self.gpio_pull_var, values=tuple(GPIO_PULLS.keys()), state="readonly", width=14).pack(side=tk.LEFT)
        ttk.Button(cfg_inner, text="Configure", command=self.gpio_config_selected).pack(side=tk.LEFT, padx=(14, 0))

    def _build_gpio_analog_tab(self, parent):
        parent.columnconfigure(0, weight=1)

        analog_frame = ttk.LabelFrame(parent, text="Analog input (ADC1, 12-bit raw)")
        analog_frame.grid(row=0, column=0, sticky="ew", padx=6, pady=6)
        analog_frame.columnconfigure(1, weight=1)

        ttk.Label(analog_frame, text="Pin").grid(row=0, column=0, sticky="w", padx=8, pady=(8, 4))
        ttk.Combobox(
            analog_frame,
            textvariable=self.analog_pin_var,
            values=analog_capable_pin_names(),
            state="readonly",
            width=12,
        ).grid(row=0, column=1, sticky="w", padx=8, pady=(8, 4))
        ttk.Button(analog_frame, text="Read analog", command=self.analog_read_selected).grid(
            row=0, column=2, sticky="e", padx=(0, 8), pady=(8, 4)
        )
        ttk.Label(analog_frame, text="Raw").grid(row=1, column=0, sticky="w", padx=8, pady=(0, 8))
        ttk.Label(analog_frame, textvariable=self.analog_value_var, width=10).grid(
            row=1, column=1, sticky="w", padx=8, pady=(0, 8)
        )
        ttk.Label(analog_frame, text="(0 – 4095)", foreground="gray").grid(
            row=1, column=2, sticky="w", padx=(0, 8), pady=(0, 8)
        )

    def _build_gpio_states_tab(self, parent):
        parent.columnconfigure(0, weight=1)

        states_frame = ttk.LabelFrame(parent, text="GPIO pin states")
        states_frame.grid(row=0, column=0, sticky="nsew", padx=6, pady=6)
        for _idx, (_pin_id, _pin_name) in enumerate(sorted(BOARD_PIN_NAMES.items())):
            _col = (_idx % 2) * 3
            _row = _idx // 2
            ttk.Label(states_frame, text=_pin_name, anchor="w").grid(row=_row, column=_col, sticky="w", padx=(8, 2), pady=2)
            ttk.Label(states_frame, text="=").grid(row=_row, column=_col + 1, padx=0, pady=2)
            ttk.Label(states_frame, textvariable=self.gpio_pin_state_vars[_pin_id], width=3).grid(row=_row, column=_col + 2, sticky="w", padx=(0, 16), pady=2)

        ttk.Button(parent, text="Refresh all", command=self.gpio_read_all).grid(row=1, column=0, sticky="w", padx=8, pady=(0, 8))

    def _build_gpio_events_tab(self, parent):
        """Arm/disarm hardware GPIO interrupts on the firmware side.

        The firmware exposes CMD_ID_GPIOEVT to attach an ISR to a configurable
        pin. While armed, every edge produces a spontaneous CMD_ID_GPIOEVT
        response frame (high bit of the `index` field set) carrying the pin,
        new level and a millisecond timestamp. Use this for buttons or any
        external signal you want to react to without polling.
        """
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(1, weight=1)

        ctrl = ttk.LabelFrame(parent, text="GPIO event sources")
        ctrl.grid(row=0, column=0, sticky="ew", padx=6, pady=6)
        ctrl.columnconfigure(1, weight=1)

        ttk.Label(ctrl, text="Pin").grid(row=0, column=0, sticky="w", padx=8, pady=(8, 4))
        # Restrict to GPIOs that make sense as inputs (exclude the LED, which is owned by LEDC).
        event_pin_names = tuple(name for name, pin_id in BOARD_PINS.items() if pin_id != BOARD_LED_PIN_ID)
        ttk.Combobox(
            ctrl,
            textvariable=self.gpioevt_pin_var,
            values=event_pin_names,
            state="readonly",
            width=20,
        ).grid(row=0, column=1, sticky="ew", padx=8, pady=(8, 4))
        btns = ttk.Frame(ctrl)
        btns.grid(row=0, column=2, sticky="e", padx=(0, 8), pady=(8, 4))
        ttk.Button(btns, text="Enable", command=self.gpioevt_enable_selected).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btns, text="Disable", command=self.gpioevt_disable_selected).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btns, text="Disable all", command=self.gpioevt_clear).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(btns, text="Query", command=self.gpioevt_query).pack(side=tk.LEFT)

        ttk.Label(ctrl, text="Armed pins").grid(row=1, column=0, sticky="w", padx=8, pady=(4, 8))
        ttk.Label(ctrl, textvariable=self.gpioevt_enabled_mask_var, foreground="gray").grid(
            row=1, column=1, columnspan=2, sticky="w", padx=8, pady=(4, 8))

        log_frame = ttk.LabelFrame(parent, text="Event log")
        log_frame.grid(row=1, column=0, sticky="nsew", padx=6, pady=(0, 6))
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)

        self.gpioevt_log = tk.Text(log_frame, height=12, wrap=tk.NONE, state=tk.DISABLED)
        scroll = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.gpioevt_log.yview)
        self.gpioevt_log.configure(yscrollcommand=scroll.set)
        self.gpioevt_log.grid(row=0, column=0, sticky="nsew", padx=(6, 0), pady=6)
        scroll.grid(row=0, column=1, sticky="ns", pady=6)
        ttk.Button(log_frame, text="Clear log", command=self._gpioevt_clear_log).grid(
            row=1, column=0, columnspan=2, sticky="w", padx=8, pady=(0, 6))

    def _gpioevt_clear_log(self):
        if self.gpioevt_log is None:
            return
        self.gpioevt_log.configure(state=tk.NORMAL)
        self.gpioevt_log.delete("1.0", tk.END)
        self.gpioevt_log.configure(state=tk.DISABLED)

    def _gpioevt_log_line(self, line: str):
        if self.gpioevt_log is None:
            return
        self.gpioevt_log.configure(state=tk.NORMAL)
        self.gpioevt_log.insert(tk.END, line + "\n")
        self.gpioevt_log.see(tk.END)
        self.gpioevt_log.configure(state=tk.DISABLED)

    def _selected_event_pin_id(self) -> int:
        return BOARD_PINS.get(self.gpioevt_pin_var.get(), -1)

    def gpioevt_enable_selected(self):
        pin = self._selected_event_pin_id()
        if pin < 0:
            return
        self.safe_command(lambda: self.client.command_gpioevt_enable(pin))

    def gpioevt_disable_selected(self):
        pin = self._selected_event_pin_id()
        if pin < 0:
            return
        self.safe_command(lambda: self.client.command_gpioevt_disable(pin))

    def gpioevt_clear(self):
        self.safe_command(self.client.command_gpioevt_clear)

    def gpioevt_query(self):
        self.safe_command(self.client.command_gpioevt_query)

    def _build_rf_tab(self, tab):
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(0, weight=1)

        rf_frame = ttk.LabelFrame(tab, text=DEVICE_RF_FRAME_TITLE)
        rf_frame.grid(row=0, column=0, sticky="nsew", padx=6, pady=6)
        rf_frame.columnconfigure(1, weight=1)

        ttk.Button(rf_frame, text="Read status", command=self.rf_read_status).grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 4))
        ttk.Label(rf_frame, textvariable=self.rf_status_var).grid(row=0, column=1, sticky="w", padx=8, pady=(8, 4))

        ttk.Label(rf_frame, text="BLE TX power").grid(row=1, column=0, sticky="w", padx=8, pady=4)
        ttk.Combobox(
            rf_frame,
            textvariable=self.rf_power_var,
            values=tuple(label for label, _ in RF_POWER_OPTIONS),
            state="readonly",
            width=22,
        ).grid(row=1, column=1, sticky="ew", padx=8, pady=4)
        ttk.Button(rf_frame, text="Apply", command=self.rf_apply_power).grid(row=1, column=2, sticky="ew", padx=(0, 8), pady=4)

        ttk.Label(rf_frame, text="XTAL cap").grid(row=2, column=0, sticky="w", padx=8, pady=4)
        cap_frame = ttk.Frame(rf_frame)
        cap_frame.grid(row=2, column=1, sticky="ew", padx=8, pady=4)
        self.rf_cap_spinbox = ttk.Spinbox(cap_frame, textvariable=self.rf_cap_var, from_=0, to=63, width=6)
        self.rf_cap_spinbox.pack(side=tk.LEFT)
        ttk.Label(cap_frame, textvariable=self.rf_cap_status_var).pack(side=tk.LEFT, padx=(8, 0))
        self.rf_cap_button = ttk.Button(rf_frame, text="Apply", command=self.rf_apply_cap)
        self.rf_cap_button.grid(row=2, column=2, sticky="ew", padx=(0, 8), pady=4)
        if not RF_CAP_SUPPORTED:
            self.rf_cap_spinbox.state(["disabled"])
            self.rf_cap_button.state(["disabled"])

        ttk.Label(rf_frame, text="Primary channels").grid(row=3, column=0, sticky="w", padx=8, pady=4)
        channel_frame = ttk.Frame(rf_frame)
        channel_frame.grid(row=3, column=1, sticky="w", padx=8, pady=4)
        self.rf_ch0_spinbox = ttk.Spinbox(channel_frame, textvariable=self.rf_ch0_var, from_=0, to=39, width=4)
        self.rf_ch0_spinbox.pack(side=tk.LEFT)
        self.rf_ch1_spinbox = ttk.Spinbox(channel_frame, textvariable=self.rf_ch1_var, from_=0, to=39, width=4)
        self.rf_ch1_spinbox.pack(side=tk.LEFT, padx=4)
        self.rf_ch2_spinbox = ttk.Spinbox(channel_frame, textvariable=self.rf_ch2_var, from_=0, to=39, width=4)
        self.rf_ch2_spinbox.pack(side=tk.LEFT)
        self.rf_channels_button = ttk.Button(rf_frame, text="Apply", command=self.rf_apply_channels)
        self.rf_channels_button.grid(row=3, column=2, sticky="ew", padx=(0, 8), pady=4)
        if not RF_CHANNELS_SUPPORTED:
            self.rf_ch0_spinbox.state(["disabled"])
            self.rf_ch1_spinbox.state(["disabled"])
            self.rf_ch2_spinbox.state(["disabled"])
            self.rf_channels_button.state(["disabled"])

        ttk.Label(rf_frame, text="Coded min window ms").grid(row=4, column=0, sticky="w", padx=8, pady=4)
        ttk.Spinbox(rf_frame, textvariable=self.coded_min_ms_var, from_=10, to=2000, increment=10, width=8).grid(row=4, column=1, sticky="w", padx=8, pady=4)
        ttk.Button(rf_frame, text="Apply", command=self.rf_apply_coded_min).grid(row=4, column=2, sticky="ew", padx=(0, 8), pady=4)

        quick = ttk.Frame(rf_frame)
        quick.grid(row=5, column=0, columnspan=3, sticky="ew", padx=8, pady=(12, 8))
        ttk.Button(quick, text="Set +20 dBm", command=self.rf_high_power).pack(side=tk.LEFT)
        ttk.Button(quick, text="Set 0 dBm", command=self.rf_zero_dbm).pack(side=tk.LEFT, padx=6)
        ttk.Button(quick, text="Coded 50% of window", command=self.rf_coded_50pct_of_window).pack(side=tk.LEFT, padx=6)

        ttk.Label(
            rf_frame,
            text=DEVICE_RF_NOTE,
            foreground="gray",
        ).grid(row=6, column=0, columnspan=3, sticky="w", padx=8, pady=(0, 8))

    def _build_uart_tab(self, tab):
        tab.columnconfigure(0, weight=1)

        uart_frame = ttk.LabelFrame(tab, text="USB Serial/JTAG")
        uart_frame.grid(row=0, column=0, sticky="ew", padx=6, pady=6)
        uart_frame.columnconfigure(1, weight=1)

        ttk.Button(uart_frame, text="Status", command=self.uart_status).grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 4))
        ttk.Label(uart_frame, textvariable=self.uart_status_var).grid(row=0, column=1, sticky="w", padx=8, pady=(8, 4))
        ttk.Button(uart_frame, text="Ping", command=self.uart_ping).grid(row=1, column=0, sticky="ew", padx=8, pady=4)
        ttk.Spinbox(uart_frame, textvariable=self.uart_ping_seq_var, from_=0, to=65535, width=8).grid(row=1, column=1, sticky="w", padx=8, pady=4)

        self.uart_baud_combo = ttk.Combobox(
            uart_frame,
            textvariable=self.uart_baud_index_var,
            values=tuple(f"{index} - {baud}" for index, baud in enumerate(FIRMWARE_BAUDS)),
            state="readonly",
            width=18,
        )
        self.uart_baud_combo.grid(row=2, column=0, sticky="ew", padx=8, pady=(4, 8))
        self.uart_set_baud_button = ttk.Button(uart_frame, text="Set firmware baud", command=self.uart_set_baud)
        self.uart_set_baud_button.grid(row=2, column=1, sticky="w", padx=8, pady=(4, 8))
        if not UART_SET_BAUD_SUPPORTED:
            self.uart_baud_combo.state(["disabled"])
            self.uart_set_baud_button.state(["disabled"])
        ttk.Label(
            uart_frame,
            text=DEVICE_UART_NOTE,
            foreground="gray",
            justify="left",
            wraplength=520,
        ).grid(row=3, column=0, columnspan=2, sticky="w", padx=8, pady=(0, 8))

    def _build_txadv_tab(self, tab):
        tab.columnconfigure(0, weight=1)

        frm = ttk.LabelFrame(tab, text="Transmit Custom Advertisement")
        frm.grid(row=0, column=0, sticky="nsew", padx=6, pady=6)
        frm.columnconfigure(1, weight=1)

        # PHY
        ttk.Label(frm, text="PHY mode").grid(row=0, column=0, sticky="w", padx=8, pady=(8, 4))
        ttk.Combobox(
            frm,
            textvariable=self.txadv_phy_var,
            values=[lbl for lbl, _ in TXADV_PHY_OPTIONS],
            state="readonly",
            width=32,
        ).grid(row=0, column=1, sticky="ew", padx=8, pady=(8, 4))

        # Interval
        ttk.Label(frm, text="Interval ms").grid(row=1, column=0, sticky="w", padx=8, pady=4)
        int_frame = ttk.Frame(frm)
        int_frame.grid(row=1, column=1, sticky="w", padx=8, pady=4)
        ttk.Spinbox(int_frame, textvariable=self.txadv_interval_ms_var,
                    from_=20.0, to=10240.0, increment=10.0, width=10).pack(side=tk.LEFT)
        ttk.Label(int_frame, text="(units = ms / 0.625)").pack(side=tk.LEFT, padx=(8, 0))

        # ADV data
        ttk.Label(frm, text="ADV data (hex)").grid(row=2, column=0, sticky="w", padx=8, pady=4)
        ttk.Entry(frm, textvariable=self.txadv_data_var, width=52).grid(
            row=2, column=1, sticky="ew", padx=8, pady=4)
        ttk.Label(frm, text="max 31 bytes; spaces optional", foreground="gray").grid(
            row=3, column=1, sticky="w", padx=8, pady=(0, 4))

        # Buttons
        btn_frame = ttk.Frame(frm)
        btn_frame.grid(row=4, column=0, columnspan=2, sticky="ew", padx=8, pady=(8, 4))
        ttk.Button(btn_frame, text="Start TX Adv", command=self.txadv_start).pack(side=tk.LEFT)
        ttk.Button(btn_frame, text="Stop TX Adv", command=self.txadv_stop).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(btn_frame, text="Query status", command=self.txadv_query).pack(side=tk.LEFT, padx=(8, 0))

        # Quick presets
        ttk.Separator(frm, orient=tk.HORIZONTAL).grid(
            row=5, column=0, columnspan=2, sticky="ew", padx=8, pady=8)
        ttk.Label(frm, text="Quick preset").grid(row=6, column=0, sticky="w", padx=8, pady=4)
        preset_frame = ttk.Frame(frm)
        preset_frame.grid(row=6, column=1, sticky="w", padx=8, pady=4)
        ttk.Button(preset_frame, text="iBeacon-like",
                   command=self._txadv_preset_ibeacon).pack(side=tk.LEFT)
        ttk.Button(preset_frame, text="TB-03F-KIT name",
                   command=self._txadv_preset_name).pack(side=tk.LEFT, padx=(8, 0))

        # Status
        ttk.Separator(frm, orient=tk.HORIZONTAL).grid(
            row=7, column=0, columnspan=2, sticky="ew", padx=8, pady=8)
        ttk.Label(frm, text="Status").grid(row=8, column=0, sticky="w", padx=8, pady=(0, 8))
        ttk.Label(frm, textvariable=self.txadv_status_var, anchor="w").grid(
            row=8, column=1, sticky="ew", padx=8, pady=(0, 8))

    def _build_conn_tab(self, tab):
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(1, weight=1)

        # --- Top: connection controls ---
        ctrl = ttk.LabelFrame(tab, text="BLE Central Connection")
        ctrl.grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 4))
        ctrl.columnconfigure(1, weight=1)

        ttk.Label(ctrl, text="Peer MAC (hex)").grid(row=0, column=0, sticky="w", padx=8, pady=4)
        peer_entry = ttk.Entry(ctrl, textvariable=self.conn_peer_var, width=20)
        peer_entry.grid(row=0, column=1, sticky="w", padx=8, pady=4)
        ttk.Label(ctrl, text="e.g. AA:BB:CC:DD:EE:FF").grid(row=0, column=2, sticky="w", padx=4)

        ttk.Label(ctrl, text="Addr type").grid(row=1, column=0, sticky="w", padx=8, pady=4)
        ttk.Combobox(ctrl, textvariable=self.conn_peer_type_var,
                     values=["Public", "Random"], state="readonly", width=10).grid(
            row=1, column=1, sticky="w", padx=8, pady=4)

        ttk.Label(ctrl, text="PHY").grid(row=2, column=0, sticky="w", padx=8, pady=4)
        ttk.Combobox(ctrl, textvariable=self.conn_phy_var,
                     values=[lbl for lbl, _ in CONN_PHY_OPTIONS], state="readonly", width=14).grid(
            row=2, column=1, sticky="w", padx=8, pady=4)

        ttk.Label(ctrl, text="Timeout (s)").grid(row=2, column=2, sticky="e", padx=(0, 4), pady=4)
        timeout_frame = ttk.Frame(ctrl)
        timeout_frame.grid(row=2, column=3, sticky="w", padx=(0, 8), pady=4)
        ttk.Spinbox(timeout_frame, textvariable=self.conn_timeout_s_var, from_=0, to=120, width=4).pack(side=tk.LEFT)
        ttk.Label(timeout_frame, text="(0 = wait for firmware)", foreground="gray").pack(side=tk.LEFT, padx=(6, 0))

        btn_row = ttk.Frame(ctrl)
        btn_row.grid(row=3, column=0, columnspan=3, sticky="w", padx=8, pady=(4, 8))
        ttk.Button(btn_row, text="Connect", command=self.conn_connect).pack(side=tk.LEFT)
        ttk.Button(btn_row, text="Disconnect", command=self.conn_disconnect).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(btn_row, text="Cancel", command=self.conn_cancel).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(btn_row, text="Status", command=self.conn_status).pack(side=tk.LEFT, padx=(8, 0))

        info_row = ttk.Frame(ctrl)
        info_row.grid(row=4, column=0, columnspan=3, sticky="ew", padx=8, pady=(0, 8))
        ttk.Label(info_row, text="State:").pack(side=tk.LEFT)
        ttk.Label(info_row, textvariable=self.conn_state_var, width=14).pack(side=tk.LEFT, padx=(4, 12))
        ttk.Label(info_row, text="Handle:").pack(side=tk.LEFT)
        ttk.Label(info_row, textvariable=self.conn_handle_var, width=6).pack(side=tk.LEFT, padx=(4, 12))
        ttk.Label(info_row, text="Interval:").pack(side=tk.LEFT)
        ttk.Label(info_row, textvariable=self.conn_interval_var, width=10).pack(side=tk.LEFT, padx=(4, 12))
        # "Last result" surfaces the most recent non-OK CONN status (e.g. "Denied")
        # so the user sees connection failures without scrolling the log.
        ttk.Label(info_row, text="Last result:").pack(side=tk.LEFT)
        self.conn_last_result_label = ttk.Label(info_row, textvariable=self.conn_last_result_var, width=22, foreground="gray")
        self.conn_last_result_label.pack(side=tk.LEFT, padx=(4, 0))

        # --- Middle: ATT write ---
        write_frame = ttk.LabelFrame(tab, text="ATT Write (Write Without Response)")
        write_frame.grid(row=1, column=0, sticky="ew", padx=8, pady=4)
        write_frame.columnconfigure(1, weight=1)

        ttk.Label(write_frame, text="ATT handle (hex)").grid(row=0, column=0, sticky="w", padx=8, pady=4)
        ttk.Entry(write_frame, textvariable=self.txdata_handle_var, width=8).grid(
            row=0, column=1, sticky="w", padx=8, pady=4)

        ttk.Label(write_frame, text="Data (hex bytes)").grid(row=1, column=0, sticky="w", padx=8, pady=4)
        ttk.Entry(write_frame, textvariable=self.txdata_data_var, width=40).grid(
            row=1, column=1, sticky="ew", padx=8, pady=4)

        ttk.Button(write_frame, text="Send ATT Write", command=self.conn_txdata).grid(
            row=2, column=0, columnspan=2, sticky="w", padx=8, pady=(4, 8))

        # --- Bottom: received data log ---
        rx_frame = ttk.LabelFrame(tab, text="Received ATT Notifications / Indications")
        rx_frame.grid(row=2, column=0, sticky="nsew", padx=8, pady=(4, 8))
        rx_frame.columnconfigure(0, weight=1)
        rx_frame.rowconfigure(0, weight=1)
        tab.rowconfigure(2, weight=1)

        self.conn_rxdata_log = tk.Text(rx_frame, height=8, wrap=tk.WORD, state=tk.DISABLED)
        rxscroll = ttk.Scrollbar(rx_frame, orient=tk.VERTICAL, command=self.conn_rxdata_log.yview)
        self.conn_rxdata_log.configure(yscrollcommand=rxscroll.set)
        self.conn_rxdata_log.grid(row=0, column=0, sticky="nsew", padx=(6, 0), pady=6)
        rxscroll.grid(row=0, column=1, sticky="ns", pady=6)
        ttk.Button(rx_frame, text="Clear", command=self._conn_rxdata_clear).grid(
            row=1, column=0, columnspan=2, sticky="w", padx=8, pady=(0, 6))

    def _build_log_tab(self, tab):
        tab.columnconfigure(0, weight=1)
        tab.rowconfigure(0, weight=1)
        self.log_text = tk.Text(tab, height=10, wrap=tk.WORD)
        yscroll = ttk.Scrollbar(tab, orient=tk.VERTICAL, command=self.log_text.yview)
        self.log_text.configure(yscrollcommand=yscroll.set)
        self.log_text.grid(row=0, column=0, sticky="nsew", padx=(6, 0), pady=6)
        yscroll.grid(row=0, column=1, sticky="ns", pady=6)

    def refresh_ports(self):
        ports = []
        if list_ports is not None:
            ports = [port.device for port in list_ports.comports()]
        if not ports:
            ports = ["COM4", "COM7", "COM10"] if sys.platform.startswith("win") else ["/dev/ttyUSB0", "/dev/ttyACM0"]
        preferred = self.initial_port or self.port_var.get().strip()
        if self.initial_port and self.initial_port not in ports:
            ports = [self.initial_port] + [port for port in ports if port != self.initial_port]
        self.port_combo.configure(values=ports)
        if ports and (not preferred or preferred not in ports):
            self.port_var.set(ports[0])
        elif preferred:
            self.port_var.set(preferred)

    def apply_startup_window_mode(self):
        if not self.start_maximized:
            return
        try:
            self.state("zoomed")
            return
        except tk.TclError:
            pass
        try:
            self.attributes("-zoomed", True)
            return
        except tk.TclError:
            pass
        self.update_idletasks()
        self.geometry(f"{self.winfo_screenwidth()}x{self.winfo_screenheight()}+0+0")

    def connect(self):
        try:
            self.client.open(self.port_var.get().strip(), int(self.baud_var.get()), self.pulse_reset_var.get())
            self.status_var.set("Connected")
            self.led_blink_generation += 1
            self.after(GUI_BOOTSTRAP_DELAY_MS, self.send_info)
            self.after(GUI_BOOTSTRAP_DELAY_MS + 240, self.led_query_state)
        except Exception as exc:
            self.status_var.set("Connection failed")
            self.log(f"Connection failed: {exc}")
            messagebox.showerror("Connection failed", str(exc))
        finally:
            self._sync_connection_buttons()

    def disconnect(self):
        self.led_blink_generation += 1
        self.cancel_gpio_read_batch()
        self._cancel_scan_response_watchdog()
        self._cancel_conn_timeout()
        self.client.close()
        self.status_var.set("Disconnected")
        self.scan_state_var.set("Stopped")
        self.vbat_var.set("-")
        self.temp_var.set("-")
        self.log("Disconnected")
        self._sync_connection_buttons()

    def _sync_connection_buttons(self):
        """Enable Connect when idle, Disconnect when the serial link is open."""
        is_open = bool(self.client and self.client.is_open)
        try:
            if is_open:
                self.connect_button.state(["disabled"])
                self.disconnect_button.state(["!disabled"])
            else:
                self.connect_button.state(["!disabled"])
                self.disconnect_button.state(["disabled"])
        except Exception:
            pass
        # Scan buttons depend on connection state too — without a link there's
        # nobody to start/stop scanning. Resync them in the same pass.
        self._sync_scan_buttons()

    def _sync_scan_buttons(self):
        """Start enabled only when scan is Stopped AND link is open.
        Stop enabled only when scan is running AND link is open.
        Both disabled when disconnected."""
        if not hasattr(self, "start_scan_button") or not hasattr(self, "stop_scan_button"):
            return
        is_open = bool(self.client and self.client.is_open)
        scan_running = is_open and self.scan_state_var.get() not in ("", "Stopped")
        try:
            if not is_open:
                self.start_scan_button.state(["disabled"])
                self.stop_scan_button.state(["disabled"])
            elif scan_running:
                self.start_scan_button.state(["disabled"])
                self.stop_scan_button.state(["!disabled"])
            else:
                self.start_scan_button.state(["!disabled"])
                self.stop_scan_button.state(["disabled"])
        except Exception:
            pass

    def send_info(self):
        self.safe_command(self.client.command_info)
        self.after(80, lambda: self.safe_command(self.client.command_version_info))
        if DEVICE_SUPPORTS_VBAT:
            self.after(160, self.send_vbat)

    def clear_mac_list(self):
        self.white_list.clear()
        self.black_list.clear()
        self.refresh_mac_tree()
        self.safe_command(self.client.command_clear_list)

    def add_white_mac(self):
        self.add_mac("white")

    def add_black_mac(self):
        self.add_mac("black")

    def add_mac(self, mode: str):
        try:
            mac = normalize_mac(self.mac_entry_var.get())
        except ValueError as exc:
            messagebox.showerror("Invalid MAC", str(exc))
            return
        target = self.white_list if mode == "white" else self.black_list
        other = self.black_list if mode == "white" else self.white_list
        switched_mode = bool(other)
        if switched_mode:
            other.clear()
            self.log("Firmware supports one MAC-list mode at a time; switched list mode and cleared the other list")
        if mac not in target:
            target.append(mac)
        self.refresh_mac_tree()
        try:
            if switched_mode and self.client.is_open:
                self.client.command_clear_list()
            if mode == "white":
                self.client.command_add_white(mac)
            else:
                self.client.command_add_black(mac)
        except Exception as exc:
            self.command_error(exc)

    def remove_selected_mac(self):
        for item_id in self.mac_tree.selection():
            mode, mac = self.mac_tree.item(item_id, "values")
            if mode == "White" and mac in self.white_list:
                self.white_list.remove(mac)
            if mode == "Black" and mac in self.black_list:
                self.black_list.remove(mac)
        self.refresh_mac_tree()
        self.log("Local list changed. Use Clear MAC List and Start Scan to reapply it to firmware.")

    def refresh_mac_tree(self):
        self.mac_tree.delete(*self.mac_tree.get_children())
        for mac in self.white_list:
            self.mac_tree.insert("", tk.END, values=("White", mac))
        for mac in self.black_list:
            self.mac_tree.insert("", tk.END, values=("Black", mac))

    def apply_long_range_preset(self):
        self.phy_1m_var.set(False)
        self.phy_coded_var.set(True)
        self.active_scan_var.set(False)
        self.duplicate_filter_var.set(False)
        self.filter_random_var.set(False)
        self.filter_private_var.set(False)
        self.window_ms_var.set(150.0)
        self.window_coded_ms_var.set(0.0)
        self.own_addr_var.set(OWN_ADDRESS_TYPES[0])

    def apply_mixed_preset(self):
        self.phy_1m_var.set(True)
        self.phy_coded_var.set(True)
        self.active_scan_var.set(False)
        self.duplicate_filter_var.set(False)
        self.filter_random_var.set(False)
        self.filter_private_var.set(False)
        self.window_ms_var.set(150.0)
        self.window_coded_ms_var.set(0.0)
        self.own_addr_var.set(OWN_ADDRESS_TYPES[0])

    def apply_1m_preset(self):
        self.phy_1m_var.set(True)
        self.phy_coded_var.set(False)
        self.active_scan_var.set(False)
        self.duplicate_filter_var.set(False)
        self.filter_random_var.set(False)
        self.filter_private_var.set(False)
        self.window_ms_var.set(30.0)
        self.window_coded_ms_var.set(0.0)
        self.own_addr_var.set(OWN_ADDRESS_TYPES[0])

    def current_scan_config(self) -> ScanConfig:
        reverse_own = {value: key for key, value in OWN_ADDRESS_TYPES.items()}
        return ScanConfig(
            own_address_type=reverse_own.get(self.own_addr_var.get(), 0),
            filter_random_addresses=self.filter_random_var.get(),
            filter_private_addresses=self.filter_private_var.get(),
            duplicate_filter=self.duplicate_filter_var.get(),
            active_scan=self.active_scan_var.get(),
            phy_1m=self.phy_1m_var.get(),
            phy_coded=self.phy_coded_var.get(),
            window_ms=float(self.window_ms_var.get()),
            window_coded_ms=float(self.window_coded_ms_var.get()),
        )

    def start_scan(self):
        config = self.current_scan_config()
        if not config.phy_1m and not config.phy_coded:
            messagebox.showerror("Invalid scan", "Enable at least one PHY")
            return
        try:
            if self.white_list and self.black_list:
                raise RuntimeError("firmware supports either whitelist or blacklist, not both at the same time")
            if self.reapply_lists_var.get():
                self.client.command_info()
                self.client.command_clear_list()
                for mac in self.white_list:
                    self.client.command_add_white(mac)
                for mac in self.black_list:
                    self.client.command_add_black(mac)
            self.client.command_start_scan(config)
            self.last_started_scan_config = config
            self.log(f"Start scan requested: {config.describe()}")
            self.log("Advertisements are shown in the Advertisements tab and counters, not in the command log")
            # Do NOT set scan_state_var here. The firmware's CMD_ID_SCAN response
            # is the source of truth; if the firmware never replies (chip crash,
            # cable issue, etc.) the user must keep Start Scan clickable.
            # A 2.5 s grace window: if no scan-enabled response arrives, log a
            # diagnostic so the user understands why nothing happened.
            self._cancel_scan_response_watchdog()
            self._scan_response_watchdog = self.after(2500, self._scan_response_timeout)
        except Exception as exc:
            self.command_error(exc)

    def stop_scan(self):
        self.last_started_scan_config = None
        self.safe_command(self.client.command_stop_scan)
        # Similar reasoning: wait for the firmware's response before flipping
        # buttons. If no response within 2.5 s, log and resync to "Stopped"
        # so the user is not left locked out.
        self._cancel_scan_response_watchdog()
        self._scan_response_watchdog = self.after(2500, self._scan_stop_response_timeout)

    def _cancel_scan_response_watchdog(self):
        wd = getattr(self, "_scan_response_watchdog", None)
        if wd is not None:
            try:
                self.after_cancel(wd)
            except Exception:
                pass
            self._scan_response_watchdog = None

    def _scan_response_timeout(self):
        self._scan_response_watchdog = None
        self.log("Scan start: no response from firmware within 2.5 s — buttons stay unlocked, you can retry")
        # Force state back to Stopped so Start Scan is clickable again.
        self.scan_state_var.set("Stopped")

    def _scan_stop_response_timeout(self):
        self._scan_response_watchdog = None
        self.log("Scan stop: no response from firmware within 2.5 s — forcing Stopped state")
        self.scan_state_var.set("Stopped")

    def safe_command(self, callback):
        try:
            callback()
        except Exception as exc:
            self.command_error(exc)

    def command_error(self, exc: Exception):
        self.log(f"Command failed: {exc}")
        messagebox.showerror("Command failed", str(exc))

    def unsupported_feature(self, title: str, detail: str):
        self.log(detail)
        messagebox.showinfo(title, detail)

    def status_name(self, status: int) -> str:
        return COMMAND_STATUS.get(status, f"0x{status:02X}")

    def selected_gpio_pin_id(self) -> int:
        return BOARD_PINS.get(self.gpio_pin_var.get(), next(iter(BOARD_PINS.values()), BOARD_LED_PIN_ID))

    def selected_uart_baud_index(self) -> int:
        return int(self.uart_baud_index_var.get().split("-", 1)[0].strip())

    def led_mask_from_board_mask(self, board_mask: int) -> int:
        return board_mask & LED_ALL_MASK

    def pin_state_is_high(self, pin_id: int):
        state = self.gpio_pin_state_vars[pin_id].get()
        if state == "?":
            return None
        return state not in ("0", "")

    def status_pin_text(self, pin_is_high: bool | None) -> str:
        if pin_is_high is None:
            return "?"
        is_active = (not pin_is_high) if DEVICE_STATUS_PIN_ACTIVE_LOW else bool(pin_is_high)
        return DEVICE_STATUS_PIN_ACTIVE_LABEL if is_active else DEVICE_STATUS_PIN_INACTIVE_LABEL

    def set_single_led_state(self, pin_is_high: bool | None):
        if pin_is_high is None:
            self.led_state_var.set("?")
            return
        is_on = (not pin_is_high) if DEVICE_BOARD_LED_ACTIVE_LOW else bool(pin_is_high)
        self.led_state_var.set("On" if is_on else "Off")

    def refresh_board_labels_from_cache(self):
        board_mask = 0
        for pin_id, mask_index in BOARD_PIN_MASK_INDEX.items():
            if self.pin_state_is_high(pin_id):
                board_mask |= 1 << mask_index
        self.board_mask_var.set(f"0x{board_mask:04X}")
        if DEVICE_LED_MASK_MODE:
            self.led_state_var.set(f"0x{self.led_mask_from_board_mask(board_mask):02X}")
        else:
            self.set_single_led_state(self.pin_state_is_high(BOARD_LED_PIN_ID))
        self.key_state_var.set(self.status_pin_text(self.pin_state_is_high(STATUS_GPIO_PIN_ID)))

    def set_board_labels(self, board_mask: int, led_mask=None):
        self.board_mask_var.set(f"0x{board_mask:04X}")
        if DEVICE_LED_MASK_MODE:
            if led_mask is None:
                led_mask = self.led_mask_from_board_mask(board_mask)
            self.led_state_var.set(f"0x{led_mask:02X}")
            key_index = BOARD_PIN_MASK_INDEX.get(STATUS_GPIO_PIN_ID)
            if key_index is None:
                self.key_state_var.set("?")
            else:
                self.key_state_var.set(self.status_pin_text(bool(board_mask & (1 << key_index))))
            return
        led_index = BOARD_PIN_MASK_INDEX.get(BOARD_LED_PIN_ID)
        if led_index is None:
            self.led_state_var.set("?")
        else:
            self.set_single_led_state(bool(board_mask & (1 << led_index)))
        key_index = BOARD_PIN_MASK_INDEX.get(STATUS_GPIO_PIN_ID)
        if key_index is None:
            self.key_state_var.set("?")
        else:
            self.key_state_var.set(self.status_pin_text(bool(board_mask & (1 << key_index))))

    def cancel_gpio_read_batch(self):
        self.gpio_read_generation += 1
        self.pending_gpio_read_ids = []
        self.pending_gpio_read_pin = None

    def start_gpio_read_batch(self, pin_ids):
        if not self.client.is_open:
            raise RuntimeError("Serial port is not open")
        self.gpio_read_generation += 1
        self.pending_gpio_read_ids = list(pin_ids)
        self.pending_gpio_read_pin = None
        self.issue_next_gpio_read(self.gpio_read_generation)

    def issue_next_gpio_read(self, generation: int):
        if generation != self.gpio_read_generation or not self.client.is_open:
            return
        if self.pending_gpio_read_pin is not None or not self.pending_gpio_read_ids:
            return
        pin_id = self.pending_gpio_read_ids.pop(0)
        self.pending_gpio_read_pin = pin_id
        try:
            self.client.command_gpio_read(pin_id)
        except Exception:
            self.pending_gpio_read_pin = None
            raise

    def continue_gpio_read_batch(self, response_pin_id: int | None = None):
        if self.pending_gpio_read_pin is None:
            return
        if response_pin_id is not None and response_pin_id != self.pending_gpio_read_pin:
            return
        generation = self.gpio_read_generation
        self.pending_gpio_read_pin = None
        if self.pending_gpio_read_ids:
            self.after(0, lambda: self.issue_next_gpio_read(generation))

    def led_pin_ids_from_mask(self, mask: int):
        return [pin_id for bit, pin_id in LED_PIN_CODES.items() if mask & bit]

    def _write_led_pins(self, pin_ids, enabled: bool):
        for pin_id in pin_ids:
            if pin_id == BOARD_LED_PIN_ID and DEVICE_BOARD_LED_ACTIVE_LOW:
                value = 0 if enabled else 1
            else:
                value = 1 if enabled else 0
            self.client.command_gpio_write(pin_id, value)

    def _toggle_led_pins(self, pin_ids):
        for pin_id in pin_ids:
            self.client.command_gpio_toggle(pin_id)

    def _toggle_board_led(self):
        board_led_high = self.pin_state_is_high(BOARD_LED_PIN_ID)
        if board_led_high is None:
            self.client.command_gpio_toggle(BOARD_LED_PIN_ID)
            return
        board_led_on = (not board_led_high) if DEVICE_BOARD_LED_ACTIVE_LOW else bool(board_led_high)
        self._write_led_pins([BOARD_LED_PIN_ID], not board_led_on)

    def _blink_led_pins(self, pin_ids, steps_left: int, enabled: bool, delay_ms: int, generation: int):
        if generation != self.led_blink_generation or not self.client.is_open:
            return
        try:
            self._write_led_pins(pin_ids, enabled)
        except Exception as exc:
            self.command_error(exc)
            return
        if steps_left > 1:
            self.after(delay_ms, lambda: self._blink_led_pins(pin_ids, steps_left - 1, not enabled, delay_ms, generation))

    def led_set_mask(self, mask: int, enabled: bool):
        pin_ids = self.led_pin_ids_from_mask(mask)
        if not pin_ids:
            return
        self.led_blink_generation += 1
        try:
            self._write_led_pins(pin_ids, enabled)
        except Exception as exc:
            self.command_error(exc)

    def led_toggle_mask(self, mask: int):
        pin_ids = self.led_pin_ids_from_mask(mask)
        if not pin_ids:
            return
        self.led_blink_generation += 1
        try:
            if pin_ids == [BOARD_LED_PIN_ID]:
                self._toggle_board_led()
            else:
                self._toggle_led_pins(pin_ids)
        except Exception as exc:
            self.command_error(exc)

    def led_all_off(self):
        self.led_set_mask(LED_ALL_MASK, False)

    def led_blink_mask(self, mask: int):
        pin_ids = self.led_pin_ids_from_mask(mask)
        if not pin_ids:
            return
        count = max(1, min(5, int(self.led_blink_count_var.get())))
        delay_ms = max(10, min(200, int(self.led_blink_delay_var.get())))
        self.led_blink_generation += 1
        self._blink_led_pins(pin_ids, count * 2, True, delay_ms, self.led_blink_generation)

    def gpio_read_selected(self):
        self.cancel_gpio_read_batch()
        self.safe_command(lambda: self.client.command_gpio_read(self.selected_gpio_pin_id()))

    def gpio_write_selected(self):
        pin_id = self.selected_gpio_pin_id()
        if not DEVICE_ALLOW_GENERAL_GPIO_WRITE and pin_id != BOARD_LED_PIN_ID:
            self.unsupported_feature(
                "GPIO write",
                f"{DEVICE_LABEL} firmware allows GPIO write only on the board LED.",
            )
            return
        if pin_id in LED_PIN_IDS:
            self.safe_command(lambda: self._write_led_pins([pin_id], self.gpio_value_var.get()))
            return
        self.safe_command(lambda: self.client.command_gpio_write(pin_id, self.gpio_value_var.get()))

    def gpio_toggle_selected(self):
        pin_id = self.selected_gpio_pin_id()
        if not DEVICE_ALLOW_GENERAL_GPIO_WRITE and pin_id != BOARD_LED_PIN_ID:
            self.unsupported_feature(
                "GPIO toggle",
                f"{DEVICE_LABEL} firmware allows GPIO toggle only on the board LED.",
            )
            return
        if pin_id == BOARD_LED_PIN_ID:
            self.safe_command(self._toggle_board_led)
            return
        self.safe_command(lambda: self.client.command_gpio_toggle(pin_id))

    def gpio_config_selected(self):
        pin_id = self.selected_gpio_pin_id()
        pull = GPIO_PULLS.get(self.gpio_pull_var.get(), 0)
        self.safe_command(
            lambda: self.client.command_gpio_config(
                pin_id,
                self.gpio_input_var.get(),
                self.gpio_output_var.get(),
                pull,
            )
        )

    def selected_analog_pin_id(self) -> int:
        return BOARD_PINS.get(self.analog_pin_var.get(), self.selected_gpio_pin_id())

    def analog_read_selected(self):
        self.safe_command(lambda: self.client.command_gpio_analog_read(self.selected_analog_pin_id()))

    def gpio_pwm_selected(self):
        pin_id = self.selected_gpio_pin_id()
        duty = int(self.gpio_pwm_duty_var.get())
        period = int(self.gpio_pwm_period_var.get())
        self.safe_command(lambda: self.client.command_gpio_pwm(pin_id, duty, period))

    def gpio_pwm_off_selected(self):
        self.safe_command(lambda: self.client.command_gpio_pwm_off(self.selected_gpio_pin_id()))

    def uart_status(self):
        self.safe_command(self.client.command_uart_status)

    def uart_ping(self):
        sequence = int(self.uart_ping_seq_var.get()) & 0xFFFF
        self.uart_ping_seq_var.set((sequence + 1) & 0xFFFF)
        self.safe_command(lambda: self.client.command_uart_ping(sequence))

    def uart_set_baud(self):
        if not UART_SET_BAUD_SUPPORTED:
            self.unsupported_feature(
                "UART baud",
                "Baud-rate change is not supported on the ESP32-C3 USB Serial/JTAG transport.",
            )
            return
        index = self.selected_uart_baud_index()
        baud = FIRMWARE_BAUDS[index]
        if not messagebox.askyesno(
            "Change firmware baud",
            f"The device will switch to {baud} baud after acknowledging the command. Continue?",
        ):
            return
        self.pending_gui_baud = baud
        self.safe_command(lambda: self.client.command_uart_set_baud(index))

    def apply_pending_gui_baud(self):
        if self.pending_gui_baud is None:
            return
        baud = self.pending_gui_baud
        self.pending_gui_baud = None
        if self.client.is_open:
            try:
                self.client.serial.baudrate = baud
                self.baud_var.set(str(baud))
                self.log(f"Local serial baud switched to {baud}")
            except Exception as exc:
                self.log(f"Local serial baud switch failed: {exc}")

    def rf_read_status(self):
        self.safe_command(self.client.command_rfsdk_status)

    def rf_apply_power(self):
        label = self.rf_power_var.get()
        value = RF_POWER_VALUE_BY_LABEL.get(label)
        if value is None:
            messagebox.showerror("RF power", "Select a known RF power value")
            return
        self.safe_command(lambda: self.client.command_rf_power(value))

    def rf_apply_cap(self):
        if not RF_CAP_SUPPORTED:
            self.unsupported_feature(
                "XTAL cap",
                "Crystal-capacitance override is not supported by the current ESP32-C3 firmware.",
            )
            return
        value = max(0, min(63, int(self.rf_cap_var.get())))
        self.safe_command(lambda: self.client.command_rf_cap(value))

    def rf_apply_channels(self):
        if not RF_CHANNELS_SUPPORTED:
            self.unsupported_feature(
                "Scan channels",
                "Custom primary-channel selection is not exposed by the current ESP32-C3 firmware.",
            )
            return
        channels = [int(self.rf_ch0_var.get()), int(self.rf_ch1_var.get()), int(self.rf_ch2_var.get())]
        if any(channel < 0 or channel > 39 for channel in channels):
            messagebox.showerror("Primary channels", "Channels must be between 0 and 39")
            return
        self.safe_command(lambda: self.client.command_rf_channels(*channels))

    def rf_apply_coded_min(self):
        units = max(1, min(200, int(round(int(self.coded_min_ms_var.get()) / 10))))
        self.safe_command(lambda: self.client.command_coded_min_window(units))

    def rf_default_channels(self):
        self.rf_ch0_var.set(37)
        self.rf_ch1_var.set(38)
        self.rf_ch2_var.set(39)
        self.rf_apply_channels()

    def rf_high_power(self):
        self.rf_power_var.set(RF_POWER_LABEL_BY_VALUE[15])
        self.rf_apply_power()

    def rf_zero_dbm(self):
        self.rf_power_var.set(RF_POWER_LABEL_BY_VALUE[8])
        self.rf_apply_power()

    def process_events(self):
        processed = 0
        while processed < 200:
            try:
                event_type, payload = self.events.get_nowait()
            except queue.Empty:
                break
            processed += 1
            if event_type == "rx":
                self.handle_rx(payload)
            elif event_type == "tx":
                self.log(format_tx_log(payload))
            elif event_type == "log":
                self.log(payload)
            elif event_type == "error":
                self.status_var.set("Serial error")
                self.log(payload)
                self._sync_connection_buttons()
        self.after(50, self.process_events)

    def handle_rx(self, event):
        if isinstance(event, AdvPacket):
            self._flush_crc_discards()
            self.handle_advertisement(event)
        elif isinstance(event, CommandResponse):
            self._flush_crc_discards()
            self.handle_response(event)
        elif isinstance(event, tuple) and event[0] == "crc_error":
            self._record_crc_discard(event[1])

    def _record_crc_discard(self, byte_value: int):
        """Coalesce parser CRC-discard bytes into a single log message.

        Per-byte logging floods the console when the device emits any
        non-protocol text (e.g. ESP32-C3 ROM bootloader output after a
        reset). We buffer the bytes and flush after a short quiet period
        or when a valid frame interrupts the stream.
        """
        self._crc_discard_buf.append(byte_value & 0xFF)
        if self._crc_discard_flush_after is not None:
            try:
                self.after_cancel(self._crc_discard_flush_after)
            except Exception:
                pass
        # 150 ms quiet window: noise bursts within that gap stay coalesced.
        self._crc_discard_flush_after = self.after(150, self._flush_crc_discards)

    def _flush_crc_discards(self):
        if self._crc_discard_flush_after is not None:
            try:
                self.after_cancel(self._crc_discard_flush_after)
            except Exception:
                pass
            self._crc_discard_flush_after = None
        if not self._crc_discard_buf:
            return
        data = bytes(self._crc_discard_buf)
        self._crc_discard_buf = bytearray()
        ascii_view = "".join(chr(b) if 32 <= b <= 126 else "." for b in data)
        # Heuristic: if the bytes look like ROM bootloader output, label it as a reset.
        signature_present = b"ESP-ROM" in data or b"rst:0x" in data or b"boot:0x" in data or b"load:0x" in data
        printable_ratio = sum(1 for b in data if 32 <= b <= 126 or b in (0x0A, 0x0D)) / len(data)
        if signature_present:
            self.log(f"RX device reset detected ({len(data)} bytes ROM/boot output): '{ascii_view}'")
            # The chip has rebooted: any prior scan/conn/TX-adv state is gone.
            # Resync the GUI so user controls don't stay locked.
            self._on_device_reset_detected()
        elif printable_ratio > 0.8:
            self.log(f"RX dropped {len(data)} bytes (text-like): '{ascii_view}'")
        else:
            preview = " ".join(f"{b:02X}" for b in data[:16])
            if len(data) > 16:
                preview += " ..."
            self.log(f"RX dropped {len(data)} bytes (non-frame, CRC mismatch): {preview}")

    def _on_device_reset_detected(self):
        """Called when the parser sees ROM bootloader output mid-session.
        Clears any pending watchdog and force-resets transient state so the
        GUI does not stay locked into 'connecting' or 'scanning' phantoms."""
        self._cancel_scan_response_watchdog()
        self._cancel_conn_timeout()
        self.scan_state_var.set("Stopped")
        self.last_started_scan_config = None

    def handle_response(self, response: CommandResponse):
        if response.command == CMD_ID_INFO:
            self.version_var.set(firmware_version_label(response.index))
            if response.data_len == 6:
                self.local_mac_var.set(mac_from_wire(response.data))
            self.log(f"RX INFO fw={firmware_version_label(response.index)} local_mac={self.local_mac_var.get()}")
        elif response.command == CMD_ID_VERSION:
            self.handle_version_response(response)
        elif response.command == CMD_ID_CLRM:
            self.list_capacity_var.set(str(response.index))
            self.log(f"RX CLRM capacity={response.index}")
        elif response.command in (CMD_ID_WMAC, CMD_ID_BMAC):
            mode = "white" if response.command == CMD_ID_WMAC else "black"
            mac = mac_from_wire(response.data) if response.data_len == 6 else "-"
            self.log(f"RX add {mode} count={response.index} mac={mac}")
        elif response.command == CMD_ID_SCAN:
            # Any CMD_ID_SCAN response from the firmware cancels the watchdog.
            self._cancel_scan_response_watchdog()
            if response.data_len >= 1 and response.data[0] == 0:
                self.scan_state_var.set("Stopped")
                self.log("RX scan disabled")
            elif response.data_len in (3, 5):
                config = ScanConfig.from_payload(response.data)
                self.scan_state_var.set(config.describe())
                self.log(f"RX scan enabled list_count={response.index} {config.describe()}")
            else:
                self.log(f"RX scan response data={bytes_to_hex(response.data)}")
        elif response.command == CMD_ID_PRNT:
            self.log(f"RX debug: {response.data.decode(errors='replace')}")
        elif response.command == CMD_ID_GPIO:
            self.handle_gpio_response(response)
        elif response.command == CMD_ID_UART:
            self.handle_uart_response(response)
        elif response.command == CMD_ID_RFSDK:
            self.handle_rfsdk_response(response)
        elif response.command == CMD_ID_TXADV:
            self.handle_txadv_response(response)
        elif response.command == CMD_ID_CONN:
            self.handle_conn_response(response)
        elif response.command == CMD_ID_TXDATA:
            self.handle_txdata_response(response)
        elif response.command == CMD_ID_RXDATA:
            self.handle_rxdata_response(response)
        elif response.command == CMD_ID_VBAT:
            self.handle_vbat_response(response)
        elif response.command == CMD_ID_GPIOEVT:
            self.handle_gpioevt_response(response)
        else:
            self.log(f"RX {response.command_name} idx={response.index} data={bytes_to_hex(response.data)}")

    def handle_version_response(self, response: CommandResponse):
        self.version_var.set(firmware_version_label(response.index))
        if response.data_len >= 6:
            hw_version, cert, structure, major, minor, patch = response.data[:6]
            hw_label = HW_VERSION_LABELS.get(hw_version, f"0x{hw_version:02X}")
            sdk_label = sdk_version_label(cert, structure, major, minor, patch)
            self.hw_version_var.set(hw_label)
            self.sdk_version_var.set(sdk_label)
            self.log(f"RX VERSION hw={hw_label} fw={firmware_version_label(response.index)} sdk={sdk_label}")
        else:
            self.log(f"RX VERSION data={bytes_to_hex(response.data)}")

    def handle_vbat_response(self, response: CommandResponse):
        status = self.status_name(response.index)
        if not DEVICE_SUPPORTS_VBAT and response.index == 3:
            self.vbat_var.set("Unsupported")
            self.temp_var.set("-")
            self.log("RX VBAT unsupported on ESP32-C3 firmware")
            return
        if response.data_len >= 2:
            batt_mv = response.data[0] | (response.data[1] << 8)
            temp_c = None
            if response.data_len >= 4:
                temp_c = response.data[2] | (response.data[3] << 8)
                if temp_c & 0x8000:
                    temp_c -= 0x10000
                if temp_c == -32768:
                    temp_c = None
            if response.index == 0:
                self.vbat_var.set(f"{batt_mv} mV")
                self.temp_var.set(f"{temp_c} C" if temp_c is not None else "unavailable")
                if temp_c is not None:
                    self.log(f"RX VBAT {batt_mv} mV temp={temp_c} C")
                else:
                    self.log(f"RX VBAT {batt_mv} mV temp=unavailable")
            else:
                self.vbat_var.set(status)
                self.temp_var.set("-")
                self.log(f"RX VBAT {status} data={bytes_to_hex(response.data)}")
        else:
            self.vbat_var.set(status)
            self.temp_var.set("-")
            self.log(f"RX VBAT {status} data={bytes_to_hex(response.data)}")

    def handle_gpioevt_response(self, response: CommandResponse):
        """Handle both spontaneous GPIO edge events and command acks.

        Convention (matches firmware):
        - If ``index`` has bit 0x80 set → spontaneous event. The low 5 bits
          encode the pin id; ``data[0]`` is pin, ``data[1]`` is new level,
          ``data[2..5]`` is a 32-bit ms timestamp (little-endian).
        - Otherwise → ack of an enable/disable/query/clear request. ``data[0]``
          is the affected pin, ``data[1..4]`` is the 32-bit "armed pins" bitmap.
        """
        if response.index & GPIOEVT_EVENT_FLAG:
            # Spontaneous edge event.
            if response.data_len < 6:
                self.log(f"RX GPIOEVT event (truncated) data={bytes_to_hex(response.data)}")
                return
            pin = response.data[0]
            level = response.data[1]
            ts_ms = (
                response.data[2]
                | (response.data[3] << 8)
                | (response.data[4] << 16)
                | (response.data[5] << 24)
            )
            pin_name = BOARD_PIN_NAMES.get(pin, f"GPIO{pin}")
            line = f"[{ts_ms:>10} ms] {pin_name} → {level}  ({'RISE' if level else 'FALL'})"
            self._gpioevt_log_line(line)
            self.log(f"RX GPIOEVT event pin={pin_name} level={level} t={ts_ms}ms")
            return
        # Ack path: data[0]=affected pin (or 0), data[1..4]=enabled bitmap.
        status = self.status_name(response.index)
        if response.data_len >= 5:
            mask = (
                response.data[1]
                | (response.data[2] << 8)
                | (response.data[3] << 16)
                | (response.data[4] << 24)
            )
            self.gpioevt_enabled_mask_var.set(f"0x{mask:08X}")
            self.log(f"RX GPIOEVT ack status={status} pin=0x{response.data[0]:02X} armed_mask=0x{mask:08X}")
        elif response.data_len >= 4:
            # QUERY response: data[0..3] is the bitmap directly.
            mask = (
                response.data[0]
                | (response.data[1] << 8)
                | (response.data[2] << 16)
                | (response.data[3] << 24)
            )
            self.gpioevt_enabled_mask_var.set(f"0x{mask:08X}")
            self.log(f"RX GPIOEVT query status={status} armed_mask=0x{mask:08X}")
        else:
            self.log(f"RX GPIOEVT status={status} data={bytes_to_hex(response.data)}")

    def handle_gpio_response(self, response: CommandResponse):
        status = self.status_name(response.index)
        response_pin_id = response.data[1] if response.data_len >= 2 else None
        if response.data_len >= 6:
            op, pin_id, value, led_capable, board_mask_lo, board_mask_hi = response.data[:6]
            board_mask = board_mask_lo | (board_mask_hi << 8)
            pin_name = BOARD_PIN_NAMES.get(pin_id, f"pin {pin_id}")
            if op == 7:
                analog_raw = value | (led_capable << 8)
                self.analog_value_var.set(str(analog_raw))
                self.set_board_labels(board_mask)
                self.refresh_board_labels_from_cache()
                self.log(
                    f"RX GPIO {status} analog {pin_name} raw={analog_raw} board_mask=0x{board_mask:04X}"
                )
            elif pin_id in self.gpio_pin_state_vars:
                if op == 5:
                    self.gpio_pin_state_vars[pin_id].set("1" if value else "0")
                elif value in (0, 1):
                    self.gpio_pin_state_vars[pin_id].set(str(value))
                else:
                    self.gpio_pin_state_vars[pin_id].set("1" if value else "0")
                self.set_board_labels(board_mask)
                self.refresh_board_labels_from_cache()
                self.log(
                    f"RX GPIO {status} op={op} {pin_name} value={value} led_capable={led_capable} board_mask=0x{board_mask:04X}"
                )
        else:
            self.log(f"RX GPIO {status} data={bytes_to_hex(response.data)}")
        self.continue_gpio_read_batch(response_pin_id)

    def handle_uart_response(self, response: CommandResponse):
        status = self.status_name(response.index)
        if response.data_len >= 6 and response.data[0] == 1:
            sequence = response.data[1] | (response.data[2] << 8)
            pattern = response.data[3]
            inverted = response.data[4]
            baud_index = response.data[5]
            self.uart_status_var.set(f"ping seq={sequence} pattern=0x{pattern:02X} baud_index={baud_index}")
            self.log(f"RX UART {status} ping seq={sequence} pattern=0x{pattern:02X} inv=0x{inverted:02X}")
            return
        if response.data_len >= 6:
            op, baud_index, count = response.data[:3]
            baud = response.data[3] | (response.data[4] << 8) | (response.data[5] << 16)
            if op == 2 and response.index == 3:
                self.uart_status_var.set(f"{status}: fixed USB Serial/JTAG baud={baud}")
            else:
                self.uart_status_var.set(f"index={baud_index}/{count} baud={baud}")
            if 0 <= baud_index < len(FIRMWARE_BAUDS):
                self.uart_baud_index_var.set(f"{baud_index} - {FIRMWARE_BAUDS[baud_index]}")
            self.log(f"RX UART {status} op={op} baud_index={baud_index} baud={baud}")
            if op == 2:
                if response.index == 0 and self.pending_gui_baud:
                    self.after(600, self.apply_pending_gui_baud)
                elif response.index != 0:
                    self.pending_gui_baud = None
            return
        self.log(f"RX UART {status} data={bytes_to_hex(response.data)}")

    def handle_rfsdk_response(self, response: CommandResponse):
        status = self.status_name(response.index)
        if response.data_len >= 6:
            power, cap, ch0, ch1, ch2, coded_units = response.data[:6]
            label = RF_POWER_LABEL_BY_VALUE.get(power, f"raw 0x{power:02X}")
            self.rf_power_var.set(label)
            if RF_CAP_SUPPORTED and cap != 0xFF:
                self.rf_cap_var.set(cap)
                self.rf_cap_status_var.set(f"active {cap}")
            else:
                self.rf_cap_status_var.set(f"unsupported (raw 0x{cap:02X})")
            self.rf_ch0_var.set(ch0)
            self.rf_ch1_var.set(ch1)
            self.rf_ch2_var.set(ch2)
            self.coded_min_ms_var.set(coded_units * 10)
            self.rf_status_var.set(
                f"{status}: power={label}, xtal_cap=unsupported, channels={ch0}/{ch1}/{ch2}, coded_min={coded_units * 10} ms"
            )
            self.log(
                f"RX RFSDK {status} power={label} xtal_cap=0x{cap:02X} channels={ch0}/{ch1}/{ch2} coded_min={coded_units * 10} ms"
            )
        else:
            self.log(f"RX RFSDK {status} data={bytes_to_hex(response.data)}")

    def handle_txadv_response(self, response: CommandResponse):
        status = self.status_name(response.index)
        if response.data_len >= 4:
            running_phy = response.data[0]
            interval_units = response.data[1] | (response.data[2] << 8)
            adv_len = response.data[3]
            interval_ms = round(interval_units * 0.625, 1)
            if running_phy == 0:
                phy_label = "stopped"
            else:
                phy_label = TXADV_PHY_NAMES.get(running_phy - 1, f"phy {running_phy - 1}")
            state = (f"running: {phy_label}, interval={interval_ms} ms, adv_len={adv_len}"
                     if running_phy else "stopped")
            self.txadv_status_var.set(f"{status} – {state}")
            self.log(f"RX TXADV {status} phy={running_phy}({phy_label}) interval={interval_ms}ms adv_len={adv_len}")
        else:
            self.log(f"RX TXADV {status} data={bytes_to_hex(response.data)}")

    def handle_conn_response(self, response: CommandResponse):
        STATE_NAMES = {0: "Idle", 1: "Connecting", 2: "Connected"}
        state_code = response.data[0] if response.data_len >= 1 else response.index
        state_name = STATE_NAMES.get(state_code, f"state {state_code}")
        self.conn_state_var.set(state_name)
        if response.data_len >= 6:
            peer_type = response.data[1]
            peer_mac = bytes(response.data[2:6]) + b'\x00\x00'  # first 4 bytes
            handle = response.data[2] | (response.data[3] << 8) if response.data_len >= 4 else 0
        handle_lo = response.data[2] if response.data_len >= 3 else 0
        handle_hi = response.data[3] if response.data_len >= 4 else 0
        conn_handle = handle_lo | (handle_hi << 8)
        interval_lo = response.data[4] if response.data_len >= 5 else 0
        interval_hi = response.data[5] if response.data_len >= 6 else 0
        interval_125us = interval_lo | (interval_hi << 8)
        if state_code == 2 and conn_handle:
            self.conn_handle_var.set(f"0x{conn_handle:04X}")
            if interval_125us:
                self.conn_interval_var.set(f"{round(interval_125us * 1.25, 2)} ms")
        elif state_code == 0:
            self.conn_handle_var.set("-")
            self.conn_interval_var.set("-")
        status_lbl = self.status_name(response.index)
        self.log(f"RX CONN status={status_lbl}(0x{response.index:02X}) state={state_name} handle=0x{conn_handle:04X} interval={interval_125us * 1.25:.2f}ms data={bytes_to_hex(response.data)}")

        # Persist the most informative outcome in the BLE Conn tab.
        # Order of preference:
        #   1) any non-OK status — keep visible until the next non-OK or until reset
        #   2) "Connected" once we reach state=Connected
        #   3) otherwise "OK – Idle" so the user knows commands are flowing
        timestamp = time.strftime("%H:%M:%S")
        if response.index != 0:
            self.conn_last_result_var.set(f"{status_lbl} ({state_name})  @{timestamp}")
            try:
                self.conn_last_result_label.configure(foreground="red")
            except Exception:
                pass
        elif state_code == 2:
            # Successful connection — kill any pending host-side timeout.
            self._cancel_conn_timeout()
            self.conn_last_result_var.set(f"Connected  @{timestamp}")
            try:
                self.conn_last_result_label.configure(foreground="dark green")
            except Exception:
                pass
        # If status is OK and state is Idle/Connecting we leave the last-result
        # field alone — that way a previous "Denied" message stays visible
        # instead of being clobbered by the cleanup ESP_GATTC_DISCONNECT_EVT
        # which also returns OK.

    def handle_txdata_response(self, response: CommandResponse):
        status = self.status_name(response.index)
        if response.data_len >= 4:
            att_handle = response.data[0] | (response.data[1] << 8)
            data_len = response.data[2]
            ble_status = response.data[3]
            self.log(f"RX TXDATA {status} att_handle=0x{att_handle:04X} len={data_len} ble_sts=0x{ble_status:02X}")
        else:
            self.log(f"RX TXDATA {status} data={bytes_to_hex(response.data)}")

    def handle_rxdata_response(self, response: CommandResponse):
        # opcode (ATT) is in index field; data: [att_handle_lo, att_handle_hi, value_len, value...]
        opcode = response.index
        if response.data_len >= 3:
            att_handle = response.data[0] | (response.data[1] << 8)
            value_len = response.data[2]
            value = bytes(response.data[3:3 + value_len]) if response.data_len >= 3 + value_len else bytes(response.data[3:])
            opcode_name = "IND" if opcode == 0x1D else "NOTI"
            hex_val = bytes_to_hex(value)
            line = f"ATT {opcode_name} handle=0x{att_handle:04X} len={len(value)} data={hex_val}\n"
            self.log(f"RX RXDATA {opcode_name} att=0x{att_handle:04X} {hex_val}")
            if self.conn_rxdata_log is not None:
                self.conn_rxdata_log.config(state=tk.NORMAL)
                self.conn_rxdata_log.insert(tk.END, line)
                self.conn_rxdata_log.see(tk.END)
                self.conn_rxdata_log.config(state=tk.DISABLED)
        else:
            self.log(f"RX RXDATA op=0x{opcode:02X} data={bytes_to_hex(response.data)}")

    def handle_advertisement(self, packet: AdvPacket):
        self.total_count += 1
        if packet.phys == 0:
            self.legacy_count += 1
        if packet.primary_phy == 1 or packet.secondary_phy == 1:
            self.one_m_count += 1
        if packet.is_coded:
            self.coded_count += 1
        self.last_packet_time = packet.timestamp
        self.advertisements.append(self.packet_to_row(packet))
        self.update_counter_labels()
        self.update_mac_stats(packet)
        if not self.pause_table_var.get() and self.packet_matches_display_filter(packet):
            self.insert_adv_row(packet)

    def packet_matches_display_filter(self, packet: AdvPacket) -> bool:
        text = self.monitor_filter_var.get().strip().upper()
        if not text:
            return True
        haystack = f"{packet.mac} {bytes_to_hex(packet.payload)} {payload_ascii(packet.payload).upper()}"
        return text in haystack.upper()

    def packet_to_row(self, packet: AdvPacket) -> dict:
        return {
            "time": time.strftime("%H:%M:%S", time.localtime(packet.timestamp)),
            "timestamp": f"{packet.timestamp:.3f}",
            "rssi": packet.rssi,
            "event": f"0x{packet.event_type:02X}",
            "address_type": packet.addr_label(),
            "phys": packet.phy_label(),
            "phys_raw": f"0x{packet.phys:02X}",
            "mac": packet.mac,
            "payload_len": packet.payload_len,
            "payload_hex": bytes_to_hex(packet.payload),
            "payload_ascii": payload_ascii(packet.payload),
        }

    def insert_adv_row(self, packet: AdvPacket):
        row = self.packet_to_row(packet)
        item = self.adv_tree.insert(
            "",
            tk.END,
            values=(
                row["time"],
                row["rssi"],
                row["event"],
                row["address_type"],
                row["phys"],
                row["mac"],
                row["payload_len"],
                row["payload_hex"],
                row["payload_ascii"],
            ),
        )
        if packet.is_coded:
            self.adv_tree.item(item, tags=("coded",))
            self.adv_tree.tag_configure("coded", background="#e9f7ef")
        if self.autoscroll_var.get():
            self.adv_tree.see(item)
        if len(self.adv_tree.get_children()) > 2500:
            self.adv_tree.delete(self.adv_tree.get_children()[0])

    def update_counter_labels(self):
        self.total_var.set(str(self.total_count))
        self.legacy_var.set(str(self.legacy_count))
        self.one_m_var.set(str(self.one_m_count))
        self.coded_var.set(str(self.coded_count))
        if self.last_packet_time:
            self.last_seen_var.set(time.strftime("%H:%M:%S", time.localtime(self.last_packet_time)))

    def update_mac_stats(self, packet: AdvPacket):
        stats = self.stats_by_mac.setdefault(
            packet.mac,
            {
                "count": 0,
                "last_rssi": packet.rssi,
                "min_rssi": packet.rssi,
                "max_rssi": packet.rssi,
                "last_phy": packet.phy_label(),
                "last_seen": packet.timestamp,
                "item": None,
            },
        )
        stats["count"] += 1
        stats["last_rssi"] = packet.rssi
        stats["min_rssi"] = min(stats["min_rssi"], packet.rssi)
        stats["max_rssi"] = max(stats["max_rssi"], packet.rssi)
        stats["last_phy"] = packet.phy_label()
        stats["last_seen"] = packet.timestamp
        values = (
            packet.mac,
            stats["count"],
            stats["last_rssi"],
            stats["min_rssi"],
            stats["max_rssi"],
            stats["last_phy"],
            time.strftime("%H:%M:%S", time.localtime(stats["last_seen"])),
        )
        if stats["item"] is None:
            stats["item"] = self.stats_tree.insert("", tk.END, values=values)
        else:
            self.stats_tree.item(stats["item"], values=values)

    def reset_monitor(self):
        self.total_count = 0
        self.legacy_count = 0
        self.one_m_count = 0
        self.coded_count = 0
        self.last_packet_time = None
        self.advertisements.clear()
        self.stats_by_mac.clear()
        self.clear_adv_table()
        self.stats_tree.delete(*self.stats_tree.get_children())
        self.update_counter_labels()
        self.last_seen_var.set("-")

    def clear_adv_table(self):
        self.adv_tree.delete(*self.adv_tree.get_children())

    def export_csv(self):
        if not self.advertisements:
            messagebox.showinfo("Export CSV", "No advertisements to export")
            return
        default_name = f"adv2uart_capture_{time.strftime('%Y%m%d_%H%M%S')}.csv"
        path = filedialog.asksaveasfilename(
            title="Export advertisements",
            initialfile=default_name,
            defaultextension=".csv",
            filetypes=(("CSV files", "*.csv"), ("All files", "*.*")),
        )
        if not path:
            return
        fieldnames = [
            "time",
            "timestamp",
            "rssi",
            "event",
            "address_type",
            "phys",
            "phys_raw",
            "mac",
            "payload_len",
            "payload_hex",
            "payload_ascii",
        ]
        with Path(path).open("w", newline="") as csv_file:
            writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(self.advertisements)
        self.log(f"Exported {len(self.advertisements)} advertisements to {path}")

    def log(self, text: str):
        stamp = time.strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{stamp}] {text}\n")
        self.log_text.see(tk.END)

    def send_version_info(self):
        """Query only HW / FW / SDK version (CMD_ID_VERSION)."""
        self.safe_command(self.client.command_version_info)

    def send_vbat(self):
        """Query the current device VBAT / 3V3 rail (CMD_ID_VBAT)."""
        if not DEVICE_SUPPORTS_VBAT:
            self.vbat_var.set("Unsupported")
            self.temp_var.set("-")
            self.log("VBAT is not supported by the current ESP32-C3 firmware")
            return
        self.safe_command(self.client.command_vbat)

    def gpio_read_all(self):
        """Read all board GPIO pins sequentially and update the pin-states grid."""
        self.safe_command(lambda: self.start_gpio_read_batch(sorted(BOARD_PIN_NAMES.keys())))

    def led_query_state(self):
        """Refresh LED state by reading all board GPIO pins through CMD_ID_GPIO."""
        self.gpio_read_all()

    def rf_coded_50pct_of_window(self):
        """Set Coded PHY min window to 50 % of the current scan window."""
        window = float(self.window_ms_var.get())
        coded = max(10, int(round(window * 0.5 / 10)) * 10)
        self.coded_min_ms_var.set(coded)
        self.rf_apply_coded_min()

    # ---- TX Adv actions ------------------------------------------------

    def _txadv_parse_data(self) -> bytes | None:
        """Parse hex string from txadv_data_var; return bytes or None on error."""
        raw = self.txadv_data_var.get().replace(" ", "").replace(":", "").strip()
        if not raw:
            return b""
        try:
            return bytes.fromhex(raw)
        except ValueError:
            messagebox.showerror("TX Adv", "ADV data is not valid hex.")
            return None

    def _txadv_phy_value(self) -> int:
        lbl = self.txadv_phy_var.get()
        for label, val in TXADV_PHY_OPTIONS:
            if label == lbl:
                return val
        return 0

    def txadv_start(self):
        data = self._txadv_parse_data()
        if data is None:
            return
        if len(data) > 31:
            messagebox.showerror("TX Adv", "ADV data must be ≤ 31 bytes.")
            return
        phy = self._txadv_phy_value()
        interval_ms = float(self.txadv_interval_ms_var.get())
        interval_units = max(32, int(round(interval_ms / 0.625)))
        self.safe_command(lambda: self.client.command_txadv_start(phy, interval_units, data))

    def txadv_stop(self):
        self.safe_command(self.client.command_txadv_stop)

    def txadv_query(self):
        self.safe_command(self.client.command_txadv_status)

    def _txadv_preset_ibeacon(self):
        """Load a minimal iBeacon-like payload into the hex entry."""
        beacon = bytes([
            0x02, 0x01, 0x06,                    # Flags: LE General Discoverable, BR/EDR Not Supported
            0x1A, 0xFF,                           # Mfr data len=26
            0x4C, 0x00,                           # Company: Apple
            0x02, 0x15,                           # iBeacon type + length
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55,  # UUID (first 6 bytes)
            0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,  # UUID (next 6 bytes)
            0xCC, 0xDD, 0xEE, 0xFF,              # UUID (last 4 bytes)
            0x00, 0x01,                           # Major
            0x00, 0x02,                           # Minor
            0xC4,                                 # TX power
        ])
        self.txadv_data_var.set(bytes_to_hex(beacon))

    def _txadv_preset_name(self):
        """Load a Local Name advertising payload for 'TB-03F-KIT'."""
        name = b"TB-03F-KIT"
        flags = bytes([0x02, 0x01, 0x06])
        name_ad = bytes([len(name) + 1, 0x09]) + name
        self.txadv_data_var.set(bytes_to_hex(flags + name_ad))

    # --- BLE Conn actions ---

    def _parse_mac_entry(self) -> bytes:
        """Parse the peer MAC entry field. Returns 6-byte little-endian bytes or raises ValueError."""
        raw = self.conn_peer_var.get().strip().replace(":", "").replace("-", "").replace(" ", "")
        if len(raw) != 12:
            raise ValueError(f"Invalid MAC: '{self.conn_peer_var.get()}'")
        mac_bytes = bytes(int(raw[i:i+2], 16) for i in range(0, 12, 2))
        return bytes(reversed(mac_bytes))  # wire order = little-endian (reversed)

    def conn_connect(self):
        try:
            peer_addr = self._parse_mac_entry()
        except ValueError as e:
            self.log(f"CONN connect error: {e}")
            return
        peer_type = 0 if self.conn_peer_type_var.get().lower().startswith("pub") else 1
        phy_lbl = self.conn_phy_var.get()
        phy = next((v for lbl, v in CONN_PHY_OPTIONS if lbl == phy_lbl), 0)
        # Reset the BLE Conn "Last result" line so the user sees a fresh
        # attempt; the next response will update it.
        self.conn_last_result_var.set("Connect requested…")
        try:
            self.conn_last_result_label.configure(foreground="gray")
        except Exception:
            pass
        self._arm_conn_timeout()
        self.safe_command(lambda: self.client.command_conn_connect(peer_addr, peer_type, phy))

    def _arm_conn_timeout(self):
        """Schedule the host-side connect timeout. 0 disables (firmware decides)."""
        self._cancel_conn_timeout()
        try:
            seconds = max(0, int(self.conn_timeout_s_var.get()))
        except Exception:
            seconds = 0
        if seconds <= 0:
            return
        self._conn_timeout_after = self.after(seconds * 1000, self._conn_timeout_fired)

    def _cancel_conn_timeout(self):
        if self._conn_timeout_after is not None:
            try:
                self.after_cancel(self._conn_timeout_after)
            except Exception:
                pass
            self._conn_timeout_after = None

    def _conn_timeout_fired(self):
        self._conn_timeout_after = None
        # If we're already connected, the user got there in time — nothing to do.
        if self.conn_state_var.get() == "Connected":
            return
        # Otherwise send a Cancel to abort the in-flight connect (no-op if the
        # controller has already given up). Surface the timeout in the UI.
        try:
            seconds = int(self.conn_timeout_s_var.get())
        except Exception:
            seconds = 0
        timestamp = time.strftime("%H:%M:%S")
        msg = f"Timeout after {seconds}s  @{timestamp}"
        self.conn_last_result_var.set(msg)
        try:
            self.conn_last_result_label.configure(foreground="red")
        except Exception:
            pass
        self.log(f"BLE Conn: host-side connect timeout after {seconds} s — sending CANCEL")
        self.safe_command(self.client.command_conn_cancel)

    def conn_disconnect(self):
        self._cancel_conn_timeout()
        self.safe_command(self.client.command_conn_disconnect)

    def conn_cancel(self):
        self._cancel_conn_timeout()
        self.safe_command(self.client.command_conn_cancel)

    def conn_status(self):
        self.safe_command(self.client.command_conn_status)

    def conn_txdata(self):
        try:
            handle = int(self.txdata_handle_var.get().strip(), 16)
        except ValueError:
            self.log("TXDATA: invalid ATT handle (expected hex)")
            return
        try:
            data = bytes.fromhex(self.txdata_data_var.get().replace(" ", ""))
        except ValueError:
            self.log("TXDATA: invalid hex data")
            return
        self.safe_command(lambda: self.client.command_txdata(handle, data))

    def _conn_rxdata_clear(self):
        if self.conn_rxdata_log is not None:
            self.conn_rxdata_log.config(state=tk.NORMAL)
            self.conn_rxdata_log.delete("1.0", tk.END)
            self.conn_rxdata_log.config(state=tk.DISABLED)

    def on_close(self):
        try:
            if self.client.is_open:
                self.client.command_stop_scan()
                time.sleep(0.05)
        except Exception:
            pass
        self.client.close()
        self.destroy()


def main():
    parser = argparse.ArgumentParser(description="ADV_BLE2UART test console")
    parser.add_argument(
        "--device",
        choices=tuple(DEVICE_PROFILES),
        default=DEFAULT_DEVICE_PROFILE_ID,
        help="target device profile (default: esp32-c3)",
    )
    parser.add_argument(
        "-p",
        "--port",
        dest="serial_port",
        help="serial port to preselect in the GUI",
        default="",
        metavar="PORT",
    )
    parser.add_argument(
        "-m",
        "--maximized",
        action="store_true",
        help="start the GUI maximized",
    )
    parser.add_argument(
        "--connect",
        action="store_true",
        help="connect automatically at startup using the selected port",
    )
    args = parser.parse_args()

    apply_device_profile(args.device)
    app = AdvBle2UartGui(
        initial_port=args.serial_port,
        start_maximized=args.maximized,
        auto_connect=args.connect,
    )
    try:
        app.mainloop()
    except KeyboardInterrupt:
        try:
            app.on_close()
        except Exception:
            try:
                app.destroy()
            except Exception:
                pass
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())