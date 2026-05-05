#!/usr/bin/env python3

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

GUI_BOOTSTRAP_DELAY_MS = 1000
GUI_SERIAL_TIMEOUT_S = 0.3

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

BOARD_PINS = {
    "Key/SWS PA7": 0x07,
    "Side yellow PB4": 0x14,
    "Side white PB5": 0x15,
    "RGB blue PC2": 0x22,
    "RGB red PC3": 0x23,
    "RGB green PC4": 0x24,
}
BOARD_PIN_NAMES = {value: key for key, value in BOARD_PINS.items()}
BOARD_PIN_MASK_INDEX = {pin_id: index for index, pin_id in enumerate(sorted(BOARD_PIN_NAMES))}

GPIO_PULLS = {
    "Float": 0,
    "Pull-up 1M": 1,
    "Pull-down 100K": 2,
    "Pull-up 10K": 3,
}

LED_BITS = {
    "Blue PC2": 0x01,
    "Red PC3": 0x02,
    "Green PC4": 0x04,
    "Yellow PB4": 0x08,
    "White PB5": 0x10,
}
LED_ALL_MASK = 0x1F
LED_PIN_CODES = {
    0x01: 0x22,
    0x02: 0x23,
    0x04: 0x24,
    0x08: 0x14,
    0x10: 0x15,
}
LED_PIN_IDS = tuple(LED_PIN_CODES[bit] for bit in LED_PIN_CODES)

HW_VERSION_LABELS = {
    0x10: "0x10 TB-03F KIT",
}

FIRMWARE_BAUDS = (2000000, 921600, 115200)

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
}

RF_POWER_OPTIONS = (
    ("+10.46 dBm VBAT", 0x3F),
    ("+10.01 dBm VBAT", 0x3A),
    ("+7.79 dBm VBAT", 0x29),
    ("+5.13 dBm VBAT", 0x1D),
    ("+3.01 dBm VANT", 0xBF),
    ("+0.04 dBm VANT", 0xA9),
    ("-5.03 dBm VANT", 0x96),
    ("-15.88 dBm VANT", 0x86),
    ("-30 dBm", 0xFF),
)
RF_POWER_LABEL_BY_VALUE = {value: label for label, value in RF_POWER_OPTIONS}
RF_POWER_VALUE_BY_LABEL = {label: value for label, value in RF_POWER_OPTIONS}

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
        self.serial = serial.serial_for_url(port, baudrate=baudrate, timeout=GUI_SERIAL_TIMEOUT_S)
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

    def command_gpio_read(self, pin_id: int = 0x22):
        self.send(bytes((CMD_ID_GPIO, 1, pin_id & 0xFF)))

    def command_gpio_read_all(self):
        """Read all known board GPIO pins sequentially."""
        for pin_id in sorted(BOARD_PIN_NAMES.keys()):
            self.send(bytes((CMD_ID_GPIO, 1, pin_id & 0xFF)))

    def command_gpio_write(self, pin_id: int, value: int):
        self.send(bytes((CMD_ID_GPIO, 2, pin_id & 0xFF, 1 if value else 0)))

    def command_gpio_toggle(self, pin_id: int):
        self.send(bytes((CMD_ID_GPIO, 3, pin_id & 0xFF)))

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
        # op=0x0F is not 0 or 1 → firmware returns CMD_STATUS_ARGS but still sends current state
        self.send(bytes((CMD_ID_TXADV, 0x0F)))

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


class AdvBle2UartGui(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("ADV_BLE2UART Test Console")
        self.geometry("1220x820")
        self.minsize(980, 680)

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

        self._build_variables()
        self._build_ui()
        self.refresh_ports()
        self.apply_long_range_preset()
        self.after(50, self.process_events)
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def _build_variables(self):
        self.port_var = tk.StringVar(value="COM4" if sys.platform.startswith("win") else "/dev/ttyUSB0")
        self.baud_var = tk.StringVar(value="2000000")
        self.status_var = tk.StringVar(value="Disconnected")
        self.local_mac_var = tk.StringVar(value="-")
        self.version_var = tk.StringVar(value="-")
        self.hw_version_var = tk.StringVar(value="-")
        self.sdk_version_var = tk.StringVar(value="-")
        self.list_capacity_var = tk.StringVar(value="-")
        self.scan_state_var = tk.StringVar(value="Stopped")

        self.pulse_reset_var = tk.BooleanVar(value=True)
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
        self.gpio_pin_var = tk.StringVar(value="RGB blue PC2")
        self.gpio_value_var = tk.BooleanVar(value=True)
        self.gpio_input_var = tk.BooleanVar(value=True)
        self.gpio_output_var = tk.BooleanVar(value=True)
        self.gpio_pull_var = tk.StringVar(value="Float")
        self.gpio_pwm_duty_var = tk.IntVar(value=50)    # kept for method compatibility
        self.gpio_pwm_period_var = tk.IntVar(value=1000)
        self.gpio_pin_state_vars = {pin_id: tk.StringVar(value="?") for pin_id in BOARD_PIN_NAMES}
        self.gpio_read_generation = 0
        self.led_blink_count_var = tk.IntVar(value=3)
        self.led_blink_delay_var = tk.IntVar(value=50)
        self.led_blink_generation = 0

        self.uart_status_var = tk.StringVar(value="-")
        self.uart_baud_index_var = tk.StringVar(value="0 - 2000000")
        self.uart_ping_seq_var = tk.IntVar(value=1)
        self.pending_gui_baud = None

        self.rf_power_var = tk.StringVar(value=RF_POWER_OPTIONS[5][0])
        self.rf_cap_var = tk.IntVar(value=0)
        self.rf_cap_status_var = tk.StringVar(value="calibrated/unknown")
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
        self.conn_peer_var = tk.StringVar(value="")
        self.conn_peer_type_var = tk.StringVar(value="Random")
        self.conn_phy_var = tk.StringVar(value=CONN_PHY_OPTIONS[0][0])
        self.conn_interval_var = tk.StringVar(value="-")
        self.conn_handle_var = tk.StringVar(value="-")
        self.txdata_handle_var = tk.StringVar(value="0012")
        self.txdata_data_var = tk.StringVar(value="48 65 6C 6C 6F")
        self.conn_rxdata_log = None  # set when tab is built
        self.last_started_scan_config = None

    def _build_ui(self):
        self.columnconfigure(0, weight=1)
        self.rowconfigure(2, weight=1)

        self._build_serial_bar()
        self._build_device_bar()

        main = ttk.Panedwindow(self, orient=tk.HORIZONTAL)
        main.grid(row=2, column=0, sticky="nsew", padx=8, pady=(0, 8))

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

        self._build_scan_controls(scan_tab)
        self._build_mac_controls(mac_tab)
        self._build_counters(counters_tab)
        self._build_monitor(right)

    def _build_serial_bar(self):
        frame = ttk.LabelFrame(self, text="Serial")
        frame.grid(row=0, column=0, sticky="ew", padx=8, pady=8)
        for column in range(12):
            frame.columnconfigure(column, weight=0)
        frame.columnconfigure(2, weight=1)

        ttk.Label(frame, text="Port").grid(row=0, column=0, padx=(8, 4), pady=8)
        self.port_combo = ttk.Combobox(frame, textvariable=self.port_var, width=18)
        self.port_combo.grid(row=0, column=1, padx=4, pady=8)
        ttk.Button(frame, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, sticky="w", padx=4, pady=8)

        ttk.Label(frame, text="Baud").grid(row=0, column=3, padx=(16, 4), pady=8)
        self.baud_combo = ttk.Combobox(
            frame,
            textvariable=self.baud_var,
            values=("2000000", "1500000", "921600", "460800", "230400", "115200"),
            width=10,
        )
        self.baud_combo.grid(row=0, column=4, padx=4, pady=8)
        ttk.Checkbutton(frame, text="Legacy DTR/RTS activation", variable=self.pulse_reset_var).grid(row=0, column=5, padx=8, pady=8)
        ttk.Button(frame, text="Connect", command=self.connect).grid(row=0, column=6, padx=(16, 4), pady=8)
        ttk.Button(frame, text="Disconnect", command=self.disconnect).grid(row=0, column=7, padx=4, pady=8)
        ttk.Label(frame, textvariable=self.status_var).grid(row=0, column=8, padx=(16, 8), pady=8)

    def _build_device_bar(self):
        frame = ttk.LabelFrame(self, text="Device")
        frame.grid(row=1, column=0, sticky="ew", padx=8, pady=(0, 8))
        for column in range(15):
            frame.columnconfigure(column, weight=0)
        frame.columnconfigure(14, weight=1)

        ttk.Button(frame, text="Info", command=self.send_info).grid(row=0, column=0, padx=(8, 4), pady=8)
        ttk.Button(frame, text="Version", command=self.send_version_info).grid(row=0, column=1, padx=4, pady=8)
        ttk.Button(frame, text="Clear MAC List", command=self.clear_mac_list).grid(row=0, column=2, padx=4, pady=8)
        ttk.Label(frame, text="Local MAC").grid(row=0, column=3, padx=(16, 4), pady=8)
        ttk.Label(frame, textvariable=self.local_mac_var, width=14).grid(row=0, column=4, padx=4, pady=8)
        ttk.Label(frame, text="HW").grid(row=0, column=5, padx=(16, 4), pady=8)
        ttk.Label(frame, textvariable=self.hw_version_var, width=16).grid(row=0, column=6, padx=4, pady=8)
        ttk.Label(frame, text="FW").grid(row=0, column=7, padx=(16, 4), pady=8)
        ttk.Label(frame, textvariable=self.version_var, width=10).grid(row=0, column=8, padx=4, pady=8)
        ttk.Label(frame, text="SDK").grid(row=0, column=9, padx=(16, 4), pady=8)
        ttk.Label(frame, textvariable=self.sdk_version_var, width=18).grid(row=0, column=10, padx=4, pady=8)
        ttk.Label(frame, text="List").grid(row=0, column=11, padx=(16, 4), pady=8)
        ttk.Label(frame, textvariable=self.list_capacity_var, width=6).grid(row=0, column=12, padx=4, pady=8)
        ttk.Label(frame, text="Scan").grid(row=0, column=13, padx=(16, 4), pady=8)
        ttk.Label(frame, textvariable=self.scan_state_var).grid(row=0, column=14, padx=4, pady=8)

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
        ttk.Button(actions, text="Start Scan", command=self.start_scan).pack(side=tk.LEFT, padx=(12, 4))
        ttk.Button(actions, text="Stop Scan", command=self.stop_scan).pack(side=tk.LEFT, padx=4)

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
        test_tab = ttk.Frame(notebook)
        txadv_tab = ttk.Frame(notebook)
        conn_tab = ttk.Frame(notebook)
        log_tab = ttk.Frame(notebook)
        notebook.add(adv_tab, text="Advertisements")
        notebook.add(stats_tab, text="MAC Stats")
        notebook.add(test_tab, text="GPIO / UART / RF")
        notebook.add(log_tab, text="Log")

        self._build_adv_tab(adv_tab)
        self._build_stats_tab(stats_tab)
        self._build_test_tab(test_tab)
        self._build_txadv_tab(txadv_tab)  # built but not added to notebook
        self._build_conn_tab(conn_tab)    # built but not added to notebook
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
        self.adv_tree.configure(yscrollcommand=yscroll.set)
        self.adv_tree.grid(row=1, column=0, sticky="nsew", padx=(6, 0), pady=(0, 6))
        yscroll.grid(row=1, column=1, sticky="ns", pady=(0, 6))

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

    def _build_test_tab(self, tab):
        tab.columnconfigure(0, weight=1)
        tab.columnconfigure(1, weight=1)
        tab.rowconfigure(2, weight=1)

        board_frame = ttk.LabelFrame(tab, text="LED pins via GPIO")
        board_frame.grid(row=0, column=0, sticky="nsew", padx=6, pady=6)
        board_frame.columnconfigure(1, weight=1)

        direct_leds = ttk.Frame(board_frame)
        direct_leds.grid(row=0, column=0, columnspan=3, sticky="ew", padx=8, pady=(8, 8))
        direct_leds.columnconfigure(0, weight=1)
        for row, (name, bit) in enumerate(LED_BITS.items()):
            ttk.Label(direct_leds, text=name).grid(row=row, column=0, sticky="w", pady=2)
            ttk.Button(direct_leds, text="On", command=lambda value=bit: self.led_set_mask(value, True)).grid(row=row, column=1, padx=2, pady=2)
            ttk.Button(direct_leds, text="Off", command=lambda value=bit: self.led_set_mask(value, False)).grid(row=row, column=2, padx=2, pady=2)
            ttk.Button(direct_leds, text="Toggle", command=lambda value=bit: self.led_toggle_mask(value)).grid(row=row, column=3, padx=2, pady=2)
            ttk.Button(direct_leds, text="Blink", command=lambda value=bit: self.led_blink_mask(value)).grid(row=row, column=4, padx=2, pady=2)


        blink_frame = ttk.Frame(board_frame)
        blink_frame.grid(row=2, column=0, columnspan=3, sticky="ew", padx=8, pady=4)
        ttk.Label(blink_frame, text="Blink count").pack(side=tk.LEFT)
        ttk.Spinbox(blink_frame, textvariable=self.led_blink_count_var, from_=1, to=5, width=4).pack(side=tk.LEFT, padx=(4, 10))
        ttk.Label(blink_frame, text="delay ms").pack(side=tk.LEFT)
        ttk.Spinbox(blink_frame, textvariable=self.led_blink_delay_var, from_=10, to=200, increment=10, width=5).pack(side=tk.LEFT, padx=(4, 10))
        ttk.Button(blink_frame, text="RGB blink", command=lambda: self.led_blink_mask(0x07)).pack(side=tk.LEFT)
        ttk.Button(blink_frame, text="All off", command=self.led_all_off).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Button(blink_frame, text="Query state", command=self.led_query_state).pack(side=tk.LEFT, padx=(6, 0))

        ttk.Separator(board_frame, orient=tk.HORIZONTAL).grid(row=3, column=0, columnspan=3, sticky="ew", padx=8, pady=8)

        ttk.Label(board_frame, text="GPIO pin").grid(row=4, column=0, sticky="w", padx=8, pady=4)
        ttk.Combobox(
            board_frame,
            textvariable=self.gpio_pin_var,
            values=tuple(BOARD_PINS.keys()),
            state="readonly",
            width=20,
        ).grid(row=4, column=1, sticky="ew", padx=8, pady=4)
        gpio_read_frame = ttk.Frame(board_frame)
        gpio_read_frame.grid(row=4, column=2, sticky="ew", padx=(0, 8), pady=4)
        ttk.Button(gpio_read_frame, text="Read", command=self.gpio_read_selected).pack(side=tk.LEFT, padx=(0, 4))
        ttk.Button(gpio_read_frame, text="Read all", command=self.gpio_read_all).pack(side=tk.LEFT)

        ttk.Checkbutton(board_frame, text="Value high", variable=self.gpio_value_var).grid(row=5, column=0, sticky="w", padx=8, pady=4)
        ttk.Button(board_frame, text="Write pin", command=self.gpio_write_selected).grid(row=5, column=1, sticky="ew", padx=8, pady=4)
        ttk.Button(board_frame, text="Toggle pin", command=self.gpio_toggle_selected).grid(row=5, column=2, sticky="ew", padx=(0, 8), pady=4)

        gpio_cfg = ttk.Frame(board_frame)
        gpio_cfg.grid(row=6, column=0, columnspan=3, sticky="ew", padx=8, pady=4)
        ttk.Checkbutton(gpio_cfg, text="Input", variable=self.gpio_input_var).pack(side=tk.LEFT)
        ttk.Checkbutton(gpio_cfg, text="Output", variable=self.gpio_output_var).pack(side=tk.LEFT, padx=(10, 0))
        ttk.Combobox(gpio_cfg, textvariable=self.gpio_pull_var, values=tuple(GPIO_PULLS.keys()), state="readonly", width=14).pack(side=tk.LEFT, padx=(10, 0))
        ttk.Button(gpio_cfg, text="Configure", command=self.gpio_config_selected).pack(side=tk.LEFT, padx=(10, 0))

        status_frame = ttk.Frame(board_frame)
        status_frame.grid(row=8, column=0, columnspan=3, sticky="ew", padx=8, pady=(8, 8))
        ttk.Label(status_frame, text="Key").grid(row=0, column=0, sticky="w")
        ttk.Label(status_frame, textvariable=self.key_state_var, width=12).grid(row=0, column=1, sticky="w", padx=(4, 14))
        ttk.Label(status_frame, text="LED mask").grid(row=0, column=2, sticky="w")
        ttk.Label(status_frame, textvariable=self.led_state_var, width=8).grid(row=0, column=3, sticky="w", padx=(4, 14))
        ttk.Label(status_frame, text="Board mask").grid(row=0, column=4, sticky="w")
        ttk.Label(status_frame, textvariable=self.board_mask_var, width=8).grid(row=0, column=5, sticky="w", padx=(4, 0))

        pin_states_frame = ttk.LabelFrame(board_frame, text="GPIO pin states")
        pin_states_frame.grid(row=9, column=0, columnspan=3, sticky="ew", padx=8, pady=(0, 8))
        for _idx, (_pin_id, _pin_name) in enumerate(sorted(BOARD_PIN_NAMES.items())):
            _col = (_idx % 2) * 3
            _row = _idx // 2
            ttk.Label(pin_states_frame, text=_pin_name, anchor="w").grid(row=_row, column=_col, sticky="w", padx=(6, 2), pady=1)
            ttk.Label(pin_states_frame, text="=").grid(row=_row, column=_col + 1, padx=0, pady=1)
            ttk.Label(pin_states_frame, textvariable=self.gpio_pin_state_vars[_pin_id], width=3).grid(row=_row, column=_col + 2, sticky="w", padx=(0, 12), pady=1)

        uart_frame = ttk.LabelFrame(tab, text="UART CH340C")
        uart_frame.grid(row=1, column=0, sticky="ew", padx=6, pady=6)
        uart_frame.columnconfigure(1, weight=1)

        ttk.Button(uart_frame, text="Status", command=self.uart_status).grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 4))
        ttk.Label(uart_frame, textvariable=self.uart_status_var).grid(row=0, column=1, sticky="w", padx=8, pady=(8, 4))
        ttk.Button(uart_frame, text="Ping", command=self.uart_ping).grid(row=1, column=0, sticky="ew", padx=8, pady=4)
        ttk.Spinbox(uart_frame, textvariable=self.uart_ping_seq_var, from_=0, to=65535, width=8).grid(row=1, column=1, sticky="w", padx=8, pady=4)

        ttk.Combobox(
            uart_frame,
            textvariable=self.uart_baud_index_var,
            values=tuple(f"{index} - {baud}" for index, baud in enumerate(FIRMWARE_BAUDS)),
            state="readonly",
            width=18,
        ).grid(row=2, column=0, sticky="ew", padx=8, pady=(4, 8))
        ttk.Button(uart_frame, text="Set firmware baud", command=self.uart_set_baud).grid(row=2, column=1, sticky="w", padx=8, pady=(4, 8))

        rf_frame = ttk.LabelFrame(tab, text="TLSR825x RF/SDK Tuning")
        rf_frame.grid(row=0, column=1, rowspan=2, sticky="nsew", padx=6, pady=6)
        rf_frame.columnconfigure(1, weight=1)

        ttk.Button(rf_frame, text="Read status", command=self.rf_read_status).grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 4))
        ttk.Label(rf_frame, textvariable=self.rf_status_var).grid(row=0, column=1, sticky="w", padx=8, pady=(8, 4))

        ttk.Label(rf_frame, text="RF power").grid(row=1, column=0, sticky="w", padx=8, pady=4)
        ttk.Combobox(
            rf_frame,
            textvariable=self.rf_power_var,
            values=tuple(label for label, _ in RF_POWER_OPTIONS),
            state="readonly",
            width=22,
        ).grid(row=1, column=1, sticky="ew", padx=8, pady=4)
        ttk.Button(rf_frame, text="Apply", command=self.rf_apply_power).grid(row=1, column=2, sticky="ew", padx=(0, 8), pady=4)

        ttk.Label(rf_frame, text="Internal cap").grid(row=2, column=0, sticky="w", padx=8, pady=4)
        cap_frame = ttk.Frame(rf_frame)
        cap_frame.grid(row=2, column=1, sticky="ew", padx=8, pady=4)
        ttk.Spinbox(cap_frame, textvariable=self.rf_cap_var, from_=0, to=63, width=6).pack(side=tk.LEFT)
        ttk.Label(cap_frame, textvariable=self.rf_cap_status_var).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(rf_frame, text="Apply", command=self.rf_apply_cap).grid(row=2, column=2, sticky="ew", padx=(0, 8), pady=4)

        ttk.Label(rf_frame, text="Primary channels").grid(row=3, column=0, sticky="w", padx=8, pady=4)
        channel_frame = ttk.Frame(rf_frame)
        channel_frame.grid(row=3, column=1, sticky="w", padx=8, pady=4)
        ttk.Spinbox(channel_frame, textvariable=self.rf_ch0_var, from_=0, to=39, width=4).pack(side=tk.LEFT)
        ttk.Spinbox(channel_frame, textvariable=self.rf_ch1_var, from_=0, to=39, width=4).pack(side=tk.LEFT, padx=4)
        ttk.Spinbox(channel_frame, textvariable=self.rf_ch2_var, from_=0, to=39, width=4).pack(side=tk.LEFT)
        ttk.Button(rf_frame, text="Apply", command=self.rf_apply_channels).grid(row=3, column=2, sticky="ew", padx=(0, 8), pady=4)

        ttk.Label(rf_frame, text="Coded min window ms").grid(row=4, column=0, sticky="w", padx=8, pady=4)
        ttk.Spinbox(rf_frame, textvariable=self.coded_min_ms_var, from_=10, to=2000, increment=10, width=8).grid(row=4, column=1, sticky="w", padx=8, pady=4)
        ttk.Button(rf_frame, text="Apply", command=self.rf_apply_coded_min).grid(row=4, column=2, sticky="ew", padx=(0, 8), pady=4)

        quick = ttk.Frame(rf_frame)
        quick.grid(row=5, column=0, columnspan=3, sticky="ew", padx=8, pady=(12, 8))
        ttk.Button(quick, text="Default adv channels", command=self.rf_default_channels).pack(side=tk.LEFT)
        ttk.Button(quick, text="High TX/RX test power", command=self.rf_high_power).pack(side=tk.LEFT, padx=6)
        ttk.Button(quick, text="0 dBm VANT", command=self.rf_zero_dbm).pack(side=tk.LEFT)
        ttk.Button(quick, text="Coded 50% of window", command=self.rf_coded_50pct_of_window).pack(side=tk.LEFT, padx=6)

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

        btn_row = ttk.Frame(ctrl)
        btn_row.grid(row=3, column=0, columnspan=3, sticky="w", padx=8, pady=(4, 8))
        ttk.Button(btn_row, text="Connect", command=self.conn_connect).pack(side=tk.LEFT)
        ttk.Button(btn_row, text="Disconnect", command=self.conn_disconnect).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(btn_row, text="Cancel", command=self.conn_cancel).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(btn_row, text="Status", command=self.conn_status).pack(side=tk.LEFT, padx=(8, 0))

        info_row = ttk.Frame(ctrl)
        info_row.grid(row=4, column=0, columnspan=3, sticky="ew", padx=8, pady=(0, 8))
        ttk.Label(info_row, text="State:").pack(side=tk.LEFT)
        ttk.Label(info_row, textvariable=self.conn_state_var, width=14).pack(side=tk.LEFT, padx=(4, 16))
        ttk.Label(info_row, text="Handle:").pack(side=tk.LEFT)
        ttk.Label(info_row, textvariable=self.conn_handle_var, width=6).pack(side=tk.LEFT, padx=(4, 16))
        ttk.Label(info_row, text="Interval:").pack(side=tk.LEFT)
        ttk.Label(info_row, textvariable=self.conn_interval_var, width=10).pack(side=tk.LEFT, padx=(4, 0))

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
        self.port_combo.configure(values=ports)
        if not self.port_var.get() and ports:
            self.port_var.set(ports[0])

    def connect(self):
        try:
            self.client.open(self.port_var.get().strip(), int(self.baud_var.get()), self.pulse_reset_var.get())
            self.status_var.set("Connected")
            self.led_blink_generation += 1
            self.after(GUI_BOOTSTRAP_DELAY_MS, self.send_info)
        except Exception as exc:
            self.status_var.set("Connection failed")
            self.log(f"Connection failed: {exc}")
            messagebox.showerror("Connection failed", str(exc))

    def disconnect(self):
        self.led_blink_generation += 1
        self.client.close()
        self.status_var.set("Disconnected")
        self.scan_state_var.set("Stopped")
        self.log("Disconnected")

    def send_info(self):
        self.safe_command(self.client.command_info)
        self.after(80, lambda: self.safe_command(self.client.command_version_info))

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
            self.scan_state_var.set(config.describe())
            self.log(f"Start scan: {config.describe()}")
        except Exception as exc:
            self.command_error(exc)

    def stop_scan(self):
        self.last_started_scan_config = None
        self.safe_command(self.client.command_stop_scan)

    def safe_command(self, callback):
        try:
            callback()
        except Exception as exc:
            self.command_error(exc)

    def command_error(self, exc: Exception):
        self.log(f"Command failed: {exc}")
        messagebox.showerror("Command failed", str(exc))

    def status_name(self, status: int) -> str:
        return COMMAND_STATUS.get(status, f"0x{status:02X}")

    def selected_gpio_pin_id(self) -> int:
        return BOARD_PINS.get(self.gpio_pin_var.get(), 0x22)

    def selected_uart_baud_index(self) -> int:
        return int(self.uart_baud_index_var.get().split("-", 1)[0].strip())

    def led_mask_from_board_mask(self, board_mask: int) -> int:
        led_mask = 0
        for bit, pin_id in LED_PIN_CODES.items():
            mask_index = BOARD_PIN_MASK_INDEX.get(pin_id)
            if mask_index is not None and (board_mask & (1 << mask_index)):
                led_mask |= bit
        return led_mask

    def pin_state_is_high(self, pin_id: int):
        state = self.gpio_pin_state_vars[pin_id].get()
        if state == "?":
            return None
        return state not in ("0", "")

    def refresh_board_labels_from_cache(self):
        board_mask = 0
        for pin_id, mask_index in BOARD_PIN_MASK_INDEX.items():
            if self.pin_state_is_high(pin_id):
                board_mask |= 1 << mask_index
        self.board_mask_var.set(f"0x{board_mask:04X}")
        self.led_state_var.set(f"0x{self.led_mask_from_board_mask(board_mask):02X}")
        key_state = self.pin_state_is_high(0x07)
        if key_state is None:
            self.key_state_var.set("?")
        else:
            self.key_state_var.set("Pressed" if not key_state else "Released")

    def set_board_labels(self, board_mask: int, led_mask=None):
        self.board_mask_var.set(f"0x{board_mask:04X}")
        if led_mask is None:
            led_mask = self.led_mask_from_board_mask(board_mask)
        self.led_state_var.set(f"0x{led_mask:02X}")
        key_index = BOARD_PIN_MASK_INDEX.get(0x07)
        self.key_state_var.set("Pressed" if key_index is not None and not (board_mask & (1 << key_index)) else "Released")

    def led_pin_ids_from_mask(self, mask: int):
        return [pin_id for bit, pin_id in LED_PIN_CODES.items() if mask & bit]

    def _write_led_pins(self, pin_ids, enabled: bool):
        for pin_id in pin_ids:
            self.client.command_gpio_write(pin_id, enabled)

    def _toggle_led_pins(self, pin_ids):
        for pin_id in pin_ids:
            self.client.command_gpio_toggle(pin_id)

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
        self.safe_command(lambda: self.client.command_gpio_read(self.selected_gpio_pin_id()))

    def gpio_write_selected(self):
        pin_id = self.selected_gpio_pin_id()
        self.safe_command(lambda: self.client.command_gpio_write(pin_id, self.gpio_value_var.get()))

    def gpio_toggle_selected(self):
        self.safe_command(lambda: self.client.command_gpio_toggle(self.selected_gpio_pin_id()))

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
        value = max(0, min(63, int(self.rf_cap_var.get())))
        self.safe_command(lambda: self.client.command_rf_cap(value))

    def rf_apply_channels(self):
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
        self.rf_power_var.set("+10.46 dBm VBAT")
        self.rf_apply_power()

    def rf_zero_dbm(self):
        self.rf_power_var.set("+0.04 dBm VANT")
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
        self.after(50, self.process_events)

    def handle_rx(self, event):
        if isinstance(event, AdvPacket):
            self.handle_advertisement(event)
        elif isinstance(event, CommandResponse):
            self.handle_response(event)
        elif isinstance(event, tuple) and event[0] == "crc_error":
            self.log(f"RX CRC/resync discard 0x{event[1]:02X}")

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

    def handle_gpio_response(self, response: CommandResponse):
        status = self.status_name(response.index)
        if response.data_len >= 6:
            op, pin_id, value, caps = response.data[:4]
            pin_name = BOARD_PIN_NAMES.get(pin_id, f"pin {pin_id}")
            if pin_id in self.gpio_pin_state_vars:
                if op == 5:
                    self.gpio_pin_state_vars[pin_id].set("1" if value else "0")
                elif value in (0, 1):
                    self.gpio_pin_state_vars[pin_id].set(str(value))
                else:
                    self.gpio_pin_state_vars[pin_id].set("1" if value else "0")
            self.refresh_board_labels_from_cache()
            self.log(f"RX GPIO {status} op={op} {pin_name} value={value} caps=0x{caps:02X}")
        else:
            self.log(f"RX GPIO {status} data={bytes_to_hex(response.data)}")

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
            if cap == 0xFF:
                self.rf_cap_status_var.set("calibrated/unknown")
            else:
                self.rf_cap_var.set(cap)
                self.rf_cap_status_var.set(f"active {cap}")
            self.rf_ch0_var.set(ch0)
            self.rf_ch1_var.set(ch1)
            self.rf_ch2_var.set(ch2)
            self.coded_min_ms_var.set(coded_units * 10)
            self.rf_status_var.set(f"{status}: power={label}, cap=0x{cap:02X}, channels={ch0}/{ch1}/{ch2}")
            self.log(
                f"RX RFSDK {status} power={label} cap=0x{cap:02X} channels={ch0}/{ch1}/{ch2} coded_min={coded_units * 10} ms"
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
        self.log(f"RX CONN state={state_name} handle=0x{conn_handle:04X} interval={interval_125us * 1.25:.2f}ms data={bytes_to_hex(response.data)}")

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

    def gpio_read_all(self):
        """Read all board GPIO pins sequentially and update the pin-states grid."""
        self.gpio_read_generation += 1
        generation = self.gpio_read_generation
        pin_ids = sorted(BOARD_PIN_NAMES.keys())

        def send_next(index: int = 0):
            if generation != self.gpio_read_generation or not self.client.is_open:
                return
            if index >= len(pin_ids):
                return
            self.safe_command(lambda pin_id=pin_ids[index]: self.client.command_gpio_read(pin_id))
            self.after(25, lambda: send_next(index + 1))

        send_next()

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
        self.safe_command(lambda: self.client.command_conn_connect(peer_addr, peer_type, phy))

    def conn_disconnect(self):
        self.safe_command(self.client.command_conn_disconnect)

    def conn_cancel(self):
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
    app = AdvBle2UartGui()
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