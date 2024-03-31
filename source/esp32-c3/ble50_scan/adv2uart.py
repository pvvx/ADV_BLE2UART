#!/usr/bin/env python3

import sys
import time
import serial
import binascii
from pynput import keyboard

crctable = (
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
0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 )
def crc16(data: bytearray, length):
	crc = 0xFFFF
	for i in range(0, length):
		crc = (crc >> 8) ^ crctable[(crc ^ data[i]) & 0xFF]
	return crc 

class BLE2UART:
	def __init__(self, port, baud = 921600):
		self.reopen(baud, port)
	def reopen(self, baud, port = None):
		if port == None:
			if self.port == None:
				print ('ReOpen ?')
				return False
			else:
				self._port.close();
				port = self.port
			print ('ReOpen %s, %d bit/s...' % (port, baud), end = ' ')
		else:
			print ('Open %s, %d bit/s...' % (port, baud), end = ' ')
		try:
			self._port = serial.Serial(port, baud, \
									   serial.EIGHTBITS,\
									   serial.PARITY_NONE, \
									   serial.STOPBITS_ONE)
			self._port.setRTS(True)
			self._port.setDTR(True)
			time.sleep(0.5)
			self._port.flushOutput()
			self._port.flushInput()
			self._port.reset_output_buffer()
			self._port.reset_input_buffer()
			self._port.setRTS(False)
			self._port.setDTR(False)
			self._port.timeout = 0.1
		except OSError as err:
			print("OS error:", err)
			sys.exit(1)
		except ValueError:
			print("Could not convert data to an integer.")
			sys.exit(1)
		except Exception as err:
			print(f"Unexpected {err=}, {type(err)=}")
			raise
			sys.exit(1)
		except:
			print('Error!')
			sys.exit(1)
		print('ok')
		self.port = port
		return True
	def read(self, rdlen):
		try:
			rblk = self._port.read(rdlen)
		except OSError as err:
			print("OS error:", err)
			sys.exit(1)
		except:
			print('\rError read %s!' % (self.port))
			sys.exit(1)
		return rblk
	def write(self, blk):
		try:
			self._port.write(blk)
		except OSError as err:
			print("OS error:", err)
			sys.exit(1)
		except:
			print('\rError write %s!' % (self.port))
			sys.exit(1)
		return True
	def command(self, blk):
		ac = crc16(blk, len(blk))
		b = blk+bytearray([ac&0xFF,(ac>>8)&0xFF])
		self.write(b)
		self._port.flushOutput()
		print("cmd:", blk.hex())
		time.sleep(0.02)
		return True
	def add_mac_list(self, mac, m = 2):
		b = bytearray(7)
		b[0] = m
		b[1] = mac[5]
		b[2] = mac[4]
		b[3] = mac[3]
		b[4] = mac[2]
		b[5] = mac[1]
		b[6] = mac[0]
		return self.command(b)
	def close(self):
		return self._port.close()

def on_press(key):
    if key == keyboard.Key.esc:
        return False

def main():
	if(len(sys.argv) < 2):
		print("Usage: adv2uart <COM Port>")
		sys.exit(1)
	if(sys.argv[1] == "-h"):
		print("Usage: adv2uart <COM Port>")
		sys.exit(0)
	print("Press 'ESC' to exit")
	print ('Connecting to '+sys.argv[1]+' ...')
	dv = BLE2UART(sys.argv[1], 921600)
	data = bytearray() #[] #dv.read(1)
	#if data != None and len(data) > 0:
	#	if data[0] == 0:
	#		data = data[1:]				
	dv.command(b'\x04') #clear w/b list
	dv.add_mac_list(binascii.unhexlify('381f8dd93cb6'), 3) #add black list
	dv.add_mac_list(binascii.unhexlify('381f8d941e11'), 3)
	dv.add_mac_list(binascii.unhexlify('381f8dd8b52d'), 3)
	dv.add_mac_list(binascii.unhexlify('381f8d942ef9'), 3)
	dv.add_mac_list(binascii.unhexlify('381f8dd93b3a'), 3)
	dv.add_mac_list(binascii.unhexlify('39ee85f86c71'), 3)
	dv.add_mac_list(binascii.unhexlify('1c90ffdc0cc6'), 3)
	dv.add_mac_list(binascii.unhexlify('1c90ffd8ba69'), 3)
	
	dv.command(b'\x01\x32\x30\x00') #Start pas.scan 1M and Coded PHY [bit0:1], filter .., Windows: 0x0030 * 0.625 = 30 ms
	with keyboard.Listener(on_press=on_press) as listener:
		while listener.running:
			blk = dv.read(64)
			if blk != None and len(blk) > 0:
				data += blk
				if len(data) >= 13:
					while(len(data) > 0 and data[0] + 13 <= len(data)):
						l = data[0]
						if crc16(data, l+13) == 0:
							rssi = data[1:2].hex() 
							adtp = data[2:3].hex() 
							evtp = data[3:4].hex()
							phys = data[4:5].hex()
							xmac = bytes([data[10], data[9], data[8], data[7], data[6], data[5]])
							if data[4] == 0xff:
								x = data[3]
								if x == 0:
									print('resp:', rssi, adtp)
								else:
									if x < (len(data) - 7):
										if x == 6:
											print('resp:', rssi, adtp, xmac.hex())
										else:
											print('resp:', rssi, adtp, data[5:x+5].hex())
									else:
										print('blk:', data.hex())
							else:
								mac = xmac.hex() 
								dump = data[11:l+11].hex()
								print('adv:', rssi, evtp, adtp, phys, mac, dump)
							data = data[l + 13:]
						else:
							data = data[1:]
	dv.command(b'\x01\x00\x00\x00') # Stop scan
	dv.close()  # close the connection
	sys.exit(0)

if __name__ == '__main__':
	main()
