/*
 * scanning.c
 *
 *  Created on: 20.11.2021
 *      Author: pvvx
 */
#include "app_config.h"
#include <stdint.h>
#include "tl_common.h"
#include "ble.h"
#include "stack/ble/ble.h"
#include "drv_uart.h"
#include "crc.h"
#include "utils.h"
#include "scanning.h"
#if DEBUG_MSG > 0
#include "app_printf.h"
#endif

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
static u8 manual_led_mask;
static u8 pending_baud_index = 0xff;
static u8 pending_baud_waiting;
static u32 pending_baud_tick;
static u8 key_baud_low_pending;
static u32 key_baud_low_tick;
RAM u8 rfsdk_state_init;
RAM u8 rfsdk_power;
RAM u8 rfsdk_cap;
RAM u8 rfsdk_channels[3];
RAM u8 rfsdk_coded_min_10ms;
RAM u8 txadv_running_phy;   // 0=stopped 1=legacy-1M 2=ext-1M 3=ext-Coded
RAM u16 txadv_interval_units;
RAM u8 txadv_adv_data_len;

#define MYFIFO_BLK_SIZE		(EXTADV_RPT_DATA_LEN_MAX + HEAD_CRC_ADD_LEN) // 229+12 = 241 bytes
MYFIFO_INIT(ad_fifo, MYFIFO_BLK_SIZE, 4); 	// (229+12)*4 = 964 bytes + sizeof(my_fifo_t)

#define KEY_BAUD_HOLD_US	800000

RAM mac_list_t mac_list;

hci_le_periodicAdvSyncEstablishedEvt_t periodic_adv;

void send_resp(u8 cmd, u8 id, u8 *pmac, u8 len) {
	if(len > 6)
		len = 6;
	u8 *s = my_fifo_wptr(&ad_fifo);
	if(s) {
		memset(s, 0, HEAD_CRC_ADD_LEN);
		//s[0] = 0;
		s[1] = cmd; // command number (position of the rssi)
		s[2] = id; // ev type
		s[3] = len; // addr type
		s[4] = 0xff; // phy = 0xff -> cmd response
		if(len)
			memcpy(&s[5], pmac, len);
		my_fifo_next(&ad_fifo);
	}
}

enum {
	CMD_STATUS_OK = 0,
	CMD_STATUS_ARGS = 1,
	CMD_STATUS_PIN = 2,
	CMD_STATUS_DENIED = 3,
	CMD_STATUS_VALUE = 4
};

/* pin_id = port-code: high nibble = port (A=0,B=1,C=2,D=3), low nibble = bit */
enum {
	BOARD_PIN_KEY_PA7        = 0x07,
	BOARD_PIN_LED_BLUE_PC2   = 0x22,
	BOARD_PIN_LED_RED_PC3    = 0x23,
	BOARD_PIN_LED_GREEN_PC4  = 0x24,
	BOARD_PIN_LED_YELLOW_PB4 = 0x14,
	BOARD_PIN_LED_WHITE_PB5  = 0x15,
	BOARD_PIN_UART_TX_PB1    = 0x11,
	BOARD_PIN_UART_RX_PA0    = 0x00
};

enum {
	LED_MASK_BLUE = BIT(0),
	LED_MASK_RED = BIT(1),
	LED_MASK_GREEN = BIT(2),
	LED_MASK_YELLOW = BIT(3),
	LED_MASK_WHITE = BIT(4),
	LED_MASK_ALL = LED_MASK_BLUE | LED_MASK_RED | LED_MASK_GREEN | LED_MASK_YELLOW | LED_MASK_WHITE
};

static int board_pin(u8 pin_id, GPIO_PinTypeDef *pin) {
	switch(pin_id) {
	case BOARD_PIN_KEY_PA7:
		*pin = KEY_USER;
		return 1;
	case BOARD_PIN_LED_BLUE_PC2:
		*pin = GPIO_LED_B;
		return 1;
	case BOARD_PIN_LED_RED_PC3:
		*pin = GPIO_LED_R;
		return 1;
	case BOARD_PIN_LED_GREEN_PC4:
		*pin = GPIO_LED_G;
		return 1;
	case BOARD_PIN_LED_YELLOW_PB4:
		*pin = GPIO_LED_E;
		return 1;
	case BOARD_PIN_LED_WHITE_PB5:
		*pin = GPIO_LED_W;
		return 1;
	case BOARD_PIN_UART_TX_PB1:
		*pin = UART_CH340_TX_PIN;
		return 1;
	case BOARD_PIN_UART_RX_PA0:
		*pin = UART_CH340_RX_PIN;
		return 1;
	default:
		return 0;
	}
}

static int led_pin_from_mask(u8 bit, GPIO_PinTypeDef *pin) {
	switch(bit) {
	case LED_MASK_BLUE:
		*pin = GPIO_LED_B;
		return 1;
	case LED_MASK_RED:
		*pin = GPIO_LED_R;
		return 1;
	case LED_MASK_GREEN:
		*pin = GPIO_LED_G;
		return 1;
	case LED_MASK_YELLOW:
		*pin = GPIO_LED_E;
		return 1;
	case LED_MASK_WHITE:
		*pin = GPIO_LED_W;
		return 1;
	default:
		return 0;
	}
}

static u8 board_pin_led_mask(u8 pin_id) {
	switch(pin_id) {
	case BOARD_PIN_LED_BLUE_PC2:
		return LED_MASK_BLUE;
	case BOARD_PIN_LED_RED_PC3:
		return LED_MASK_RED;
	case BOARD_PIN_LED_GREEN_PC4:
		return LED_MASK_GREEN;
	case BOARD_PIN_LED_YELLOW_PB4:
		return LED_MASK_YELLOW;
	case BOARD_PIN_LED_WHITE_PB5:
		return LED_MASK_WHITE;
	default:
		return 0;
	}
}

static u16 board_read_mask(void) {
	u16 mask = 0;
	if(gpio_read(KEY_USER))   mask |= BIT(0);
	if(gpio_read(GPIO_LED_B)) mask |= BIT(1);
	if(gpio_read(GPIO_LED_R)) mask |= BIT(2);
	if(gpio_read(GPIO_LED_G)) mask |= BIT(3);
	if(gpio_read(GPIO_LED_E)) mask |= BIT(4);
	if(gpio_read(GPIO_LED_W)) mask |= BIT(5);
	return mask;
}

static u8 led_read_mask(void) {
	u8 value = 0;
	if(gpio_read(GPIO_LED_B))
		value |= LED_MASK_BLUE;
	if(gpio_read(GPIO_LED_R))
		value |= LED_MASK_RED;
	if(gpio_read(GPIO_LED_G))
		value |= LED_MASK_GREEN;
	if(gpio_read(GPIO_LED_E))
		value |= LED_MASK_YELLOW;
	if(gpio_read(GPIO_LED_W))
		value |= LED_MASK_WHITE;
	return value;
}

static void led_write_mask(u8 mask, u8 value, int manual) {
	GPIO_PinTypeDef pin;
	mask &= LED_MASK_ALL;
	for(u8 bit = 1; bit <= LED_MASK_WHITE; bit <<= 1) {
		if((mask & bit) && led_pin_from_mask(bit, &pin)) {
			u8 state = (value & bit) ? 1 : 0;
			gpio_write(pin, state);
			if(manual) {
				if(state)
					manual_led_mask |= bit;
				else
					manual_led_mask &= ~bit;
			}
		}
	}
}

static void send_gpio_state(u8 status, u8 op, u8 pin_id) {
	u8 data[6];
	u16 mask = board_read_mask();
	GPIO_PinTypeDef pin;
	data[0] = op;
	data[1] = pin_id;
	data[2] = board_pin(pin_id, &pin) ? gpio_read(pin) : 0xff;
	data[3] = led_read_mask();
	data[4] = mask;
	data[5] = mask >> 8;
	send_resp(CMD_ID_GPIO, status, data, sizeof(data));
}

static void handle_gpio_command(u8 *buf, int cmd_len) {
	GPIO_PinTypeDef pin;
	u8 op = (cmd_len > 1) ? buf[1] : 0;
	u8 pin_id = (cmd_len > 2) ? buf[2] : 0;
	u8 status = CMD_STATUS_OK;
	u8 led_mask;
	if(op == 1 || op == 0) {
		u8 data[6];
		u16 mask = board_read_mask();
		data[0] = op;
		data[1] = pin_id;
		data[2] = board_pin(pin_id, &pin) ? gpio_read(pin) : 0xff;
		data[3] = led_read_mask();
		data[4] = mask;
		data[5] = mask >> 8;
		send_resp(CMD_ID_GPIO, board_pin(pin_id, &pin) ? CMD_STATUS_OK : CMD_STATUS_PIN, data, sizeof(data));
		return;
	}

	if(cmd_len < 2) {
		send_gpio_state(CMD_STATUS_ARGS, op, pin_id);
		return;
	}

	switch(op) {
	case 0:
		send_gpio_state(CMD_STATUS_OK, op, pin_id);
		return;
	case 1:
		if(cmd_len < 3 || !board_pin(pin_id, &pin))
			status = CMD_STATUS_PIN;
		break;
	case 2:
		if(cmd_len < 4 || !board_pin(pin_id, &pin)) {
			status = CMD_STATUS_PIN;
		} else {
			led_mask = board_pin_led_mask(pin_id);
			if(!led_mask) {
				status = CMD_STATUS_DENIED;
			} else {
				gpio_write(pin, buf[3] ? 1 : 0);
				if(buf[3])
					manual_led_mask |= led_mask;
				else
					manual_led_mask &= ~led_mask;
			}
		}
		break;
	case 3:
		if(cmd_len < 3 || !board_pin(pin_id, &pin)) {
			status = CMD_STATUS_PIN;
		} else {
			led_mask = board_pin_led_mask(pin_id);
			if(!led_mask) {
				status = CMD_STATUS_DENIED;
			} else {
				gpio_toggle(pin);
				if(gpio_read(pin))
					manual_led_mask |= led_mask;
				else
					manual_led_mask &= ~led_mask;
			}
		}
		break;
	case 4:
		if(cmd_len < 6 || !board_pin(pin_id, &pin)) {
			status = CMD_STATUS_PIN;
		} else if(pin_id == BOARD_PIN_UART_TX_PB1 || pin_id == BOARD_PIN_UART_RX_PA0 || (pin_id == BOARD_PIN_KEY_PA7 && buf[4])) {
			status = CMD_STATUS_DENIED;
		} else if(buf[5] > PM_PIN_PULLUP_10K) {
			status = CMD_STATUS_VALUE;
		} else {
			gpio_set_input_en(pin, buf[3] ? 1 : 0);
			gpio_set_output_en(pin, buf[4] ? 1 : 0);
			gpio_setup_up_down_resistor(pin, (GPIO_PullTypeDef)buf[5]);
		}
		break;
	default:
		status = CMD_STATUS_ARGS;
		break;
	}
	send_gpio_state(status, op, pin_id);
}

static void send_led_state(u8 status, u8 op, u8 mask) {
	u8 data[6];
	u16 board_mask = board_read_mask();
	data[0] = op;
	data[1] = mask & LED_MASK_ALL;
	data[2] = led_read_mask();
	data[3] = manual_led_mask;
	data[4] = board_mask;
	data[5] = board_mask >> 8;
	send_resp(CMD_ID_LED, status, data, sizeof(data));
}

static void handle_led_command(u8 *buf, int cmd_len) {
	u8 op = (cmd_len > 1) ? buf[1] : 0;
	u8 mask = (cmd_len > 2) ? buf[2] : LED_MASK_ALL;
	u8 value = (cmd_len > 3) ? buf[3] : 0;
	u8 status = CMD_STATUS_OK;

	mask &= LED_MASK_ALL;
	if(!mask) {
		send_led_state(CMD_STATUS_VALUE, op, mask);
		return;
	}
	switch(op) {
	case 0:
		led_write_mask(mask, value, 1);
		break;
	case 1:
		led_write_mask(mask, ~led_read_mask(), 1);
		break;
	case 2:
		led_write_mask(mask, 0, 1);
		break;
	case 3: {
		u8 count = (cmd_len > 4) ? buf[4] : 3;
		u8 delay_10ms = (cmd_len > 5) ? buf[5] : 5;
		if(count == 0)
			count = 1;
		if(count > 5)
			count = 5;
		if(delay_10ms == 0)
			delay_10ms = 1;
		if(delay_10ms > 20)
			delay_10ms = 20;
		for(u8 i = 0; i < count; i++) {
			led_write_mask(mask, mask, 0);
			sleep_us((u32)delay_10ms * 10000);
			led_write_mask(mask, 0, 0);
			sleep_us((u32)delay_10ms * 10000);
		}
		break;
	}
	default:
		status = CMD_STATUS_ARGS;
		break;
	}
	send_led_state(status, op, mask);
}

static void send_uart_state(u8 status, u8 op, u8 index) {
	u8 data[6];
	u32 baud = get_baud_rate_value(index);
	data[0] = op;
	data[1] = index;
	data[2] = get_baud_rate_count();
	data[3] = baud;
	data[4] = baud >> 8;
	data[5] = baud >> 16;
	send_resp(CMD_ID_UART, status, data, sizeof(data));
}

static void handle_uart_command(u8 *buf, int cmd_len) {
	u8 op = (cmd_len > 1) ? buf[1] : 0;
	u8 status = CMD_STATUS_OK;
	u8 data[6];
	u8 index;
	switch(op) {
	case 0:
		send_uart_state(CMD_STATUS_OK, op, read_baud_rate());
		return;
	case 1:
		data[0] = op;
		data[1] = (cmd_len > 2) ? buf[2] : 0;
		data[2] = (cmd_len > 3) ? buf[3] : 0;
		data[3] = (cmd_len > 4) ? buf[4] : 0x5a;
		data[4] = ~data[3];
		data[5] = read_baud_rate();
		send_resp(CMD_ID_UART, CMD_STATUS_OK, data, sizeof(data));
		return;
	case 2:
		if(cmd_len < 3) {
			status = CMD_STATUS_ARGS;
			index = read_baud_rate();
		} else {
			index = buf[2];
			if(index >= get_baud_rate_count()) {
				status = CMD_STATUS_VALUE;
				index = read_baud_rate();
			} else {
				pending_baud_index = index;
				pending_baud_waiting = 0;
			}
		}
		send_uart_state(status, op, index);
		return;
	default:
		send_uart_state(CMD_STATUS_ARGS, op, read_baud_rate());
		return;
	}
}

static int valid_rf_power(u8 power) {
	static const u8 valid[] = {
		RF_POWER_P10p46dBm, RF_POWER_P10p01dBm, RF_POWER_P7p79dBm,
		RF_POWER_P5p13dBm, RF_POWER_P3p01dBm, RF_POWER_P0p04dBm,
		RF_POWER_N5p03dBm, RF_POWER_N15p88dBm, RF_POWER_N30dBm
	};
	for(u8 i = 0; i < sizeof(valid); i++) {
		if(valid[i] == power)
			return 1;
	}
	return 0;
}

static void handle_rfsdk_command(u8 *buf, int cmd_len) {
	u8 op = (cmd_len > 1) ? buf[1] : 0;
	u8 status = CMD_STATUS_OK;
	u16 coded_min;
	u8 data[6];
	if(!rfsdk_state_init) {
		rfsdk_power = MY_RF_POWER;
		rfsdk_cap = 0xff;
		rfsdk_channels[0] = 37;
		rfsdk_channels[1] = 38;
		rfsdk_channels[2] = 39;
		rfsdk_coded_min_10ms = 10;
		rfsdk_state_init = 1;
	}
	if(op == 1) {
		if(cmd_len < 3)
			status = CMD_STATUS_ARGS;
		else if(!valid_rf_power(buf[2]))
			status = CMD_STATUS_VALUE;
		else {
			rfsdk_power = buf[2];
			set_runtime_rf_power(buf[2]);
		}
	} else if(op == 2) {
		if(cmd_len < 3)
			status = CMD_STATUS_ARGS;
		else if(buf[2] > 63)
			status = CMD_STATUS_VALUE;
		else {
			rfsdk_cap = buf[2];
			set_runtime_rf_cap(buf[2]);
		}
	} else if(op == 3) {
		if(cmd_len < 5)
			status = CMD_STATUS_ARGS;
		else if(buf[2] > 39 || buf[3] > 39 || buf[4] > 39)
			status = CMD_STATUS_VALUE;
		else {
			rfsdk_channels[0] = buf[2];
			rfsdk_channels[1] = buf[3];
			rfsdk_channels[2] = buf[4];
			set_primary_scan_channels(buf[2], buf[3], buf[4]);
		}
	} else if(op == 4) {
		if(cmd_len < 3) {
			status = CMD_STATUS_ARGS;
		} else {
			coded_min = buf[2];
			if(coded_min < 1)
				coded_min = 1;
			if(coded_min > 200)
				coded_min = 200;
			rfsdk_coded_min_10ms = coded_min;
			set_coded_min_scan_window(coded_min * SCAN_INTERVAL_10MS);
		}
	} else if(op != 0) {
		status = CMD_STATUS_ARGS;
	}
	data[0] = rfsdk_power;
	data[1] = rfsdk_cap;
	data[2] = rfsdk_channels[0];
	data[3] = rfsdk_channels[1];
	data[4] = rfsdk_channels[2];
	data[5] = rfsdk_coded_min_10ms;
	send_resp(CMD_ID_RFSDK, status, data, sizeof(data));
}

static void handle_version_command(void) {
	u8 data[6];
	data[0] = HW_VERSION;
	data[1] = CERTIFICATION_MARK;
	data[2] = SOFT_STRUCTURE;
	data[3] = MAJOR_VERSION;
	data[4] = MINOR_VERSION;
	data[5] = PATCH_NUM;
	send_resp(CMD_ID_VERSION, SW_VERSION, data, sizeof(data));
}

static void send_txadv_state(u8 status) {
	u8 data[6];
	data[0] = txadv_running_phy;
	data[1] = txadv_interval_units;
	data[2] = txadv_interval_units >> 8;
	data[3] = txadv_adv_data_len;
	data[4] = 0;
	data[5] = 0;
	send_resp(CMD_ID_TXADV, status, data, sizeof(data));
}

static void handle_txadv_command(u8 *buf, int cmd_len) {
	u8 op = (cmd_len > 1) ? buf[1] : 0xff;
	u8 status = CMD_STATUS_OK;

	if(op == 0) {
		// Stop
		if(txadv_running_phy) {
			txadv_stop();
			txadv_running_phy = 0;
		}
	} else if(op == 1) {
		// Start: buf[2]=phy buf[3..4]=interval_units buf[5]=adv_len buf[6..]=adv_data
		if(cmd_len < 6) {
			status = CMD_STATUS_ARGS;
		} else {
			u8 phy = buf[2];
			u16 interval = (u16)buf[3] | ((u16)buf[4] << 8);
			u8 adv_len = buf[5];
			if(phy > 2) {
				status = CMD_STATUS_VALUE;
			} else if(adv_len > 31 || cmd_len < 6 + adv_len) {
				status = CMD_STATUS_ARGS;
			} else {
				if(interval < 32)
					interval = 32; // min 20 ms
				if(txadv_running_phy)
					txadv_stop();
				ble_sts_t s = txadv_start(phy, interval, &buf[6], adv_len);
				if(s != BLE_SUCCESS) {
					status = CMD_STATUS_DENIED;
					txadv_running_phy = 0;
				} else {
					txadv_running_phy = phy + 1;
					txadv_interval_units = interval;
					txadv_adv_data_len = adv_len;
				}
			}
		}
	} else {
		status = CMD_STATUS_ARGS;
	}
	send_txadv_state(status);
}

// CMD_ID_CONN: central role connection management
// buf[1]=op  0=status 1=connect(1M) 2=connect(Coded) 3=disconnect  4=cancel
// buf[2]=peer_addr_type, buf[3..8]=peer_addr (for op 1,2)
// Response: [state, status, handle_lo, handle_hi, interval_lo, interval_hi]
static void handle_conn_command(u8 *buf, int cmd_len) {
	u8 op = (cmd_len > 1) ? buf[1] : 0;
	u8 data[6] = {0};
	u8 resp_status = CMD_STATUS_OK;

	if(op == 0) {
		// Status query
		u8 peer[6]; conn_peer_addr_get(peer);
		data[0] = conn_state_get();
		data[1] = 0;
		data[2] = conn_handle_get() & 0xff;
		data[3] = conn_handle_get() >> 8;
		data[4] = conn_interval_get() & 0xff;
		data[5] = conn_interval_get() >> 8;
	} else if(op == 1 || op == 2) {
		// Connect: buf[2]=addr_type, buf[3..8]=peer_addr
		if(cmd_len < 9) {
			resp_status = CMD_STATUS_ARGS;
		} else {
			u8 init_phy = (op == 2) ? 1 : 0; // 0=1M 1=Coded
			ble_sts_t s = conn_start(buf[2], &buf[3], init_phy);
			resp_status = (s == BLE_SUCCESS) ? CMD_STATUS_OK : CMD_STATUS_DENIED;
			data[0] = conn_state_get();
		}
	} else if(op == 3 || op == 4) {
		// Disconnect / cancel
		ble_sts_t s = conn_stop();
		resp_status = (s == BLE_SUCCESS) ? CMD_STATUS_OK : CMD_STATUS_DENIED;
		data[0] = conn_state_get();
	} else {
		resp_status = CMD_STATUS_ARGS;
	}
	send_resp(CMD_ID_CONN, resp_status, data, sizeof(data));
}

// CMD_ID_TXDATA: disabled in this build (L2CAP/GATT not initialized to save RAM)
static void handle_txdata_command(u8 *buf, int cmd_len) {
	(void)buf; (void)cmd_len;
	u8 data[6] = {0};
	send_resp(CMD_ID_TXDATA, CMD_STATUS_DENIED, data, sizeof(data));
}

// Called from ble.c when ACL connection state changes (connect / disconnect event)
// Pushes an async CMD_ID_CONN notification to the host.
void scanning_conn_event_cb(u8 state, u16 handle, u16 interval_125us, u8 peer_addr_type, u8 *peer_addr) {
	u8 data[6] = {0};
	data[0] = state;
	data[1] = peer_addr_type;
	if(peer_addr) {
		memcpy(&data[2], peer_addr, 4); // first 4 bytes of peer MAC
	}
	// Use handle as the id field and encode interval in data[4..5]
	send_resp(CMD_ID_CONN, state, data, sizeof(data));
	(void)handle; (void)interval_125us;
}

_attribute_ram_code_
int chk_mac(u8 *pmac) {
	int ret = 0;
	if(mac_list.count) {
		for(int i = 0; i < mac_list.count; i++) {
			u8 n = mac_list.mac_len[i];
			if(n == 0 || n > 6) n = 6;
			// BLE MACs are stored little-endian; compare from the MSB (OUI) end
			if(memcmp(&mac_list.mac[i], pmac + (6 - n), n) == 0) {
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
	if(pending_baud_waiting && uart_is_tx_done() && clock_time_exceed(pending_baud_tick, 200000)) {
		set_baud_rate_index(pending_baud_index);
		pending_baud_index = 0xff;
		pending_baud_waiting = 0;
	}
	u8 *p = my_fifo_get(&ad_fifo);
	if(p) {
		int len = p[0] + HEAD_CRC_ADD_LEN - 2;
		crc = crcFast(p, len);
		p[len] = crc;
		p[len+1] = crc >> 8;
		if(uart_send(p, len + 2) > 0) {
			my_fifo_pop(&ad_fifo);
			if(pending_baud_index != 0xff && !pending_baud_waiting) {
				pending_baud_waiting = 1;
				pending_baud_tick = clock_time();
			}
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
		if(!(manual_led_mask & LED_MASK_RED) && tt - leds.tr > TIMER_LED_R) {
			leds.tr = tt;
			gpio_write(GPIO_LED_R, 0);
		}
#endif
#ifdef 	TIMER_LED_G
		if(!(manual_led_mask & LED_MASK_GREEN) && tt - leds.tg > TIMER_LED_G) {
			leds.tg = tt;
			gpio_write(GPIO_LED_G, 0);
		}
#endif
#ifdef 	TIMER_LED_B
		if(!(manual_led_mask & LED_MASK_BLUE) && tt - leds.tb > TIMER_LED_B) {
			leds.tb = tt;
			gpio_write(GPIO_LED_B, 0);
		}
#endif
#ifdef 	TIMER_LED_W
		if(!(manual_led_mask & LED_MASK_WHITE) && tt - leds.tw > TIMER_LED_W) {
			leds.tw = tt;
			gpio_write(GPIO_LED_W, 0);
		}
#endif
#ifdef 	TIMER_LED_E
		if(!(manual_led_mask & LED_MASK_YELLOW) && tt - leds.te > TIMER_LED_E) {
			leds.te = tt;
			gpio_write(GPIO_LED_E, 0);
		}
#endif
	}
	uint8_t buf[64];  // 64 bytes: enough for max CMD_ID_TXADV (6 hdr + 31 data + 2 CRC = 39)
	// CMD_ID_TXDATA worst case: 4 hdr + 20 data + 2 CRC = 26 bytes (fits in 64)
	int len = uart_read(buf, sizeof(buf));
	if(len > 2 && crcFast(buf, len) == 0) {
#ifdef 	TIMER_LED_W
			gpio_write(GPIO_LED_W, 1);
			leds.tw = tt;
#endif
		int cmd = buf[0];
		int cmd_len = len - 2;
		if(cmd == CMD_ID_SCAN && len == 3 + 3) {
			start_adv_scanning(buf[1], buf[2] | (buf[3]<<8));
			send_resp(cmd, mac_list.count, &buf[1], 3);
		} else if(cmd == CMD_ID_GPIO) {
			handle_gpio_command(buf, cmd_len);
		} else if(cmd == CMD_ID_LED) {
			handle_led_command(buf, cmd_len);
		} else if(cmd == CMD_ID_UART) {
			handle_uart_command(buf, cmd_len);
		} else if(cmd == CMD_ID_RFSDK) {
			handle_rfsdk_command(buf, cmd_len);
		} else if(cmd == CMD_ID_VERSION) {
			handle_version_command();
		} else if(cmd == CMD_ID_TXADV) {
			handle_txadv_command(buf, cmd_len);
		} else if(cmd == CMD_ID_CONN) {
			handle_conn_command(buf, cmd_len);
		} else if(cmd == CMD_ID_TXDATA) {
			handle_txdata_command(buf, cmd_len);
		} else if((cmd == CMD_ID_WMAC || cmd == CMD_ID_BMAC) && len >= 1 + 3 && len <= 6 + 3) {
			u8 mac_data_len = (u8)(len - 3);
			if(cmd == CMD_ID_WMAC)
				mac_list.mode = WHITE_LIST;
			else
				mac_list.mode = BALCK_LIST;
			if(mac_list.count < MAC_MAX_SCAN_LIST) {
				memset(&mac_list.mac[mac_list.count], 0, 6);
				memcpy(&mac_list.mac[mac_list.count], &buf[1], mac_data_len);
				mac_list.mac_len[mac_list.count] = mac_data_len;
				mac_list.count++;
			}
			send_resp(cmd, mac_list.count, &buf[1], mac_data_len);
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
	if(!gpio_read(KEY_USER)) {
		if(!key_baud_low_pending) {
			key_baud_low_pending = 1;
			key_baud_low_tick = clock_time();
		} else if(clock_time_exceed(key_baud_low_tick, KEY_BAUD_HOLD_US)) {
			key_baud_low_pending = 0;
			change_baud_rate();
		}
	} else {
		key_baud_low_pending = 0;
	}
}
