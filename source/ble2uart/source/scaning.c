/*
 * scaning.c
 *
 *  Created on: 20.11.2021
 *      Author: pvvx
 */
#include "tl_common.h"
#include "ble.h"
#include "stack/ble/ble.h"
#include "app_config.h"
//#include "app.h"
#include "drv_uart.h"
#include "crc.h"
#include "utils.h"
#include "scaning.h"

typedef struct {
#if defined(GPIO_LED_R)
	u32 tr;
#endif
#if defined(GPIO_LED_G)
	u32 tg;
#endif
#if defined(GPIO_LED_B)
	u32 tb;
#endif
#if defined(GPIO_LED_W)
	u32 tw;
#endif
#if defined(GPIO_LED_E)
	u32 te;
#endif
}leds_tik_t;

#define TIMER_LED_R	(2*CLOCK_16M_SYS_TIMER_CLK_1S)
#define TIMER_LED_G	(128*CLOCK_16M_SYS_TIMER_CLK_1MS)
#define TIMER_LED_B	(128*CLOCK_16M_SYS_TIMER_CLK_1MS)
#define TIMER_LED_W	(128*CLOCK_16M_SYS_TIMER_CLK_1MS)
#define TIMER_LED_E	(64*CLOCK_16M_SYS_TIMER_CLK_1MS)

leds_tik_t leds;

#define MYFIFO_BLK_SIZE		(EXTADV_RPT_DATA_LEN_MAX + HEAD_CRC_ADD_LEN) // 229+12 = 241 bytes
MYFIFO_INIT(ad_fifo, MYFIFO_BLK_SIZE, 4); 	// (229+12)*4 = 964 bytes + sizeof(my_fifo_t)

RAM mac_list_t mac_list;

hci_le_periodicAdvSyncEstablishedEvt_t periodic_adv;

void send_resp(u8 cmd, u8 id, u8 *pmac, u8 len) {
	u8 *s = my_fifo_wptr(&ad_fifo);
	if(s) {
		memset(s, 0, HEAD_CRC_ADD_LEN);
		//s[0] = 0;
		s[1] = cmd; // rssi
		s[2] = id; // ev type
		s[3] = len; // addr type
		s[4] = 0xff; // phy = 0xff -> cmd response
		if(len)
			memcpy(&s[5], pmac, len);
		my_fifo_next(&ad_fifo);
	}
}

_attribute_ram_code_
int chk_mac(u8 *pmac) {
	int ret = 0;
	if(mac_list.count) {
		for(int i = 0; i < mac_list.count; i++) {
			if(memcmp(&mac_list.mac[i], pmac, 6) == 0) {
				ret = 1;
				break;
			}
		}
		if(mac_list.mode == BALCK_LIST)
			ret = !ret;
	} else
		ret = 1;
	return ret;
}

//////////////////////////////////////////////////////////
// scan event call back
//////////////////////////////////////////////////////////
//_attribute_ram_code_
__attribute__((optimize("-Os")))
void ble_adv_callback(u8 *p) {
	event_adv_report_t *pa = (event_adv_report_t *) p;
	u8 adlen = pa->len;
	u8 rssi = pa->data[adlen];
	if (pa->len
			&& pa->len <= EXTADV_RPT_DATA_LEN_MAX
			&& (pa->adr_type & mac_list.filtr) == 0
			&& rssi != 0
			&& chk_mac(pa->mac)) {
		u8 *s = my_fifo_wptr(&ad_fifo);
		if(s) {
			s[0] = adlen;
			s[1] = rssi;
			s[2] = pa->event_type;
			s[3] = pa->adr_type & 0x0f;
			s[4] = 0x00; // bt4.2
			memcpy(&s[5], pa->mac, 6);
			memcpy(&s[11], pa->data, adlen);
			my_fifo_next(&ad_fifo);
#if defined(GPIO_LED_G)
			gpio_write(GPIO_LED_B, 1);
			leds.tb = reg_system_tick;
#endif
		}
#ifdef 	GPIO_LED_R
		else {
			gpio_write(GPIO_LED_R, 1);
			leds.tr = reg_system_tick;
		}
#endif
	}
}

//////////////////////////////////////////////////////////
// scan event call back
//////////////////////////////////////////////////////////
//_attribute_ram_code_
__attribute__((optimize("-Os")))
void ble_ext_adv_callback(u8 *p) {
	//send_debug(p, 10);
	hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;
	extAdvEvt_info_t *pa = NULL;
	int offset = 0;
	for(int i=0; i<pExtAdvRpt->num_reports ; i++) {
		pa = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
		s8 rssi = pa->rssi; //secondary_phy; //data[adlen];
		int adlen = pa->data_length;
		offset += (EXTADV_INFO_LENGTH + pa->data_length);
		if (adlen
			&& adlen <= EXTADV_RPT_DATA_LEN_MAX
			&& (pa->address_type & mac_list.filtr) == 0
			&& rssi != 0
			&& chk_mac(pa->address)) {
			u8 *s = my_fifo_wptr(&ad_fifo);
			if(s) {
				s[0] = adlen;
				s[1] = rssi;
				s[2] = pa->event_type;
				s[3] = (pa->address_type & 0x0f) | (pa->direct_address_type << 4); // ext_adv_adr_type_t
				s[4] = pa->primary_phy | (pa->secondary_phy << 4);
				memcpy(&s[5], pa->address, 6);
				memcpy(&s[11], pa->data, adlen);
				my_fifo_next(&ad_fifo);
#if defined(GPIO_LED_G) && defined(GPIO_LED_B)
				if(pa->primary_phy == 1) {
					gpio_write(GPIO_LED_B, 1);
					leds.tb = reg_system_tick;
				} else if(pa->primary_phy == 3) {
					gpio_write(GPIO_LED_G, 1);
					leds.tg = reg_system_tick;
				}
#endif
			}
#ifdef 	GPIO_LED_R
			else {
				gpio_write(GPIO_LED_R, 1);
				leds.tr = reg_system_tick;
			}
#endif
		}
	}
}

void ble_le_periodic_adv_sync_lost_callback(u8 *p) {
	//hci_le_periodicAdvSyncLostEvt_t *pExt = (hci_le_periodicAdvSyncLostEvt_t *)p;
	periodic_adv.syncHandle = 0xffff;
}

void ble_le_periodic_adv_sync_established_callback(u8 *p) {
	hci_le_periodicAdvSyncEstablishedEvt_t *pExt = (hci_le_periodicAdvSyncEstablishedEvt_t *)p;
	memcpy(&periodic_adv, pExt, sizeof(periodic_adv));
}

void ble_le_periodic_adv_callback(u8 *p) {
	hci_le_periodicAdvReportEvt_t *pExt = (hci_le_periodicAdvReportEvt_t *)p;
	if(periodic_adv.syncHandle == pExt->syncHandle) {
		s8 rssi = pExt->RSSI; //secondary_phy; //data[adlen];
		int adlen = pExt->dataLength;
		if (adlen
			&& adlen <= EXTADV_RPT_DATA_LEN_MAX
			&& (periodic_adv.advAddrType & mac_list.filtr) == 0
			&& rssi != 0
			&& chk_mac(periodic_adv.advAddr)) { // rssi != 0
			u8 *s = my_fifo_wptr(&ad_fifo);
			if(s) {
				s[0] = adlen;
				s[1] = rssi;
				s[2] = pExt->subEventCode;
				s[3] = periodic_adv.advAddrType & 0x0f;
				s[4] = periodic_adv.advPHY;
				memcpy(&s[5], periodic_adv.advAddr, 6);
				memcpy(&s[11], pExt->data, adlen);
				my_fifo_next(&ad_fifo);
#if defined(GPIO_LED_G) && defined(GPIO_LED_B)
				if(periodic_adv.advPHY == 1) {
					gpio_write(GPIO_LED_B, 1);
					leds.tb = reg_system_tick;
				} else if(periodic_adv.advPHY == 3) {
					gpio_write(GPIO_LED_G, 1);
					leds.tg = reg_system_tick;
				}
#endif
			}
#ifdef 	GPIO_LED_R
			else {
				gpio_write(GPIO_LED_R, 1);
				leds.tr = reg_system_tick;
			}
#endif
		}
	}
}

//////////////////////////////////////////////////////////
// scan task
//////////////////////////////////////////////////////////
//_attribute_ram_code_
__attribute__((optimize("-Os")))
void scan_task(void) {
	u32 tt = reg_system_tick;
	crc_t crc;
	u8 *p = my_fifo_get(&ad_fifo);
	if(p) {
		int len = p[0] + HEAD_CRC_ADD_LEN - 2;
		crc = crcFast(p, len);
		p[len] = crc;
		p[len+1] = crc >> 8;
		if(uart_send(p, len + 2) > 0) {
			my_fifo_pop(&ad_fifo);
#ifdef 	TIMER_LED_E
			gpio_write(GPIO_LED_E, 1);
			leds.te = tt;
#endif
		}
#ifdef 	TIMER_LED_R
		else {
			gpio_write(GPIO_LED_R, 1);
			leds.tr = tt;
		}
#endif
	} else {
#ifdef 	TIMER_LED_R
		if(tt - leds.tr > TIMER_LED_R) {
			leds.tr = tt;
			gpio_write(GPIO_LED_R, 0);
		}
#endif
#ifdef 	TIMER_LED_G
		if(tt - leds.tg > TIMER_LED_G) {
			leds.tg = tt;
			gpio_write(GPIO_LED_G, 0);
		}
#endif
#ifdef 	TIMER_LED_B
		if(tt - leds.tb > TIMER_LED_B) {
			leds.tb = tt;
			gpio_write(GPIO_LED_B, 0);
		}
#endif
#ifdef 	TIMER_LED_W
		if(tt - leds.tw > TIMER_LED_W) {
			leds.tw = tt;
			gpio_write(GPIO_LED_W, 0);
		}
#endif
#ifdef 	TIMER_LED_E
		if(tt - leds.te > TIMER_LED_E) {
			leds.te = tt;
			gpio_write(GPIO_LED_E, 0);
		}
#endif
	}
	uint8_t buf[16];
	int len = uart_read(buf, sizeof(buf));
	if(len > 2 && crcFast(buf, len) == 0) {
#ifdef 	TIMER_LED_W
			gpio_write(GPIO_LED_W, 1);
			leds.tw = tt;
#endif
		int cmd = buf[0];
		if(cmd == CMD_ID_SCAN && len == 3 + 3) {
			start_adv_scanning(buf[1], buf[2] | (buf[3]<<8));
			send_resp(cmd, mac_list.count, &buf[1], 3);
		} else if(len == 6 + 3) {
			if(buf[0] == CMD_ID_WMAC) {
				mac_list.mode = WHITE_LIST;
				if(mac_list.count < MAC_MAX_SCAN_LIST) {
					memcpy(&mac_list.mac[mac_list.count], &buf[1], 6);
					mac_list.count++;
				}
				send_resp(cmd, mac_list.count, &buf[1], 6);
			} else if(buf[0] == CMD_ID_BMAC) {
				mac_list.mode = BALCK_LIST;
				if(mac_list.count < MAC_MAX_SCAN_LIST) {
					memcpy(&mac_list.mac[mac_list.count], &buf[1], 6);
					mac_list.count++;
				}
				send_resp(cmd, mac_list.count, &buf[1], 6);
			}
		} else if(len == 3) {
			if(buf[0] == CMD_ID_CLRM)  {
				mac_list.count = 0;
				send_resp(cmd, MAC_MAX_SCAN_LIST, &buf[1], 0);
			}
			else if(buf[0] == CMD_ID_INFO) {
				send_resp(cmd, SW_VERSION, mac_public, 6);
			}
		}
	}
    if (!gpio_read(KEY_USER)) {
        change_baud_rate();
    }
}
