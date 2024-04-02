#!/usr/bin/env python3

import sys
import time
import serial
import binascii
import logging
import argparse
import os
import json
import codecs
from construct import *


class Mac_Wb_List:
    WHITE_LIST = [
        # 'aabbccddeef0'  # Example of white-listed MAC address
        # 'aabbccddeef1'
    ]

    BLACK_LIST = [
        # 'aabbccddeef0'  # Example of black-listed MAC address
        # 'aabbccddeef1'
    ]


crc_table = (
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
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
)

HEX = codecs.getencoder('hex')


def crc_16(data, length):
    crc = 0xFFFF
    for i in range(0, length):
        crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xFF]
    return crc


adv_scanning = Struct(
    "flag" / BitStruct(
        "address_type" / Enum(BitsInteger(2),  # Bits 6 and 7. Generally PUBLIC
            PUBLIC = 0,
            RANDOM = 1,
            RESOLVE_PRIVATE_PUBLIC = 2,
            RESOLVE_PRIVATE_RANDOM = 3,
        ),
        "address_type_filter_random" / Flag,  # Bits 4 and 5: address_type_filter. Generally both.
        "address_type_filter_private" / Flag,
        "DUP_FILTER_ENABLE" / Flag,  # Bit 3 - (filter duplicates) True if DUP_FILTER_ENABLE, False if DUP_FILTER_DISABLE
        "SCAN_TYPE_ACTIVE" / Flag,  # Bit 2 - True if SCAN_TYPE_ACTIVE, False if SCAN_TYPE_PASSIVE
        "SCAN_PHY_CODED" / Flag,  # Bit 1 - True if Coded PHY. SCAN_PHY_1M_CODED = SCAN_PHY_CODED | SCAN_PHY_1M
        "SCAN_PHY_1M" / Flag,  # Bit 0 - True if 1M PHY
        # Set both SCAN_PHY_CODED and SCAN_PHY_1M to False to disable scan
    ),
    "window_ms" / ExprAdapter(Int16sl,
        decoder=lambda obj, ctx: int(float(obj) * 0.625),
        encoder=lambda obj, ctx: int(float(obj) / 0.625)
    )
)


class Command:
    CMD_ID_INFO = b'\x00'
    CMD_ID_SCAN = b'\x01'  # Scan on/off, (len_cmd = 3: parameters)
    CMD_ID_WMAC = b'\x02'  # add white mac (len_cmd = 6: mac)
    CMD_ID_BMAC = b'\x03'  # add black mac (len_cmd = 6: mac)
    CMD_ID_CLRM = b'\x04'  # clear mac list (len_cmd = 0, mac=000000000000)
    # CMD_ID_PRNT = b'\x05'  # debug print
    DEBUG_PRINT = bytearray.fromhex("05 ff ff ff 00 00 00 00 00 00")
    START_SCAN  = CMD_ID_SCAN + adv_scanning.build(
        {
            "flag": {
                "address_type": "PUBLIC",
                "address_type_filter_random": True,
                "address_type_filter_private": True,
                "DUP_FILTER_ENABLE": False,
                "SCAN_TYPE_ACTIVE": False,
                "SCAN_PHY_CODED": True,
                "SCAN_PHY_1M": True
            },
            "window_ms": 30
        }
    )
    STOP_SCAN   = CMD_ID_SCAN + b'\x00\x00\x00'


class Ble2Uart:
    def __init__(self, port=None, baud=921600, timeout=0.1):
        self.sync = None
        self.data = None
        self.ser = None

        self.timeout = timeout
        self.baud = baud
        self.port = None

        self.reopen(baud, port, timeout)

    def reopen(self, baud=None, port=None, timeout=None):
        self.data = bytearray()
        self.sync = None
        if timeout:
            self.timeout = timeout
        if baud:
            self.baud = baud
        if port:
            self.port = port
        if not self.port:
            logging.warning('ReOpen ?')
            return False
        if self.ser and self.ser.isOpen():
            self.ser.close()
            logging.warning(
                'ReOpen %s, %d bit/s, timeout: %s secs...',
                self.port, self.baud, self.timeout
            )
        else:
            logging.warning(
                'Open %s, %d bit/s, timeout: %s secs...',
                self.port, self.baud, self.timeout
            )
        try:
            self.ser = serial.serial_for_url(port, self.baud)
            time.sleep(0.05)
            self.ser.setDTR(True)
            self.ser.setRTS(True)
            time.sleep(0.05)
            self.ser.setDTR(False)
            self.ser.setRTS(False)
            self.ser.timeout = self.timeout
        except OSError as err:
            logging.critical("OS error: %s. Terminating.", err)
            sys.exit(1)
        except ValueError:
            logging.critical(
                "Could not convert data to an integer. Terminating."
            )
            sys.exit(1)
        except Exception as err:
            logging.critical("Unexpected %s - %s. Terminating.", err, type(err))
            sys.exit(1)
        except:
            logging.critical('Generic error. Terminating.')
            sys.exit(1)
        logging.warning('Connected')
        self.sync = False
        return True

    def read(self, rdlen):
        try:
            rblk = self.ser.read(rdlen)
        except OSError as err:
            logging.critical("OS error: %s. Terminating.", err)
            sys.exit(1)
        except Exception as e:
            logging.critical('%s read error: %s Terminating.', self.port, e)
            sys.exit(1)
        except:
            logging.critical('Received program termination command.')
            sys.exit(1)
        return rblk

    def write(self, blk):
        try:
            self.ser.write(blk)
        except OSError as err:
            logging.critical("OS error: %s. Terminating.", err)
            sys.exit(1)
        except Exception as e:
            logging.critical('Write error %s: %s. Terminating.', self.port, e)
            sys.exit(1)
        return True

    def command(self, cmd):
        blk = bytearray([d for d in cmd])
        ac = crc_16(blk, len(blk))
        b = blk + bytearray([ac & 0xFF, (ac >> 8) & 0xFF])
        self.write(b)
        self.ser.flushOutput()
        logging.debug(
            "send cmd: %s [%s] %s",
            HEX(b[0:1])[0], HEX(b[1:-2])[0], HEX(b[-2:])[0]
        )
        time.sleep(0.05)
        return True

    def add_mac_list(self, mac, cmd=Command.CMD_ID_WMAC):
        return self.command(bytearray([cmd[0]] + [i for i in mac[::-1]]))

    def close(self):
        return self.ser.close()

    def read_adv(self):
        """
        read_adv(): read serial input, detect advertisements and decode commands.

        Return: rssi, evtp, adtp, phys, mac, payload

        Input packet format (commands and advertisements have different content):

          - Header: 5 bytes (fixed for commands and advertisements)

            - the first byte (l) is the length of the payload (if command: 0)

            - rssi (if command: command number)

            - adtp:
                - BT4.2: pa->event_type
                - ext_adv: pa->event_type
                - periodic_adv: pExt->subEventCode
                (if command: mac array position, or max number of positions)
                (if command 0=CMD_ID_INFO: version in BCD format; 0x34 -> '3.4')

                ADV_REPORT_EVENT_TYPE_ADV_IND           = 0x00,
                ADV_REPORT_EVENT_TYPE_DIRECT_IND        = 0x01,
                ADV_REPORT_EVENT_TYPE_SCAN_IND          = 0x02,
                ADV_REPORT_EVENT_TYPE_NONCONN_IND       = 0x03,
                ADV_REPORT_EVENT_TYPE_SCAN_RSP          = 0x04,

            - evtp:
                - BT4.2: pa->adr_type & 0x0f
                - ext_adv: (pa->address_type & 0x0f) | (pa->direct_address_type << 4)
                - periodic_adv: periodic_adv.advAddrType & 0x0f
                (if command: length of the command data; can be 0)

                PUBLIC = 0,
                RANDOM = 1,
                RESOLVE_PRIVATE_PUBLIC = 2,
                RESOLVE_PRIVATE_RANDOM = 3,

            - phys
                - bt4.2: 0x00
                - ext_adv: pa->primary_phy | (pa->secondary_phy << 4)
                - periodic_adv: periodic_adv.advPHY
                (if command: 0xff)

                BLE_PHY_1M              = 0x01,
                BLE_PHY_2M              = 0x02,
                BLE_PHY_CODED           = 0x03,

          - mac: 6 bytes, fixed for commands and advertisements
                for advertisements: MAC in scope
                (if command CMD_ID_CLRM: 000000000000)
                (if command CMD_ID_INFO: public MAC address of the device)
                (if command CMD_ID_SCAN: adv_scanning parameters; see length)
                (if CMD_ID_WMAC and CMD_ID_BMAC: MAC in scope)

          - payload: variable length 'l' (if command: not present)

          - crc: 2 bytes, fixed for commands and advertisements
                CRC16 of the total packet length

        header (5 bytes) + mac (6 bytes) + crc (2 bytes) = 13 bytes
        total packet length: l + 13

        Examples of the structure of some commands:
        - Command CMD_ID_INFO: 00 00 01 06 ff a4c138bfff34
        - Command CMD_ID_CLRM: 00 04 40 00 ff 000000000000
        - Command CMD_ID_BMAC: 00 03 01 06 ff aabbccddeeff
        - Command CMD_ID_SCAN: 00 01 10 03 ff 000000003033
        """
        rssi = ""
        adtp = ""
        evtp = ""
        phys = ""
        mac = ""
        payload = ""  # valued if advertisement
        blk = self.read(64)
        if blk is not None and len(blk) > 0:
            self.data += blk
            if len(self.data) >= 13:  # minimum packet size (11 bytes + crc16)
                while (
                    len(self.data) > 0 and self.data[0] + 13 <= len(self.data)  # data exists and includes at least a full packet
                ):
                    len_payload = self.data[0]
                    if crc_16(self.data, len_payload + 13) == 0:  # l + 13 = total packet size
                        logging.verbose(
                            "packet: %s %s",
                            HEX(self.data[:11]),
                            HEX(self.data[11: len_payload + 11])
                        )
                        self.sync = True
                        rssi = Int8sl.parse(self.data[1:2])
                        adtp = HEX(self.data[2:3])[0]
                        evtp = HEX(self.data[3:4])[0]
                        phys = HEX(self.data[4:5])[0]
                        mac = HEX(bytearray(self.data[10:4:-1]))[0]
                        if self.data[4] == 0xff:  # phys = 0xff -> cmd response received; rssi is the cmd number
                            # 0, cmd id, position, length, 0xff; all commands have 11 bytes packet length
                            logging.verbose(
                                'resp-blk: %s %s %s %s %s %s',
                                len_payload, rssi, adtp, evtp, phys, mac
                            )
                            cmd = self.data[1:2]
                            len_cmd = self.data[3]  # evtp = length of the command data included in mac (max=6)
                            if self.data[1:11] == Command.DEBUG_PRINT:
                                payload = (
                                    self.data[11: len_payload + 11]
                                ).decode()
                                logging.warning("Debug message: %s", payload)
                            elif cmd == Command.CMD_ID_INFO:
                                logging.warning(
                                    'resp: %s=CmdInfo, version: %s; '
                                    'local MAC: %s', rssi, adtp, mac.decode()
                                )  # command id, total number of definable mac list elements
                            elif cmd == Command.CMD_ID_CLRM:
                                logging.warning(
                                    'resp: %s=ClearMacList, definable'
                                    ' elements: %s', rssi, self.data[2]
                                )  # command id, total number of definable mac list elements
                            elif cmd in [
                                Command.CMD_ID_WMAC,
                                Command.CMD_ID_BMAC
                            ]:  # it includes a mac; print it in reverse
                                logging.warning(  # command id, count, mac
                                    'resp: %s=Add %s List, '
                                    'position %s, MAC: %s', rssi,
                                    "WHITE" if cmd == Command.CMD_ID_WMAC
                                    else "BLACK", self.data[2], mac.decode()
                                )
                            elif cmd == Command.CMD_ID_SCAN:
                                if self.data[5] == 0:
                                    logging.warning(
                                        'resp: %s=SCAN Disable %s',
                                        rssi, adtp,  # command id, single numeric attribute
                                    )
                                else:
                                    logging.warning(
                                        'resp: %s=SCAN Enable, '
                                        'MAC addresses in list: %s, %s=%s',
                                        rssi, self.data[2],  # command id, MAC addrs in list
                                        HEX(self.data[5: len_cmd + 5]),
                                        adv_scanning.parse(
                                            self.data[5: len_cmd + 5]
                                        )
                                    )
                            else:
                                logging.error(
                                    'blk: %s', HEX(self.data[0: len_payload + 11])
                                )
                        else:  # advertisement received (payload is valued)
                            payload = HEX(self.data[11: len_payload + 11])[0]
                            logging.info(
                                'adv: %s %s %s %s %s %s %s',
                                len_payload, rssi, evtp.decode(), adtp.decode(),
                                phys.decode(), mac.decode(), payload.decode()
                            )
                        self.data = self.data[len_payload + 13:]  # remove the processed packet
                    else:  # CRC error
                        if self.sync:
                            logging.error(
                                "CRC error. Discard %02x", self.data[0]
                            )
                        else:
                            logging.debug("Discard %02x", self.data[0])
                        self.data = self.data[1:]  # discard 1 byte
        return rssi, evtp, adtp, phys, mac, payload

    def black_white_list(
        self,
        white_list=Mac_Wb_List.WHITE_LIST,
        black_list=Mac_Wb_List.BLACK_LIST,
        info=True,
        clear=True,
        start=True,
    ):
        if info:
            self.command(Command.CMD_ID_INFO)
        if clear:
            self.command(Command.CMD_ID_CLRM)  # clear w/b list
        for i in white_list:
            self.add_mac_list(binascii.unhexlify(i), Command.CMD_ID_WMAC)
        for i in black_list:
            self.add_mac_list(binascii.unhexlify(i), Command.CMD_ID_BMAC)
        if start:
            self.command(Command.START_SCAN)


def setup_logging(
        default_path='adv2uart-log.json',
        default_level=logging.WARNING,
        env_key='ADV2UART_CFG'
):
    path = default_path
    value = os.getenv(env_key, None)
    if value:
        path = value
    if os.path.exists(path):
        with open(path, 'rt') as f:
            config = json.load(f)
        try:
            logging.config.dictConfig(config)
        except Exception:
            logging.basicConfig(level=default_level)
    else:
        logging.basicConfig(level=default_level)


########################### MAIN ###################################
def main():
    parser = argparse.ArgumentParser(
        epilog='BLE TLSR825x_ADV_BLE2UART scanner'
    )
    parser.add_argument(
        '-d',
        '--debug',
        dest='debug',
        action='store_true',
        help='Print debug information'
    )
    parser.add_argument(
        '-v',
        '--verbose',
        dest='verbose',
        action='store_true',
        help='Print verbose information'
    )
    parser.add_argument(
        '-i',
        '--info',
        dest='info',
        action='store_true',
        help='Print limited debug information'
    )
    parser.add_argument(
        '-b',
        '--baudrate',
        dest='baudrate',
        type=int,
        help='serial connection baudrate (default: 921600)',
        default=921600
    )
    parser.add_argument(
        '-p', '--port',
        dest='serial_port',
        help="Serial port; default = COM11 or /dev/ttyUSB0",
        default=['COM11'] if os.name == 'nt' else ['/dev/ttyUSB0'],
        nargs=1,
        metavar='PORT'
    )
    parser.add_argument(
        '-t',
        '--timeout',
        dest='timeout',
        type=float,
        help='serial port read timeout in seconds (default: 0.1)',
        default=0.1
    )
    parser.add_argument(
        '-n',
        '--number',
        dest='number',
        type=int,
        help='Number of advertisements to process (default is 0 = infinite)',
        default=0
    )

    args = parser.parse_args()

    setup_logging()
    loglevel = None
    if args.info:
        loglevel = logging.INFO
    if args.verbose:
        loglevel = logging.VERBOSE
    if args.debug:
        loglevel = logging.DEBUG
    if loglevel:
        logging.getLogger().setLevel(loglevel)
        logging.warning("Set loglevel %s", loglevel)

    logging.warning("Press 'ESC' to exit")
    logging.warning('Connecting to %s...' + args.serial_port[0])

    dv = Ble2Uart(
        port=args.serial_port[0],
        baud=args.baudrate,
        timeout=args.timeout
    )
    dv.read(64)  # flush
    dv.black_white_list()
    count = 0
    while True:
        rssi, evtp, adtp, phys, mac, payload = dv.read_adv()
        if payload and args.number:
            count += 1
            if count == args.number:
                break
    dv.command(Command.STOP_SCAN)
    dv.read_adv()
    dv.close()  # close the connection
    sys.exit(0)


# Custom logging VERBOSE (5)
logging.VERBOSE = 5
logging.addLevelName(logging.VERBOSE, "VERBOSE")
logging.Logger.verbose = lambda inst, msg, *args, **kwargs: inst.log(
    logging.VERBOSE, msg, *args, **kwargs)
logging.LoggerAdapter.verbose = lambda inst, msg, *args, **kwargs: inst.log(
    logging.VERBOSE, msg, *args, **kwargs)
logging.verbose = lambda msg, *args, **kwargs: logging.log(
    logging.VERBOSE, msg, *args, **kwargs)

if __name__ == '__main__':
    main()
