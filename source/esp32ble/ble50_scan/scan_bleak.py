#!/usr/bin/env python3
"""
BLE advertisement scanner using Bleak with the adv2uart backend.

Connects to an ESP32 running the adv2uart firmware over USB Serial and
uses it as a BLE scanner, displaying discovered peripherals and their
advertising data in real time.

Usage:
    ./scan_bleak.py                                # default: /dev/ttyACM0
    ./scan_bleak.py --port /dev/ttyACM1            # custom serial port
    ./scan_bleak.py --timeout 10                   # scan for 10 seconds
    ./scan_bleak.py --raw                          # show raw AD payload hex
    ./scan_bleak.py --no-name                      # hide local name column
    ./scan_bleak.py --unique                       # show only one entry per device
    ./scan_bleak.py -w 11:22:33:44:55:66           # whitelist (only this device)
    ./scan_bleak.py -b DE:AD:BE:EF                 # blacklist (hide prefix)
"""

import argparse
import asyncio
import logging
import os
import sys
import time as time_mod

# Ensure the backend module is importable (same directory).
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from bleak import BleakScanner
from bleak_adv2uart import BleakScannerAdv2Uart

logger = logging.getLogger(__name__)

# Shared scan start time for relative timestamps.
_scan_start = 0.0


def on_advertisement(device, advertisement_data):
    """Detection callback — called for every advertisement the ESP32 forwards."""
    global _scan_start
    rssi = advertisement_data.rssi if advertisement_data.rssi is not None else -100
    name = advertisement_data.local_name or device.name or "?"
    mac = device.address
    phy = ""
    if advertisement_data.platform_data and len(advertisement_data.platform_data) >= 4:
        primary_phy = advertisement_data.platform_data[2]
        phy = {1: "1M", 2: "2M", 3: "Coded"}.get(primary_phy, f"PHY{primary_phy}")

    # Extract raw AD payload from platform_data[4] if available.
    raw_adv = b""
    if advertisement_data.platform_data and len(advertisement_data.platform_data) >= 5:
        raw_adv = advertisement_data.platform_data[4] or b""

    t_rel = time_mod.time() - _scan_start
    print(
        f"{t_rel:>9.3f}s  {mac:<18s}  {name:<24s}  {rssi:>4d} dBm  "
        f"{phy or '':>5s}  {raw_adv.hex()}"
    )


async def main():
    parser = argparse.ArgumentParser(
        description="Scan for BLE advertisements via an ESP32 adv2uart firmware.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  %(prog)s                         scan with default settings\n"
            "  %(prog)s --port /dev/ttyACM1      use a different serial port\n"
            "  %(prog)s --timeout 5 --unique     scan for 5 s, one entry per device\n"
            "  %(prog)s --raw                    show raw AD payload hex\n"
            "  %(prog)s -w 11:22:33:44:55:66     whitelist: only show this device\n"
            "  %(prog)s -b AABBCC                blacklist: hide all devices with that OUI\n"
            "  %(prog)s -w AA:BB:CC -b DE:AD    whitelist prefix + blacklist prefix\n"
        ),
    )
    parser.add_argument(
        "--port", "-p",
        default=os.environ.get("ADV2UART_PORT", "/dev/ttyACM0"),
        help="Serial port for the ESP32 (default: %(default)s, or $ADV2UART_PORT)",
    )
    parser.add_argument(
        "--baud", "-b",
        type=int,
        default=2000000,
        help="Serial baud rate (default: %(default)s)",
    )
    parser.add_argument(
        "--timeout", "-t",
        type=float,
        default=10.0,
        help="Scan duration in seconds (default: %(default)s)",
    )
    parser.add_argument(
        "--raw", "-r",
        action="store_true",
        help="Show raw AD payload hex in a separate column",
    )
    parser.add_argument(
        "--no-name",
        action="store_true",
        help="Hide the device name column",
    )
    parser.add_argument(
        "--unique", "-u",
        action="store_true",
        help="Print only the first sighting of each device (no duplicates)",
    )
    parser.add_argument(
        "--whitelist", "-w",
        action="append",
        default=[],
        metavar="MAC[/PREFIX_LEN]",
        help=(
            "Only show devices matching this MAC address or prefix. "
            "Can be repeated. Example: --whitelist 11:22:33:44:55:66 --whitelist AABBCC"
        ),
    )
    parser.add_argument(
        "--blacklist",
        action="append",
        default=[],
        metavar="MAC[/PREFIX_LEN]",
        help=(
            "Exclude devices matching this MAC address or prefix. "
            "Can be repeated. Example: --blacklist 00:11:22 --blacklist FFFFFF"
        ),
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable debug logging",
    )
    args = parser.parse_args()

    level = logging.DEBUG if args.verbose else logging.WARNING
    logging.basicConfig(level=level, format="%(levelname)s: %(message)s")

    # ── Column header ──────────────────────────────────────────────────────
    global _scan_start
    _scan_start = time_mod.time()
    header = (
        f"{'Time':>10s}  {'MAC':<18s}  "
        f"{'Name':<24s}  {'RSSI':>4s}  {'PHY':>7s}  {'  Advertisement data (hex)':<48s}"
    )
    if args.raw:
        header += "  Raw AD"
    print(header)
    print("-" * len(header))

    # ── Build the scanner with adv2uart backend ────────────────────────
    # Flatten comma-separated filter values
    whitelist = []
    for item in args.whitelist:
        whitelist.extend(x.strip() for x in item.split(",") if x.strip())
    blacklist = []
    for item in args.blacklist:
        blacklist.extend(x.strip() for x in item.split(",") if x.strip())

    if whitelist:
        logger.warning("Whitelist: %s", " ".join(whitelist))
    if blacklist:
        logger.warning("Blacklist: %s", " ".join(blacklist))

    scanner_kwargs = {
        "detection_callback": on_advertisement,
        "backend": BleakScannerAdv2Uart,
        "port": args.port,
        "baud": args.baud,
        "whitelist": whitelist,
        "blacklist": blacklist,
    }

    seen = set()

    start = time_mod.time()

    async def _scan_loop():
        """Inner scan loop: yields devices until timeout."""
        deadline = start + args.timeout if args.timeout else float("inf")

        async with BleakScanner(**scanner_kwargs) as scanner:
            logger.info("Scanning on %s …", args.port)

            if args.unique:
                async for device, adv in scanner.advertisement_data():
                    if time_mod.time() >= deadline:
                        break
                    if device.address in seen:
                        continue
                    seen.add(device.address)
                    on_advertisement(device, adv)
            else:
                await asyncio.sleep(args.timeout if args.timeout else 3600)

    try:
        await asyncio.wait_for(_scan_loop(), timeout=args.timeout + 10)

    except asyncio.TimeoutError:
        pass  # normal timeout
    except KeyboardInterrupt:
        logger.warning("Interrupted by user")
    except FileNotFoundError:
        logger.error("Serial port %s not found.\n", args.port)
        sys.exit(1)
    except PermissionError:
        logger.error("Permission denied opening %s.\n", args.port)
        sys.exit(1)
    except Exception:
        logger.exception("Scan error")
        sys.exit(1)

    elapsed = time_mod.time() - start
    print(f"\nScan finished — {elapsed:.1f} s elapsed")


if __name__ == "__main__":
    asyncio.run(main())
