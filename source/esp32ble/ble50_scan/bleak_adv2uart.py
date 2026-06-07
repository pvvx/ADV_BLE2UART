"""
Bleak backend for the adv2uart protocol.

Communicates with an ESP32 running the adv2uart BLE scanner firmware over
USB Serial.  Provides a full ``BleakScanner`` and a
full ``BleakClient`` implementation that satisfy Bleak's backend interface.

Usage::

    from bleak import BleakScanner, BleakClient
    from ble50_scan.bleak_adv2uart import BleakScannerAdv2Uart, BleakClientAdv2Uart

    # Scan using the ESP32
    scanner = BleakScanner(backend=BleakScannerAdv2Uart, detection_callback=my_cb)
    ...

    # Connect using the ESP32 as GATT client
    client = BleakClient(address, backend=BleakClientAdv2Uart)
    ...
"""

from __future__ import annotations

import asyncio
import logging
import os
import re
import struct
import time
import uuid
from collections.abc import Callable
from typing import Any, Optional, Union

try:
    import serial
except ImportError:
    serial = None  # type: ignore

from bleak.backends.device import BLEDevice
from bleak.backends.scanner import (
    AdvertisementData,
    AdvertisementDataCallback,
    BaseBleakScanner,
)
from bleak.backends.client import BaseBleakClient, NotifyCallback
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.descriptor import BleakGATTDescriptor
from bleak.backends.service import BleakGATTService, BleakGATTServiceCollection
from bleak.exc import BleakError, BleakDeviceNotFoundError

logger = logging.getLogger(__name__)

# ── Protocol constants ─────────────────────────────────────────────────────

CMD_ID_INFO    = 0x00
CMD_ID_SCAN    = 0x01
CMD_ID_WMAC    = 0x02
CMD_ID_BMAC    = 0x03
CMD_ID_CLRM    = 0x04
CMD_ID_PRNT    = 0x05
CMD_ID_GPIO    = 0x06
CMD_ID_UART    = 0x08
CMD_ID_RFSDK   = 0x09
CMD_ID_VERSION = 0x0A
CMD_ID_TXADV   = 0x0B
CMD_ID_CONN    = 0x0C
CMD_ID_TXDATA  = 0x0D
CMD_ID_RXDATA  = 0x0E
CMD_ID_VBAT    = 0x0F
CMD_ID_GPIOEVT = 0x10

# CONN sub-operations
CONN_OP_STATUS     = 0
CONN_OP_OPEN_1M    = 1
CONN_OP_OPEN_CODED = 2
CONN_OP_DISCONNECT = 3
CONN_OP_CANCEL     = 4
CONN_OP_DISCOVER   = 5
CONN_OP_READ_CHAR  = 6
CONN_OP_WRITE_RSP  = 7
CONN_OP_READ_DESCR = 8
CONN_OP_WRITE_DESCR= 9
CONN_OP_MTU_EXCH   = 10
CONN_OP_PAIR       = 11
CONN_OP_UNPAIR     = 12

CONN_EVENT_FLAG = 0x80
CONN_DISCOV_SERVICE  = 0
CONN_DISCOV_CHAR     = 1
CONN_DISCOV_DESCR    = 2
CONN_DISCOV_COMPLETE = 3

CMD_STATUS_OK    = 0
CMD_STATUS_ARGS  = 1
CMD_STATUS_PIN   = 2
CMD_STATUS_DENIED= 3
CMD_STATUS_VALUE = 4

FRAME_DATA_LEN = 6
HEAD_CRC_ADD_LEN = 13

SW_VERSION = 0x02

# ── CRC-16/ARC ─────────────────────────────────────────────────────────────

_CRC_TABLE = [
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
]


def _crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc = (crc >> 8) ^ _CRC_TABLE[(crc ^ b) & 0xFF]
    return crc


def _build_cmd_frame(cmd: int, payload: bytes = b"") -> bytes:
    """Build a host-to-device command frame (simple format).

    The ESP32 firmware expects:
      [0]       = cmd (CMD_ID_xxx)
      [1..N]    = payload
      [N+1..N+2] = CRC-16 LE over all preceding bytes
    """
    msg = bytes([cmd & 0xFF]) + payload
    crc = _crc16(msg)
    return msg + bytes([crc & 0xFF, crc >> 8])


def _parse_frame(data: bytes):
    """Parse a single adv2uart frame from a buffer.

    Two wire formats exist:

    **Command response** (byte[4] == 0xFF):
      [0]=payload_len  [1]=cmd  [2]=status  [3]=data_len  [4]=0xFF
      [5..10]=data_field (6 bytes)  [11..]=extra_payload  [-2:]=CRC16

    **Advertisement** (byte[4] != 0xFF):
      [0]=adv_data_len  [1]=RSSI(s8)  [2]=event_type  [3]=addr_type|dir
      [4]=primary_phy|secondary_phy<<4  [5..10]=MAC(LE-reversed)
      [11..11+adv_data_len]=AD payload  [-2:]=CRC16

    Returns (payload_len, cmd, byte2, byte3, byte4, data_field, extra, crc_ok)
    or None if incomplete.  For advertisement frames, `cmd` is set to CMD_ID_SCAN.
    """
    if len(data) < HEAD_CRC_ADD_LEN:
        return None
    payload_len = data[0]
    is_adv = data[4] != 0xFF

    if is_adv:
        # Advertisement frame: 11-byte header + adv_data + 2 CRC
        frame_len = 11 + payload_len + 2
    else:
        # Command frame: 13-byte header + extra + 2 CRC
        frame_len = HEAD_CRC_ADD_LEN + payload_len

    if len(data) < frame_len:
        return None
    crc_ok = _crc16(data[:frame_len]) == 0

    if is_adv:
        # byte2=RSSI(s8), byte3=event_type, byte4=PHY, data_field=MAC(6)
        rssi = data[1] if data[1] < 128 else data[1] - 256
        return (payload_len, CMD_ID_SCAN, rssi, data[2], data[4],
                data[5:11], data[11:11 + payload_len], crc_ok)
    else:
        data_field = data[5:5 + FRAME_DATA_LEN]
        extra = data[11:11 + payload_len] if payload_len else b""
        return (payload_len, data[1], data[2], data[3], data[4],
                data_field, extra, crc_ok)



# ── Serial transport ───────────────────────────────────────────────────────

class _SerialTransport:
    """Thin wrapper around a pySerial connection."""

    def __init__(self, port: str, baud: int = 2000000, timeout: float = 1.0):
        if serial is None:
            raise ImportError("pySerial is required; pip install pyserial")
        self._port = port
        self._baud = baud
        self._timeout = timeout
        self._ser: Optional[serial.Serial] = None
        self._read_buf = bytearray()

    async def connect(self):
        loop = asyncio.get_event_loop()
        self._ser = await loop.run_in_executor(
            None, lambda: serial.Serial(self._port, self._baud,
                                        timeout=self._timeout))
        # Drain any stale data
        await loop.run_in_executor(None, self._ser.reset_input_buffer)

    def close(self):
        if self._ser:
            self._ser.close()
            self._ser = None

    async def write(self, data: bytes):
        if not self._ser:
            raise BleakError("Not connected")
        loop = asyncio.get_event_loop()
        await loop.run_in_executor(None, self._ser.write, data)

    async def read_frame(self):
        """Read and parse one protocol frame.  Returns the parsed tuple or None."""
        if not self._ser:
            raise BleakError("Not connected")
        loop = asyncio.get_event_loop()
        # Keep reading until we have at least one complete frame
        while True:
            result = _parse_frame(bytes(self._read_buf))
            if result is not None:
                # Consume the frame from the buffer
                payload_len = self._read_buf[0]
                frame_len = HEAD_CRC_ADD_LEN + payload_len
                self._read_buf = self._read_buf[frame_len:]
                return result
            # Need more data
            chunk = await loop.run_in_executor(
                None, lambda: self._ser.read(256))
            if not chunk:
                return None  # timeout
            self._read_buf.extend(chunk)


# ── Advertisement parsing ──────────────────────────────────────────────────

def _parse_adv_data(data_field: bytes, extra: bytes, byte2: int, byte3: int, byte4: int):
    """Parse an advertisement frame into (BLEDevice, AdvertisementData) or None.

    Args:
        data_field: 6-byte MAC (LE on-wire, reversed).
        extra: AD payload bytes.
        byte2: RSSI (signed int8).
        byte3: event_type.
        byte4: primary_phy | (secondary_phy << 4).
    """
    # MAC: data_field has bytes [addr[5], addr[4], ..., addr[0]]
    mac_bytes = data_field[:6][::-1]  # reverse to display order
    mac = ":".join(f"{b:02X}" for b in mac_bytes)
    rssi = byte2  # already signed by _parse_frame
    event_type = byte3
    primary_phy = byte4 & 0x0F
    secondary_phy = (byte4 >> 4) & 0x0F
    adv_data = extra

    # Parse AD structures (simplified: extract local name, manufacturer data, service UUIDs)
    local_name = None
    manufacturer_data = {}
    service_uuids = []
    service_data = {}
    tx_power = None

    pos = 0
    while pos < len(adv_data):
        if pos + 1 >= len(adv_data):
            break
        ad_len = adv_data[pos]
        if ad_len == 0:
            break
        if pos + 1 + ad_len > len(adv_data):
            break
        ad_type = adv_data[pos + 1]
        ad_payload = adv_data[pos + 2:pos + 1 + ad_len]
        if ad_type == 0x01:  # Flags
            pass
        elif ad_type == 0x02 or ad_type == 0x03:  # 16-bit Service UUID
            for i in range(0, len(ad_payload) - 1, 2):
                uuid16 = ad_payload[i] | (ad_payload[i + 1] << 8)
                service_uuids.append(f"{uuid16:04X}")
        elif ad_type == 0x06 or ad_type == 0x07:  # 128-bit Service UUID
            pass
        elif ad_type == 0x08 or ad_type == 0x09:  # Local Name
            try:
                local_name = ad_payload.decode("utf-8")
            except UnicodeDecodeError:
                local_name = ad_payload.hex()
        elif ad_type == 0x0A:  # TX Power Level
            if ad_payload:
                tx_power = ad_payload[0] if ad_payload[0] < 128 else ad_payload[0] - 256
        elif ad_type == 0x16:  # Service Data - 16-bit UUID
            if len(ad_payload) >= 2:
                uuid16 = ad_payload[0] | (ad_payload[1] << 8)
                service_data[f"{uuid16:04X}"] = ad_payload[2:]
        elif ad_type == 0xFF:  # Manufacturer Specific
            if len(ad_payload) >= 2:
                mfr_id = ad_payload[0] | (ad_payload[1] << 8)
                manufacturer_data[mfr_id] = ad_payload[2:]
        pos += 1 + ad_len

    advertisement_data = AdvertisementData(
        local_name=local_name or "",
        manufacturer_data=manufacturer_data,
        service_data=service_data,
        service_uuids=service_uuids,
        tx_power=tx_power,
        rssi=rssi,
        platform_data=(mac, event_type, primary_phy, secondary_phy, adv_data),
    )
    device = BLEDevice(
        address=mac,
        name=local_name or mac,
        details={"path": mac, "adv2uart_mac": mac},
        rssi=rssi,
    )
    return device, advertisement_data


# ── Scanner Backend ────────────────────────────────────────────────────────

class BleakScannerAdv2Uart(BaseBleakScanner):
    """Bleak scanner backend that receives BLE advertisements via the adv2uart
    firmware over USB Serial.

    Args:
        detection_callback: Called for each advertisement received.
        service_uuids: Not supported (filtering is done on the ESP32 side).
    """

    def __init__(
        self,
        detection_callback: Optional[AdvertisementDataCallback],
        service_uuids: Optional[list[str]],
        scanning_mode: str = "active",
        **kwargs,
    ):
        super().__init__(detection_callback, service_uuids)
        self._transport: Optional[_SerialTransport] = None
        self._port: str = kwargs.get("port", os.environ.get("ADV2UART_PORT", "/dev/ttyACM0"))
        self._baud: int = kwargs.get("baud", 2000000)
        self._scan_task: Optional[asyncio.Task] = None
        self._running = False
        self._whitelist: list[str] = kwargs.get("whitelist", [])
        self._blacklist: list[str] = kwargs.get("blacklist", [])

    async def start(self) -> None:
        if self._running:
            return
        self.seen_devices = {}
        self._transport = _SerialTransport(self._port, self._baud)
        await self._transport.connect()
        # Send MAC filter commands before starting the scan.
        await self._send_mac_filters()
        # Tell the ESP32 to start scanning (both 1M + Coded PHY, 30 ms window).
        scan_cmd = _build_cmd_frame(CMD_ID_SCAN, bytes([0x03, 48, 0]))
        await self._transport.write(scan_cmd)
        self._running = True
        self._scan_task = asyncio.create_task(self._scan_loop())

    async def _send_mac_filters(self):
        """Send whitelist (WMAC) and blacklist (BMAC) entries to the ESP32."""
        for mac in self._whitelist:
            mac_bytes = _mac_to_wire(mac)
            if mac_bytes:
                await self._transport.write(_build_cmd_frame(CMD_ID_WMAC, mac_bytes))
        for mac in self._blacklist:
            mac_bytes = _mac_to_wire(mac)
            if mac_bytes:
                await self._transport.write(_build_cmd_frame(CMD_ID_BMAC, mac_bytes))
        if self._whitelist or self._blacklist:
            await asyncio.sleep(0.1)  # let the firmware process filter entries

    async def stop(self) -> None:
        self._running = False
        if self._scan_task:
            self._scan_task.cancel()
            self._scan_task = None
        if self._transport:
            self._transport.close()
            self._transport = None

    def set_scanning_filter(self, **kwargs) -> None:
        """Set scanning filters (not supported by adv2uart backend — ignored)."""
        pass

    async def _scan_loop(self):
        """Continuously read advertisement frames from the serial port."""
        while self._running:
            try:
                result = await self._transport.read_frame()
                if result is None:
                    continue
                payload_len, cmd, byte2, byte3, byte4, data_field, extra, crc_ok = result
                if not crc_ok:
                    continue
                if cmd == CMD_ID_SCAN:
                    parsed = _parse_adv_data(data_field, extra, byte2, byte3, byte4)
                    if parsed is not None:
                        device, adv_data = parsed
                        self.create_or_update_device(
                            device.address,
                            device.name,
                            device.details,
                            adv_data,
                        )
                        self.call_detection_callbacks(device, adv_data)
                elif cmd == CMD_ID_INFO:
                    # INFO response: byte[1] = SW_VERSION, data[0..5] = BD_ADDR
                    sw_ver = byte2
                    mac_bytes = data_field[:6][::-1]
                    local_addr = ":".join(f"{b:02X}" for b in mac_bytes)
                    logger.debug("ESP32 adv2uart firmware version %d, BD_ADDR %s",
                                 sw_ver, local_addr)
            except asyncio.CancelledError:
                break
            except Exception:
                logger.exception("Error in adv2uart scan loop")


# ── Client Backend ─────────────────────────────────────────────────────────

class BleakClientAdv2Uart(BaseBleakClient):
    """Bleak client backend that connects to BLE peripherals through the
    adv2uart firmware's GATT client.

    Args:
        address_or_ble_device: MAC address of the peripheral to connect to.
    """

    def __init__(
        self,
        address_or_ble_device: Union[BLEDevice, str],
        **kwargs,
    ):
        super().__init__(address_or_ble_device, **kwargs)
        self._transport: Optional[_SerialTransport] = None
        self._port: str = kwargs.get("port", os.environ.get("ADV2UART_PORT", "/dev/ttyACM0"))
        self._baud: int = kwargs.get("baud", 2000000)
        self._connected = False
        self._mtu = 23
        self._notify_callbacks: dict[int, NotifyCallback] = {}
        self._read_queue: asyncio.Queue = asyncio.Queue()
        self._discovery_done = asyncio.Event()
        self._discovery_results: list = []
        self._read_task: Optional[asyncio.Task] = None
        # GATT service collection (populated by service discovery)
        self.services: Optional[BleakGATTServiceCollection] = None

    # ── Properties ─────────────────────────────────────────────────────────

    @property
    def is_connected(self) -> bool:
        return self._connected

    @property
    def mtu_size(self) -> int:
        return self._mtu

    @property
    def name(self) -> str:
        """Device name (same as address for our backend)."""
        return self.address

    def set_disconnected_callback(
        self, callback: Optional[Callable[[], None]], **kwargs
    ) -> None:
        """Set a callback for unsolicited disconnection."""
        self._disconnected_callback = callback

    # ── Connectivity ───────────────────────────────────────────────────────

    async def connect(self, pair: bool = False, **kwargs) -> None:
        if self._connected:
            return
        self._transport = _SerialTransport(self._port, self._baud)
        await self._transport.connect()
        # Resolve address (strip colons, convert to LE on-wire bytes)
        addr = self.address.replace(":", "").upper()
        if len(addr) != 12:
            raise BleakError(f"Invalid MAC address: {self.address}")
        addr_bytes = bytes.fromhex(addr)[::-1]  # LE on-wire
        # Send connect command (1M PHY)
        frame = _build_cmd_frame(CMD_ID_CONN, bytes([CONN_OP_OPEN_1M, 0]) + addr_bytes)
        await self._transport.write(frame)
        # Wait for CONN response (success/failure)
        # Start reader task
        self._read_task = asyncio.create_task(self._reader_loop())
        # Wait for connection ack
        timeout = kwargs.get("timeout", 10.0)
        try:
            result = await asyncio.wait_for(self._wait_for_conn_ack(), timeout=timeout)
            if result != CMD_STATUS_OK:
                raise BleakError(f"Connection failed with status {result}")
            self._connected = True
            # Automatically discover services, like other Bleak backends do.
            await self.get_services()
        except asyncio.TimeoutError:
            raise BleakError("Connection timed out")

    async def disconnect(self) -> None:
        if not self._connected:
            return
        cmd = bytes([CMD_ID_CONN, CONN_OP_DISCONNECT])
        frame = _build_cmd_frame(CMD_ID_CONN, bytes([CONN_OP_DISCONNECT]))
        await self._transport.write(frame)
        self._connected = False
        if self._read_task:
            self._read_task.cancel()
            self._read_task = None
        if self._transport:
            self._transport.close()
            self._transport = None

    async def pair(self, **kwargs) -> None:
        if not self._connected:
            raise BleakError("Not connected")
        frame = _build_cmd_frame(CMD_ID_CONN, bytes([CONN_OP_PAIR]))
        await self._transport.write(frame)

    async def unpair(self) -> None:
        if not self._connected:
            raise BleakError("Not connected")
        frame = _build_cmd_frame(CMD_ID_CONN, bytes([CONN_OP_UNPAIR]))
        await self._transport.write(frame)

    # ── GATT Services ─────────────────────────────────────────────────────

    async def get_services(self) -> BleakGATTServiceCollection:
        """Discover all services, characteristics and descriptors."""
        if self.services is not None:
            return self.services
        self._discovery_done.clear()
        self._discovery_results: list = []
        frame = _build_cmd_frame(CMD_ID_CONN, bytes([CONN_OP_DISCOVER]))
        await self._transport.write(frame)
        try:
            await asyncio.wait_for(self._discovery_done.wait(), timeout=30.0)
        except asyncio.TimeoutError:
            raise BleakError("Service discovery timed out")
        # Build the service collection from _discovery_results
        self.services = BleakGATTServiceCollection()
        current_service: Optional[BleakGATTService] = None
        current_char: Optional[BleakGATTCharacteristic] = None

        for item in self._discovery_results:
            if item["type"] == CONN_DISCOV_SERVICE:
                uuid = _uuid_to_str(item["uuid"])
                current_service = BleakGATTService(
                    None, item["start_handle"], uuid)
                self.services.add_service(current_service)
            elif item["type"] == CONN_DISCOV_CHAR:
                uuid = _uuid_to_str(item["uuid"])
                handle = item["handle"]
                props = item["properties"]
                current_char = BleakGATTCharacteristic(
                    None, handle, uuid,
                    _props_from_int(props),
                    lambda: 23,
                    current_service)
                if current_service:
                    current_service.add_characteristic(current_char)
            elif item["type"] == CONN_DISCOV_DESCR:
                uuid = _uuid_to_str(item["uuid"])
                handle = item["handle"]
                if current_char:
                    descr = BleakGATTDescriptor(
                        None, handle, uuid, current_char)
                    current_char.add_descriptor(descr)
            elif item["type"] == CONN_DISCOV_COMPLETE:
                pass
        return self.services

    # ── GATT I/O ───────────────────────────────────────────────────────────

    async def read_gatt_char(
        self,
        characteristic: BleakGATTCharacteristic,
        **kwargs,
    ) -> bytearray:
        handle = characteristic.handle
        frame = _build_cmd_frame(CMD_ID_CONN,
                                  bytes([CONN_OP_READ_CHAR, 0,
                                         handle & 0xFF, handle >> 8]))
        await self._transport.write(frame)
        # Wait for the read response (async)
        result = await asyncio.wait_for(self._read_queue.get(), timeout=10.0)
        status, data = result
        if status != CMD_STATUS_OK:
            raise BleakError(f"Read failed with status {status}")
        return bytearray(data)

    async def write_gatt_char(
        self,
        characteristic: BleakGATTCharacteristic,
        data: bytes,
        response: bool = False,
    ) -> None:
        handle = characteristic.handle
        if response:
            op = CONN_OP_WRITE_RSP
        else:
            op = CONN_OP_WRITE_RSP  # TXDATA uses CONN_OP_WRITE_RSP too
            # For write-without-response, we use the existing TXDATA command path
            frame = _build_cmd_frame(CMD_ID_TXDATA,
                                      bytes([handle & 0xFF, handle >> 8, len(data)]) + data)
            await self._transport.write(frame)
            return
        frame = _build_cmd_frame(CMD_ID_CONN,
                                  bytes([op, 0, handle & 0xFF, handle >> 8, len(data)]) + data)
        await self._transport.write(frame)

    async def start_notify(
        self,
        characteristic: BleakGATTCharacteristic,
        callback: NotifyCallback,
        **kwargs,
    ) -> None:
        # Enable notifications by writing the CCCD (0x2902) descriptor.
        handle = characteristic.handle
        cccd_handle = None
        # Find the CCCD descriptor from the characteristic's descriptors.
        for desc in characteristic.descriptors:
            if desc.uuid == "00002902-0000-1000-8000-00805F9B34FB":
                cccd_handle = desc.handle
                break
        if cccd_handle is None:
            # Fallback: CCCD is typically at char_handle + 1.
            cccd_handle = handle + 1
        # Write 0x0001 (enable notification) to the CCCD.
        frame = _build_cmd_frame(CMD_ID_CONN,
            bytes([CONN_OP_WRITE_DESCR, 0,
                   cccd_handle & 0xFF, cccd_handle >> 8, 2, 0x01, 0x00]))
        await self._transport.write(frame)
        self._notify_callbacks[handle] = callback

    async def stop_notify(
        self,
        characteristic: BleakGATTCharacteristic,
    ) -> None:
        handle = characteristic.handle
        # Write 0x0000 (disable notification) to the CCCD.
        cccd_handle = handle + 1  # fallback
        for desc in characteristic.descriptors:
            if desc.uuid == "00002902-0000-1000-8000-00805F9B34FB":
                cccd_handle = desc.handle
                break
        frame = _build_cmd_frame(CMD_ID_CONN,
            bytes([CONN_OP_WRITE_DESCR, 0,
                   cccd_handle & 0xFF, cccd_handle >> 8, 2, 0x00, 0x00]))
        await self._transport.write(frame)
        self._notify_callbacks.pop(handle, None)

    # ── Descriptor I/O ─────────────────────────────────────────────────────

    async def read_gatt_descriptor(
        self,
        descriptor: BleakGATTDescriptor,
        **kwargs,
    ) -> bytearray:
        handle = descriptor.handle
        frame = _build_cmd_frame(CMD_ID_CONN,
                                  bytes([CONN_OP_READ_DESCR, 0,
                                         handle & 0xFF, handle >> 8]))
        await self._transport.write(frame)
        result = await asyncio.wait_for(self._read_queue.get(), timeout=10.0)
        status, data = result
        if status != CMD_STATUS_OK:
            raise BleakError(f"Descriptor read failed with status {status}")
        return bytearray(data)

    async def write_gatt_descriptor(
        self,
        descriptor: BleakGATTDescriptor,
        data: bytes,
    ) -> None:
        handle = descriptor.handle
        frame = _build_cmd_frame(CMD_ID_CONN,
                                  bytes([CONN_OP_WRITE_DESCR, 0,
                                         handle & 0xFF, handle >> 8, len(data)]) + data)
        await self._transport.write(frame)

    # ── Internal ───────────────────────────────────────────────────────────

    async def _reader_loop(self):
        """Background task that reads frames from serial and dispatches them."""
        try:
            while self._connected:
                result = await self._transport.read_frame()
                if result is None:
                    continue
                payload_len, cmd, byte2, byte3, byte4, data_field, extra, crc_ok = result
                if not crc_ok:
                    continue
                if cmd == CMD_ID_CONN:
                    await self._handle_conn_frame(byte2, data_field, extra)
                elif cmd == CMD_ID_RXDATA:
                    await self._handle_rxdata_frame(byte2, data_field, extra)
                elif cmd == CMD_ID_SCAN:
                    # Advertisement while connected (ESP32 continues scanning)
                    pass
        except asyncio.CancelledError:
            pass
        except Exception:
            logger.exception("Error in client reader loop")
            self._connected = False

    async def _handle_conn_frame(self, status: int, data_field: bytes, extra: bytes):
        """Handle a CMD_ID_CONN response or spontaneous event."""
        if status & CONN_EVENT_FLAG:
            # Spontaneous discovery event
            sub_type = status & 0x7F
            await self._handle_discovery_event(sub_type, data_field, extra)
            return
        # Regular CONN response — check status
        if status == CMD_STATUS_OK:
            # MTU response: first 2 bytes of data_field = MTU value
            if len(data_field) >= 2:
                mtu = data_field[0] | (data_field[1] << 8)
                if mtu > 23:
                    self._mtu = mtu
            # Connection ack
            await self._read_queue.put((CMD_STATUS_OK, data_field))
        else:
            await self._read_queue.put((status, data_field))

    async def _handle_discovery_event(self, sub_type: int, data_field: bytes, extra: bytes):
        if sub_type == CONN_DISCOV_SERVICE:
            # data: [is_primary, start_handle_lo, start_handle_hi, end_handle_lo, end_handle_hi, uuid...]
            is_primary = data_field[0] if len(data_field) > 0 else 1
            start_handle = (data_field[1] | (data_field[2] << 8)) if len(data_field) > 2 else 0
            end_handle = (data_field[3] | (data_field[4] << 8)) if len(data_field) > 4 else 0
            uuid_bytes = data_field[5:] if len(data_field) > 5 else b""
            self._discovery_results.append({
                "type": CONN_DISCOV_SERVICE,
                "uuid": uuid_bytes,
                "start_handle": start_handle,
                "end_handle": end_handle,
                "is_primary": is_primary,
            })
        elif sub_type == CONN_DISCOV_CHAR:
            # data: [handle_lo, handle_hi, properties, uuid...]
            handle = (data_field[0] | (data_field[1] << 8)) if len(data_field) > 1 else 0
            props = data_field[2] if len(data_field) > 2 else 0
            uuid_bytes = data_field[3:] if len(data_field) > 3 else b""
            self._discovery_results.append({
                "type": CONN_DISCOV_CHAR,
                "uuid": uuid_bytes,
                "handle": handle,
                "properties": props,
            })
        elif sub_type == CONN_DISCOV_DESCR:
            # data: [handle_lo, handle_hi, uuid...]
            handle = (data_field[0] | (data_field[1] << 8)) if len(data_field) > 1 else 0
            uuid_bytes = data_field[2:] if len(data_field) > 2 else b""
            self._discovery_results.append({
                "type": CONN_DISCOV_DESCR,
                "uuid": uuid_bytes,
                "handle": handle,
            })
        elif sub_type == CONN_DISCOV_COMPLETE:
            self._discovery_done.set()

    async def _handle_rxdata_frame(self, opcode: int, data_field: bytes, extra: bytes):
        """Handle a CMD_ID_RXDATA notification frame."""
        # data: [handle_lo, handle_hi, value_len, value...]
        if len(data_field) < 3:
            return
        handle = data_field[0] | (data_field[1] << 8)
        value_len = data_field[2]
        value = bytes(data_field[3:3 + value_len]) if value_len else b""
        if not value:
            value = extra[:value_len] if value_len else b""
        cb = self._notify_callbacks.get(handle)
        if cb:
            if asyncio.iscoroutinefunction(cb):
                await cb(handle, bytearray(value))
            else:
                cb(handle, bytearray(value))

    async def _wait_for_conn_ack(self) -> int:
        """Wait for the first CONN response after an open request."""
        while True:
            result = await self._transport.read_frame()
            if result is None:
                continue
            payload_len, cmd, byte2, byte3, byte4, data_field, extra, crc_ok = result
            if not crc_ok:
                continue
            if cmd == CMD_ID_CONN and not (byte2 & CONN_EVENT_FLAG):
                return byte2  # status byte


# ── Helper utilities ───────────────────────────────────────────────────────

def _uuid_to_str(uuid_bytes: bytes) -> str:
    """Convert UUID bytes to a string representation."""
    if len(uuid_bytes) == 2:
        return f"{uuid_bytes[0] | (uuid_bytes[1] << 8):04X}"
    elif len(uuid_bytes) == 16:
        return str(uuid.UUID(bytes=uuid_bytes))
    elif len(uuid_bytes) == 4:
        return f"{struct.unpack('<I', uuid_bytes)[0]:08X}"
    elif len(uuid_bytes) == 6:
        return f"{uuid_bytes[0] | (uuid_bytes[1] << 8) | (uuid_bytes[2] << 16) | (uuid_bytes[3] << 24):08X}-{uuid_bytes[4:6].hex()}"
    return uuid_bytes.hex()


def _mac_to_wire(text: str) -> bytes:
    """Convert a MAC address string to LE on-wire bytes (1–6 bytes)."""
    normalized = re.sub(r'[^0-9a-fA-F]', '', text)
    n = len(normalized)
    if n == 0 or n % 2 != 0 or n > 12:
        return b""
    return bytes.fromhex(normalized)[::-1]


def _props_from_int(props: int) -> list[str]:
    """Convert a GATT characteristic properties bitmask to a list of property names."""
    result = []
    if props & 0x01: result.append("broadcast")
    if props & 0x02: result.append("read")
    if props & 0x04: result.append("write-without-response")
    if props & 0x08: result.append("write")
    if props & 0x10: result.append("notify")
    if props & 0x20: result.append("indicate")
    if props & 0x40: result.append("authenticated-signed-writes")
    if props & 0x80: result.append("extended-properties")
    return result
