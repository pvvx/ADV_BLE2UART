/*
 * scanning.h
 *
 *  Created on: 20.11.2021
 *      Author: pvvx
 */

#ifndef SCANNING_H_
#define SCANNING_H_

enum {
	CMD_ID_INFO		= 0x00,
	CMD_ID_SCAN 	= 0x01, //  Scan on/off, parameters
	CMD_ID_WMAC		= 0x02, // add white mac
	CMD_ID_BMAC 	= 0x03, // add black mac
	CMD_ID_CLRM		= 0x04, // clear mac list
	CMD_ID_PRNT		= 0x05, // print debug message
	CMD_ID_GPIO		= 0x06, // TB-03F-KIT GPIO read/write/config
	CMD_ID_LED		= 0x07, // TB-03F-KIT LED test
	CMD_ID_UART		= 0x08, // UART test/config
	CMD_ID_RFSDK	= 0x09, // RF/SDK runtime tuning
	CMD_ID_VERSION	= 0x0a, // HW/FW/SDK version tuple
	CMD_ID_TXADV	= 0x0b, // transmit custom advertisement
	CMD_ID_CONN		= 0x0c, // BLE ACL connection (central role): connect/disconnect/status
	CMD_ID_TXDATA	= 0x0d, // send ATT Write Command to connected peer
	CMD_ID_RXDATA	= 0x0e  // (async) ATT data received from connected peer
} CMD_ID_KEYS;

#define MAC_MAX_SCAN_LIST	64

enum {
	WHITE_LIST,
	BALCK_LIST
} mode_mac_list_e;

typedef struct _mac_list_t {
	u8	mode;	// mode_mac_list_e
	u8	count;
	u8	filtr;
	u8	res;
	u8	mac[MAC_MAX_SCAN_LIST][6];
} mac_list_t;

extern mac_list_t mac_list;

#define HEAD_CRC_ADD_LEN	13

void scan_task(void);
void ble_ext_adv_callback(u8 *p);
void ble_adv_callback(u8 *p);
void ble_le_periodic_adv_callback(u8 *p);
void ble_le_periodic_adv_sync_established_callback(u8 *p);
void ble_le_periodic_adv_sync_lost_callback(u8 *p);

#endif /* SCANNING_H_ */
