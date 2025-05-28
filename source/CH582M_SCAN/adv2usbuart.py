#!/usr/bin/env python3

import sys
import time
import serial
import binascii
from pynput import keyboard

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
		#ac = crc16(blk, len(blk))
		#b = blk+bytearray([ac&0xFF,(ac>>8)&0xFF])
		self.write(blk)
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

	dv.command(b'\x01\x00\x00\x00') # Stop scan
	dv.command(b'\x00') # get version
	dv.command(b'\x04') # clear w/b list
	
	#dv.add_mac_list(binascii.unhexlify('381f8dd93cb6'), 3) # add black list
	#dv.add_mac_list(binascii.unhexlify('381f8d941e11'), 3)
	#dv.add_mac_list(binascii.unhexlify('381f8dd8b52d'), 3)
	#dv.add_mac_list(binascii.unhexlify('381f8d942ef9'), 3)
	#dv.add_mac_list(binascii.unhexlify('381f8dd93b3a'), 3)
	#dv.add_mac_list(binascii.unhexlify('39ee85f86c71'), 3)
	#dv.add_mac_list(binascii.unhexlify('1c90ffdc0cc6'), 3)
	#dv.add_mac_list(binascii.unhexlify('1c90ffd8ba69'), 3)
	#dv.add_mac_list(binascii.unhexlify('33b6e3cb1c05'), 3)
	#dv.add_mac_list(binascii.unhexlify('b8060d012ce6'), 3)
	
	dv.command(b'\x01\x35\x40\x1F') #Start pas.scan 1M and Coded PHY [bit0:2], passive = 0 or active = 1 [bit3], mode [bit4:5], filter [bit6:7], Windows: 0x1F40 * 0.625 = 5000 ms
	with keyboard.Listener(on_press=on_press) as listener:
		while listener.running:
			blk = dv.read(32)
			if blk != None and len(blk) > 0:
				data += blk
				if len(data) >= 11:
					while(len(data) > 0 and data[0] + 11 <= len(data)):
						l = data[0]
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
								if x < (len(data) - 5):
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
						data = data[l + 11:]
	dv.command(b'\x01\x00\x00\x00') # Stop scan
	dv.close()  # close the connection
	sys.exit(0)

if __name__ == '__main__':
	main()
