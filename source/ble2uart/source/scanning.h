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
	CMD_ID_RXDATA	= 0x0e, // (async) ATT data received from connected peer
	CMD_ID_VBAT		= 0x0f, // read current VBAT / 3V3 rail voltage
	CMD_ID_GPIOEVT	= 0x10  // GPIO edge events: arm/disarm + spontaneous notifications
} CMD_ID_KEYS;

/* data[0] of a CMD_STATUS_ARGS response that tells the host "I received a frame
   whose CRC did not match" (data[1] = received length).  It lets the host tell a
   lost/merged frame apart from a firmware that stopped answering. */
#define CMD_OP_BAD_FRAME	0xfe

/* TB-03F-KIT LED bitmap, shared by the firmware status indicators and the host
   manual control (CMD_ID_GPIO op 2/3/5/6/8, CMD_ID_LED).  The bit order is the
   one reported in data[3] of the CMD_ID_GPIO response (see the README). */
enum {
	LED_MASK_BLUE	= BIT(0),	// PC2 / PWM0
	LED_MASK_RED	= BIT(1),	// PC3 / PWM1
	LED_MASK_GREEN	= BIT(2),	// PC4 / PWM2
	LED_MASK_YELLOW	= BIT(3),	// PB4 / PWM4
	LED_MASK_WHITE	= BIT(4),	// PB5 / PWM5
	LED_MASK_ALL	= LED_MASK_BLUE | LED_MASK_RED | LED_MASK_GREEN | LED_MASK_YELLOW | LED_MASK_WHITE
};

/* The five LEDs are shared between the status indicators and the host.  As soon
   as the host takes manual control of a LED (CMD_ID_GPIO write/toggle/PWM), the
   firmware must stop driving that pin: otherwise the two fight over it, the
   host state gets overwritten by the next status blink and a status indicator
   (e.g. the white "command received" LED) stays latched on.
   Returns 1 when the firmware may drive the given LEDs. */
int led_status_allowed(u8 led_mask);

/* ------------------------- black box / stall report -------------------------
 * A few words between the application RAM and the stack (outside .bss, so they
 * survive a reset) that the main loop keeps updated.  When the firmware wedges,
 * the watchdog resets the chip and the last values are sent to the host as
 * CMD_ID_PRNT frames, which tells exactly where it stopped. */
void bb_init(void);
void bb_alive(void);
void bb_checkpoint(u32 code);
void bb_report(void);
void bb_report_task(void);
void bb_isr_enter(void);
void bb_isr_exit(void);
void sched_timer_guard(void);

/* Phase markers written into the black box, so a post-mortem report says which
   part of the main loop was running (the ISR phase has its own word). */
#define BB_PHASE_SDK	0x00020000u	/* entering blc_sdk_main_loop()  */
#define BB_PHASE_SCAN	0x00030000u	/* entering scan_task()          */

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
	u8	mac_len[MAC_MAX_SCAN_LIST]; // significant bytes per entry: 1..6 (0 treated as 6)
} mac_list_t;

extern mac_list_t mac_list;

#define HEAD_CRC_ADD_LEN	13

void scan_task(void);

// CMD_ID_GPIOEVT: called from the application IRQ handler for every GPIO interrupt.
void scanning_gpio_irq_handler(void);

#if GATT_ENABLE
// Called from ble.c for every incoming ATT packet of the master role.
void scanning_gatt_callback(u16 conn_handle, u8 opcode, u16 att_handle, u16 data_len, u8 *data);
#endif

void ble_ext_adv_callback(u8 *p);
void ble_adv_callback(u8 *p);
void ble_le_periodic_adv_callback(u8 *p);
void ble_le_periodic_adv_sync_established_callback(u8 *p);
void ble_le_periodic_adv_sync_lost_callback(u8 *p);

#endif /* SCANNING_H_ */
