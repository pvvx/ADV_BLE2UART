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
/* RAM lever: each FIFO block costs MYFIFO_BLK_SIZE bytes.  Command responses are
   queued here as well, so 2 is the safe minimum; 4 is the upstream default.
   Override from the makefile with EXTRA_FLAGS=-DADV_FIFO_BLOCKS=2 */
#ifndef ADV_FIFO_BLOCKS
#define ADV_FIFO_BLOCKS		4
#endif
MYFIFO_INIT(ad_fifo, MYFIFO_BLK_SIZE, ADV_FIFO_BLOCKS); // (229+12)*4 = 964 bytes + sizeof(my_fifo_t)

#define KEY_BAUD_HOLD_US	800000

RAM mac_list_t mac_list;

hci_le_periodicAdvSyncEstablishedEvt_t periodic_adv;

/* Returns 1 when the response was queued in the UART FIFO, 0 when the FIFO is
   full (callers that must not lose a frame, e.g. the GPIO event drain, retry
   on the next main-loop iteration). */
int send_resp(u8 cmd, u8 id, u8 *pmac, u8 len) {
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
		return 1;
	}
	return 0;
}

enum {
	CMD_STATUS_OK = 0,
	CMD_STATUS_ARGS = 1,
	CMD_STATUS_PIN = 2,
	CMD_STATUS_DENIED = 3,
	CMD_STATUS_VALUE = 4
};

/* -------------------------------------------------------------------------
 * Black box / stall report
 *
 * The words live in a .bss array marked "no init": the linker allocates it and
 * cstartup_825x.S does NOT zero it at boot, so the values survive a warm reset
 * (watchdog, RTS from the serial port) exactly like a fixed address would - but
 * without the danger that made the previous version wrong:
 *
 *   the words used to sit at the fixed address 0x84F800, which is inside the
 *   region the main stack can reach (the stack grows down from 0x850000 into
 *   free SRAM and nothing guarantees it stays above 0x84F800).  Writing there
 *   from the main loop and from the interrupt handler could therefore trample
 *   the saved return addresses of deep library calls, which matches the
 *   measured signature of the freeze: a single interrupt (Bn=00000001) and the
 *   CPU stuck inside blc_sdk_irq_handler() (Bi=00040000).
 *
 * The main loop keeps a coarse trace there (loop counter, irq mask, the
 * scheduler's System-Timer request word and the tick) and the command dispatcher
 * stores a checkpoint before each step.  When the firmware wedges the watchdog
 * resets it; at the next boot the values are sent to the host as CMD_ID_PRNT
 * frames.
 * ------------------------------------------------------------------------- */
#define BB_MAGIC	0x42420001u
#define BB_MAGIC_W	0
#define BB_BOOTS	1
#define BB_CHECKPOINT	2
#define BB_LOOPS	3
#define BB_IRQMASK	4
#define BB_SCH14	5
#define BB_TICK		6
#define BB_REARMS	7
#define BB_ISR		8
#define BB_ISRCNT	9
#define BB_QIN		10
#define BB_QOUT		11
#define BB_WORDS	12
#define BB_ISR_IN	0x00040000u

_attribute_data_no_init_ u32 bb_words[BB_WORDS];
#define bb_w(n)		bb_words[n]

extern u8 g_scheMng[];

void bb_init(void) {
	if(bb_w(BB_MAGIC_W) != BB_MAGIC) {
		for(u32 i = 0; i < BB_WORDS; i++)
			bb_words[i] = 0;
		bb_w(BB_MAGIC_W) = BB_MAGIC;
	} else {
		bb_w(BB_BOOTS)++;
	}
	bb_w(BB_CHECKPOINT) = 0xffffffffu;	/* running normally (main loop reached) */
}

void bb_checkpoint(u32 code) {
	if(bb_w(BB_MAGIC_W) == BB_MAGIC)
		bb_w(BB_CHECKPOINT) = code;
}

static u32 bb_div;
void bb_alive(void) {
	if(bb_w(BB_MAGIC_W) != BB_MAGIC)
		return;
	/* The main loop is running: no command is being processed right now.  If a
	   command handler wedges the CPU the marker it stored stays here instead. */
	bb_w(BB_CHECKPOINT) = 0xffffffffu;
	if((++bb_div & 0xffu) == 0) {
		bb_w(BB_LOOPS)++;
		bb_w(BB_IRQMASK) = reg_irq_mask;
		bb_w(BB_SCH14) = *(volatile u32 *)((u8 *)g_scheMng + 0x14);
		bb_w(BB_TICK) = reg_system_tick;
	}
}

/* -------------------------------------------------------------------------
 * System Timer guard
 *
 * The V4.0.2.5 library masks the System Timer interrupt off, without re-arming
 * the tick, as soon as its "who needs the System Timer" bitmap (g_scheMng+0x14)
 * is zero.  The scheduler then stops and blc_sdk_main_loop() never returns: the
 * firmware looks dead (white + yellow LED latched on).  Measured with the black
 * box: death inside the SDK main loop with BIT(20) = FLD_IRQ_SYSTEM_TIMER missing
 * from reg_irq_mask.
 *
 * Two levels of protection, both cheap enough for every main-loop pass:
 *  1. keep the application requirement bit set (bit 19, ignored by the scheduler
 *     mask 0xff87ffff) so the library's "== 0" test never matches;
 *  2. if the interrupt was masked off anyway, re-enable it properly: clear the
 *     pending flag and push the next tick into the future FIRST (re-enabling the
 *     mask without re-arming gives an immediate interrupt storm).
 *
 * DEFAULT OFF since fw 0.14 (SCHED_TIMER_GUARD in app_config.h): step 1 forces
 * bit 19 of g_scheMng+0x14, the bit the library uses for a pending auxiliary
 * scan task, and every frozen run had that bit forced on while the ext_adv_test
 * build (same module list, bit left alone) runs normally.
 * ------------------------------------------------------------------------- */
#ifndef SCHED_TIMER_GUARD
#define SCHED_TIMER_GUARD 0
#endif

#if SCHED_TIMER_GUARD
void sched_timer_guard(void) {
	volatile u32 *req = (volatile u32 *)((u8 *)g_scheMng + SCHED_TIMER_REQ_OFFSET);

	if((*req & SCHED_TIMER_KEEPALIVE_BIT) == 0)
		*req |= SCHED_TIMER_KEEPALIVE_BIT;

	/* The library parks the System Timer tick far in the future (measured:
	   22 s ahead) when the scheduler bitmap is empty, so the System Timer
	   interrupt never fires and the LL never processes the enable command.
	   Re-arm it to ~1 ms ahead whenever it is not in the near future. */
	u32 now = reg_system_tick;
	u32 next = now + (CLOCK_SYS_CLOCK_HZ / 1000u); /* ~1 ms ahead */
	if((s32)(reg_system_tick_irq - now) > (s32)(CLOCK_SYS_CLOCK_HZ / 1000u) ||
	   (s32)(reg_system_tick_irq - now) < 0) {
		reg_system_tick_irq = next & 0xfffffff8u;   /* 8-tick aligned */
		reg_irq_src = FLD_IRQ_SYSTEM_TIMER;         /* clear pending */
		irq_enable_type(FLD_IRQ_SYSTEM_TIMER);
		if(bb_w(BB_MAGIC_W) == BB_MAGIC && bb_w(BB_REARMS) < 0xffu)
			bb_w(BB_REARMS)++;
	}
}
#else
void sched_timer_guard(void) { }   /* switch off: see the note above */
#endif

/* The report is 86 characters (7 x 11 + Bb + Br) - the buffer MUST stay larger
   than that: overflowing it corrupts .bss at boot and the firmware then never
   reaches the main loop (measured with a 64-byte buffer and the Bn/Bq/Bp
   fields: Bc=00050010 in the next life's report).  Every writer is bounds
   checked. */
static char bb_txt[128];
static u8 bb_txt_len;
static u8 bb_txt_pos;

static void bb_put(const char *s) {
	while(*s && bb_txt_len < sizeof(bb_txt))
		bb_txt[bb_txt_len++] = *s++;
}

static void bb_put_hex8(u32 v) {
	static const char hx[] = "0123456789abcdef";
	u32 i;
	for(i = 0; i < 8 && bb_txt_len < sizeof(bb_txt); i++)
		bb_txt[bb_txt_len++] = hx[(v >> (28 - 4 * i)) & 0xfu];
}

static void bb_put_hex2(u32 v) {
	static const char hx[] = "0123456789abcdef";
	if(bb_txt_len < sizeof(bb_txt) - 2) {
		bb_txt[bb_txt_len++] = hx[(v >> 4) & 0xfu];
		bb_txt[bb_txt_len++] = hx[v & 0xfu];
	}
}

static void bb_put_hex1(u32 v) {
	static const char hx[] = "0123456789abcdef";
	if(bb_txt_len < sizeof(bb_txt) - 1)
		bb_txt[bb_txt_len++] = hx[v & 0xfu];
}

/* ISR phase: BB_ISR_IN while the RF interrupt handler runs, 0 after it returns.
   A report showing BB_ISR_IN therefore means the CPU died INSIDE the interrupt
   handler (the extended advertising teardown runs there), not in the main loop. */
void bb_isr_enter(void) {
	if(bb_w(BB_MAGIC_W) == BB_MAGIC) {
		bb_w(BB_ISR) = BB_ISR_IN;
		bb_w(BB_ISRCNT)++;
		bb_w(BB_QIN) = reg_irq_src;   /* pending sources, before the SDK handles them */
	}
}

void bb_isr_exit(void) {
	if(bb_w(BB_MAGIC_W) == BB_MAGIC) {
		bb_w(BB_ISR) = 0;
		bb_w(BB_QOUT) = reg_irq_src;  /* what is still pending after a normal ISR */
	}
}

void bb_report(void) {
	bb_txt_len = 0;
	bb_txt_pos = 0;
	bb_put("Bc="); bb_put_hex8(bb_w(BB_CHECKPOINT));
	bb_put("Bm="); bb_put_hex8(bb_w(BB_IRQMASK));
	bb_put("Bs="); bb_put_hex8(bb_w(BB_SCH14));
	bb_put("Bi="); bb_put_hex8(bb_w(BB_ISR));
	bb_put("Bn="); bb_put_hex8(bb_w(BB_ISRCNT));
	bb_put("Bq="); bb_put_hex8(bb_w(BB_QIN));
	bb_put("Bp="); bb_put_hex8(bb_w(BB_QOUT));
	bb_put("Bb="); bb_put_hex1(bb_w(BB_BOOTS));
	bb_put("Br="); bb_put_hex2(bb_w(BB_REARMS));
}

void bb_report_task(void) {
	u8 chunk;
	if(bb_txt_pos >= bb_txt_len)
		return;
	chunk = (u8)(bb_txt_len - bb_txt_pos);
	if(chunk > 6)
		chunk = 6;
	/* Keep the chunk queued until it really went out: send_resp() returns 0 when
	   the 4-block response FIFO is full, and the main loop is fast enough to push
	   the whole report (9 frames) in a few microseconds, so without this retry the
	   report is truncated and the host loses frame synchronisation. */
	if(send_resp(CMD_ID_PRNT, 0, (u8 *)(bb_txt + bb_txt_pos), chunk))
		bb_txt_pos = (u8)(bb_txt_pos + chunk);
}

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

/* The LED_MASK_* bitmap now lives in scanning.h so that the status indicators
   and led_status_allowed() share one definition. */

/* Generic conversion of a protocol pin id (high nibble = port A=0..D=3, low
   nibble = bit) into the SDK GPIO_PinTypeDef enumeration. */
static GPIO_PinTypeDef pin_id_to_gpio(u8 pin_id) {
	return (GPIO_PinTypeDef)(((u32)(pin_id >> 4) << 8) | BIT(pin_id & 0x0f));
}

/* GPIOs of the TB-03F-KIT 30-pin header that are neither a LED, nor the UART
   link, nor the user key (header pin number in brackets):
     PB7 (6) PB6 (7) PD7 (5) PA1 (23) PD2 (26) PD3 (27) PD4 (28) PC1 (29) PC0 (30)
   PB7 is the ADC pad of the internal VBAT monitor (GPIO_VBAT_DETECT): it can be
   read and configured, but it is driven by the SDK battery check. */
static int board_header_pin(u8 pin_id) {
	static const u8 ids[] = {
		0x17, /* PB7 */
		0x16, /* PB6 */
		0x37, /* PD7 */
		0x01, /* PA1 */
		0x32, /* PD2 */
		0x33, /* PD3 */
		0x34, /* PD4 */
		0x21, /* PC1 */
		0x20, /* PC0 */
	};
	for(u8 i = 0; i < sizeof(ids); i++) {
		if(ids[i] == pin_id)
			return 1;
	}
	return 0;
}

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
		if(board_header_pin(pin_id)) {
			*pin = pin_id_to_gpio(pin_id);
			return 1;
		}
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

/* PWM channel of a LED pin: the five TB-03F-KIT LEDs are all driven by hardware
   PWM channels (PC2/PWM0 blue, PC3/PWM1 red, PC4/PWM2 green, PB4/PWM4 yellow,
   PB5/PWM5 white). */
static int led_pin_pwm_id(u8 pin_id, u8 *pwm_id) {
	switch(pin_id) {
	case BOARD_PIN_LED_BLUE_PC2:
		*pwm_id = PWM_LED_B;
		return 1;
	case BOARD_PIN_LED_RED_PC3:
		*pwm_id = PWM_LED_R;
		return 1;
	case BOARD_PIN_LED_GREEN_PC4:
		*pwm_id = PWM_LED_G;
		return 1;
	case BOARD_PIN_LED_YELLOW_PB4:
		*pwm_id = PWM_LED_E;
		return 1;
	case BOARD_PIN_LED_WHITE_PB5:
		*pwm_id = PWM_LED_W;
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

/* Status indicators must never override a LED the host drives by hand. */
int led_status_allowed(u8 led_mask) {
	return (manual_led_mask & led_mask) ? 0 : 1;
}

/* Analog (ADC) inputs available on the TB-03F-KIT.
   The TLSR825x ADC channels are PB0..PB7, PC4 and PC5, but on this board:
     - PB0, PB2, PB3 and PC5 are not on the 30-pin header;
     - PB1 is the UART TX, PB4/PB5 are the side LEDs and PC4 is the RGB green LED;
     - PB7 is the ADC pad of the internal VBAT monitor (CMD_ID_VBAT).
   PB6 (header pin 7) is therefore the only free analog input. */
static int adc_capable_pin(u8 pin_id) {
	return pin_id == 0x16;   /* PB6 */
}

/* Single-shot ADC read of an analog-capable GPIO, returned in millivolts. */
static u16 adc_read_pin_mv(u8 pin_id) {
	GPIO_PinTypeDef pin = pin_id_to_gpio(pin_id);
	u16 mv;
	adc_init();
	adc_base_init(pin);
	/* Note: power on after every other setting has been applied. */
	adc_power_on_sar_adc(1);
	mv = (u16)adc_sample_and_get_result();
	adc_power_on_sar_adc(0);
	/* Give the pin back to the digital GPIO block. */
	gpio_set_func(pin, AS_GPIO);
	gpio_set_output_en(pin, 0);
	gpio_set_input_en(pin, 1);
	return mv;
}

/* Supply (VBAT) measurement: PB7 is driven high so that the ADC reads the
   3.3 V rail, as required by the B85 ADC (see SDK battery_check.c). */
static u16 adc_read_vbat_mv(void) {
	GPIO_PinTypeDef pin = GPIO_VBAT_DETECT;
	u16 mv;
	adc_init();
	adc_vbat_init(pin);
	adc_power_on_sar_adc(1);
	mv = (u16)adc_sample_and_get_result();
	adc_power_on_sar_adc(0);
	gpio_set_func(pin, AS_GPIO);
	gpio_set_output_en(pin, 0);
	gpio_set_input_en(pin, 1);
	return mv;
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
	case 5: {
		/* PWM brightness of a LED pin:
		   [0x06, 5, pin, duty(0..100), period_lo, period_hi]  (period in us) */
		u8 ch;
		u16 period, cmp;
		if(cmd_len < 6 || !board_pin(pin_id, &pin)) {
			status = CMD_STATUS_PIN;
			break;
		}
		if(buf[3] > 100) {
			status = CMD_STATUS_VALUE;
			break;
		}
		if(!led_pin_pwm_id(pin_id, &ch)) {
			status = CMD_STATUS_DENIED;
			break;
		}
		period = buf[4] | ((u16)buf[5] << 8);
		if(period < 10)
			period = 10;
		cmp = (u16)(((u32)period * buf[3]) / 100u);
		pwm_set_mode((pwm_id)ch, PWM_NORMAL_MODE);
		pwm_set_clk(CLOCK_SYS_CLOCK_HZ, 1000000);
		pwm_set_cycle_and_duty((pwm_id)ch, period, cmp);
		gpio_set_func(pin, (GPIO_FuncTypeDef)(AS_PWM0 + ch));
		pwm_start((pwm_id)ch);
		/* hardware owns the pin now: keep the status blink off it */
		manual_led_mask |= board_pin_led_mask(pin_id);
		break;
	}
	case 6: {
		/* PWM off, the pin goes back to a plain GPIO output: [0x06, 6, pin] */
		u8 ch;
		if(cmd_len < 3 || !board_pin(pin_id, &pin)) {
			status = CMD_STATUS_PIN;
			break;
		}
		if(!led_pin_pwm_id(pin_id, &ch)) {
			status = CMD_STATUS_DENIED;
			break;
		}
		pwm_stop((pwm_id)ch);
		gpio_set_func(pin, AS_GPIO);
		gpio_set_output_en(pin, 1);
		gpio_set_input_en(pin, 1);
		gpio_write(pin, 0);
		manual_led_mask &= ~board_pin_led_mask(pin_id);
		break;
	}
	case 7: {
		/* Analog (ADC) read: 12-bit code in data[2..3], 3.3 V full scale,
		   as in the ESP32 sibling firmware.  Only the ADC-capable pins are
		   accepted (PB0..PB6, PC4, PC5). */
		u8 adata[6];
		u16 mask = board_read_mask();
		u16 raw;
		if(cmd_len < 3 || !adc_capable_pin(pin_id)) {
			status = CMD_STATUS_PIN;
			break;
		}
		raw = (u16)(((u32)adc_read_pin_mv(pin_id) * 4095u) / 3300u);
		adata[0] = op;
		adata[1] = pin_id;
		adata[2] = raw;
		adata[3] = raw >> 8;
		adata[4] = mask;
		adata[5] = mask >> 8;
		send_resp(CMD_ID_GPIO, CMD_STATUS_OK, adata, sizeof(adata));
		return;
	}
	case 8: {
		/* Set RGB colour: the TB-03F-KIT carries three discrete RGB LEDs
		   (R = PC3, G = PC4, B = PC2), so a non-zero channel switches the
		   corresponding LED on.  Payload: [0x06, 8, pin, R, G, B]. */
		u8 rgb = 0;
		if(cmd_len < 6) {
			status = CMD_STATUS_ARGS;
			break;
		}
		if(buf[3])
			rgb |= LED_MASK_RED;
		if(buf[4])
			rgb |= LED_MASK_GREEN;
		if(buf[5])
			rgb |= LED_MASK_BLUE;
		led_write_mask(LED_MASK_RED | LED_MASK_GREEN | LED_MASK_BLUE, rgb, 1);
		break;
	}
	case 13: {
		/* --- diagnostic operations, see "GPIO diagnostics" in the README -------
		   op 13 "reply only": exercises send_resp() and the UART FIFO without
		   touching any GPIO.  If the firmware answers this but stops answering
		   op 2, the fault is in the GPIO write; if it does not answer even this,
		   the fault is in the response path. */
		u8 rdata[6];
		rdata[0] = op;
		rdata[1] = pin_id;
		rdata[2] = 0xa5;
		rdata[3] = 0x5a;
		rdata[4] = 0xa5;
		rdata[5] = 0x5a;
		send_resp(CMD_ID_GPIO, CMD_STATUS_OK, rdata, sizeof(rdata));
		return;
	}
	case 14:
		/* op 14 "dry run write": the whole op 2 path except gpio_write(). */
		if(cmd_len < 4 || !board_pin(pin_id, &pin))
			status = CMD_STATUS_PIN;
		else if(!board_pin_led_mask(pin_id))
			status = CMD_STATUS_DENIED;
		break;
	case 15:
		/* op 15 "dry run toggle": the whole op 3 path except gpio_toggle(). */
		if(cmd_len < 3 || !board_pin(pin_id, &pin))
			status = CMD_STATUS_PIN;
		else if(!board_pin_led_mask(pin_id))
			status = CMD_STATUS_DENIED;
		break;
	default:
		status = CMD_STATUS_ARGS;
		break;
	}
	send_gpio_state(status, op, pin_id);
}

/* ---------------------------------------------------------------------------
 * CMD_ID_GPIOEVT - hardware GPIO edge notifications
 *
 * The host arms a pin with op 1; from that moment every rising/falling edge on
 * the pin produces a spontaneous CMD_ID_GPIOEVT frame (see the command
 * reference in the README).  The TLSR825x GPIO interrupt is edge-polarity
 * based, so the ISR flips the polarity after every event to obtain the same
 * "any edge" behaviour as the ESP32 sibling firmware.
 *
 * Pin ids use the usual protocol encoding (high nibble = port A=0..D=3, low
 * nibble = bit).  Only ports A and B (pin id 0x00..0x17) can be armed: the
 * spontaneous frame carries the pin id in the low 5 bits of byte[2] and the
 * armed-pin bitmap is 32 bits wide, exactly as in the ESP32 firmware.
 * ------------------------------------------------------------------------- */
#define GPIOEVT_EVENT_FLAG   0x80u   // set in the id field of spontaneous events
#define GPIOEVT_QUEUE_LEN    16

typedef struct {
	u8  pin_id;
	u8  level;
	u32 timestamp_ms;
} gpio_event_msg_t;

static u32 gpio_evt_mask;                 // one bit per pin id (0x00..0x1F)
static u8  gpio_evt_head;
static u8  gpio_evt_tail;
static gpio_event_msg_t gpio_evt_queue[GPIOEVT_QUEUE_LEN];
static u32 gpio_evt_ms_base;              // clock_time() at the last ms update
static volatile u32 gpio_evt_ms;          // monotonic millisecond counter

static GPIO_PinTypeDef gpio_evt_pin(u8 pin_id) {
	return pin_id_to_gpio(pin_id);
}

/* Pins that may not be armed: ports C to E (pin id >= 0x20 does not fit the
   5-bit event field), the UART link, the board LEDs and PB7, which is driven by
   the internal VBAT monitor. */
static int gpio_evt_pin_armable(u8 pin_id) {
	if(pin_id > 0x17)
		return 0;
	if(pin_id == BOARD_PIN_UART_TX_PB1 || pin_id == BOARD_PIN_UART_RX_PA0)
		return 0;
	if(pin_id == 0x17)   /* PB7: internal VBAT monitor pad */
		return 0;
	if(board_pin_led_mask(pin_id))
		return 0;
	return 1;
}

/* Runs in interrupt context: keep it short, no printing, no blocking. */
static void gpio_evt_push(u8 pin_id, u8 level) {
	u8 next = (u8)((gpio_evt_head + 1) % GPIOEVT_QUEUE_LEN);
	if(next == gpio_evt_tail)
		return;                            // queue full: drop the event
	gpio_evt_queue[gpio_evt_head].pin_id = pin_id;
	gpio_evt_queue[gpio_evt_head].level = level;
	gpio_evt_queue[gpio_evt_head].timestamp_ms = gpio_evt_ms;
	gpio_evt_head = next;
}

/* Called from irq_handler() for every interrupt; must run from RAM. */
_attribute_ram_code_ void scanning_gpio_irq_handler(void) {
	if(!(reg_irq_src & FLD_IRQ_GPIO_EN))
		return;
	gpio_clr_irq_status(GPIO_IRQ_GPIO_STATUS);
	for(u8 pin_id = 0; pin_id < 32; pin_id++) {
		if(!(gpio_evt_mask & (1u << pin_id)))
			continue;
		GPIO_PinTypeDef pin = gpio_evt_pin(pin_id);
		u8 level = gpio_read(pin) ? 1 : 0;
		gpio_evt_push(pin_id, level);
		/* Re-arm for the opposite edge (any-edge behaviour). */
		gpio_set_interrupt_pol(pin, level ? POL_FALLING : POL_RISING);
	}
}

/* Monotonic millisecond counter, refreshed once per scan_task() iteration. */
static void gpio_evt_update_ms(void) {
	u32 t = clock_time();
	u32 d = t - gpio_evt_ms_base;
	if(d >= CLOCK_16M_SYS_TIMER_CLK_1MS) {
		u32 step = d / CLOCK_16M_SYS_TIMER_CLK_1MS;
		gpio_evt_ms += step;
		gpio_evt_ms_base += step * CLOCK_16M_SYS_TIMER_CLK_1MS;
	}
}

/* Pushes the queued edge events to the host; stops when the UART FIFO is full
   and resumes on the next iteration (no event is ever lost silently). */
static void gpio_evt_drain(void) {
	while(gpio_evt_tail != gpio_evt_head) {
		gpio_event_msg_t *m = &gpio_evt_queue[gpio_evt_tail];
		u8 data[6];
		data[0] = m->pin_id;
		data[1] = m->level;
		data[2] = m->timestamp_ms;
		data[3] = m->timestamp_ms >> 8;
		data[4] = m->timestamp_ms >> 16;
		data[5] = m->timestamp_ms >> 24;
		if(!send_resp(CMD_ID_GPIOEVT, GPIOEVT_EVENT_FLAG | (m->pin_id & 0x1f), data, sizeof(data)))
			return;
		gpio_evt_tail = (u8)((gpio_evt_tail + 1) % GPIOEVT_QUEUE_LEN);
	}
}

static void gpio_evt_disarm(u8 pin_id) {
	GPIO_PinTypeDef pin = gpio_evt_pin(pin_id);
	gpio_en_interrupt(pin, 0);
	gpio_evt_mask &= ~(1u << pin_id);
	if(!gpio_evt_mask) {
		gpio_clr_irq_status(GPIO_IRQ_GPIO_STATUS);
		gpio_clr_irq_mask(GPIO_IRQ_MASK_GPIO);
	}
}

static void gpio_evt_arm(u8 pin_id) {
	GPIO_PinTypeDef pin = gpio_evt_pin(pin_id);
	u8 level = gpio_read(pin) ? 1 : 0;
	/* Arm for the edge that is still to come, then let the ISR flip it. */
	gpio_set_interrupt(pin, level ? POL_FALLING : POL_RISING);
	gpio_evt_mask |= (1u << pin_id);
}

static void handle_gpioevt_command(u8 *buf, int cmd_len) {
	u8 op = (cmd_len > 1) ? buf[1] : 0xff;
	u8 pin_id = (cmd_len > 2) ? buf[2] : 0xff;
	u8 status = CMD_STATUS_OK;
	u8 data[6] = {0};

	switch(op) {
	case 0: // query armed-pin bitmap
		break;
	case 1: // arm one pin
		if(cmd_len < 3) {
			status = CMD_STATUS_ARGS;
			break;
		}
		if(!gpio_evt_pin_armable(pin_id)) {
			status = CMD_STATUS_PIN;
			break;
		}
		gpio_evt_arm(pin_id);
		break;
	case 2: // disarm one pin
		if(cmd_len < 3) {
			status = CMD_STATUS_ARGS;
			break;
		}
		if(pin_id > 0x1f) {
			status = CMD_STATUS_PIN;
			break;
		}
		gpio_evt_disarm(pin_id);
		break;
	case 3: // disarm every pin
		for(u8 i = 0; i < 32; i++) {
			if(gpio_evt_mask & (1u << i))
				gpio_evt_disarm(i);
		}
		break;
	default:
		status = CMD_STATUS_ARGS;
		break;
	}
	if(op == 1 || op == 2) {
		/* data[0] = pin, data[1..4] = armed-pin bitmap (little endian) */
		data[0] = pin_id;
		data[1] = gpio_evt_mask;
		data[2] = gpio_evt_mask >> 8;
		data[3] = gpio_evt_mask >> 16;
		data[4] = gpio_evt_mask >> 24;
	} else {
		/* op 0 (query): the bitmap is at data[0..3], as in the ESP32 sibling */
		data[0] = gpio_evt_mask;
		data[1] = gpio_evt_mask >> 8;
		data[2] = gpio_evt_mask >> 16;
		data[3] = gpio_evt_mask >> 24;
	}
	send_resp(CMD_ID_GPIOEVT, status, data, sizeof(data));
}

#if GATT_ENABLE
/* ---------------------------------------------------------------------------
 * GATT client: CMD_ID_TXDATA (0x0D), CMD_ID_RXDATA (0x0E) and the CMD_ID_CONN
 * ops 5..10 (service discovery, read/write, MTU exchange).
 * Enabled only when GATT_ENABLE = 1 (see app_config.h): the host stack does not
 * fit in SRAM together with the extended advertising module used by TXADV.
 * ------------------------------------------------------------------------- */
#define CONN_EVENT_FLAG     0x80u

enum {
	CONN_DISCOV_SERVICE  = 0,
	CONN_DISCOV_CHAR     = 1,
	CONN_DISCOV_DESCR    = 2,
	CONN_DISCOV_COMPLETE = 3
};

enum {
	GATT_DISC_IDLE = 0,
	GATT_DISC_SVC,
	GATT_DISC_CHAR,
	GATT_DISC_DESCR
};

static u8  gatt_disc_state;
static u16 gatt_disc_cur;      /* first handle of the range still to scan */

/* Response made of the 6-byte data field plus an extended payload: byte[0] of
   the frame carries the payload length, exactly like an advertisement report. */
static int send_ext_resp(u8 cmd, u8 id, const u8 *data, u8 dlen, const u8 *ext, u8 elen) {
	u8 *s = my_fifo_wptr(&ad_fifo);
	if(!s)
		return 0;
	if(dlen > 6)
		dlen = 6;
	if(elen > (MYFIFO_BLK_SIZE - HEAD_CRC_ADD_LEN))
		elen = MYFIFO_BLK_SIZE - HEAD_CRC_ADD_LEN;
	memset(s, 0, HEAD_CRC_ADD_LEN);
	s[0] = elen;
	s[1] = cmd;
	s[2] = id;
	s[3] = dlen;
	s[4] = 0xff;
	if(dlen)
		memcpy(&s[5], data, dlen);
	if(elen)
		memcpy(&s[11], ext, elen);
	my_fifo_next(&ad_fifo);
	return 1;
}

/* CMD_ID_RXDATA: ATT notification / indication pushed by the peer. */
static void send_rxdata(u8 opcode, u16 handle, const u8 *value, u16 len) {
	u8 data[6] = {0};
	u8 in_data;
	u8 extra = 0;

	data[0] = handle;
	data[1] = handle >> 8;
	data[2] = (len > 255) ? 255 : (u8)len;
	if(len > 3) {
		in_data = 6;
		if(len - 3 > (MYFIFO_BLK_SIZE - HEAD_CRC_ADD_LEN))
			extra = MYFIFO_BLK_SIZE - HEAD_CRC_ADD_LEN;
		else
			extra = (u8)(len - 3);
		memcpy(&data[3], value, 3);
	} else {
		in_data = (u8)(3 + len);
		if(len)
			memcpy(&data[3], value, len);
	}
	send_ext_resp(CMD_ID_RXDATA, opcode, data, in_data, extra ? value + 3 : 0, extra);
}

/* Emits one CONN discovery frame (byte[2] = 0x80 | sub-type). */
static void send_disc_frame(u8 sub_type, const u8 *data, u8 dlen, const u8 *ext, u8 elen) {
	send_ext_resp(CMD_ID_CONN, CONN_EVENT_FLAG | sub_type, data, dlen, ext, elen);
}

static void gatt_disc_continue(void);

/* ATT packets of the master role (called from ble.c gatt_data_handler()). */
void scanning_gatt_callback(u16 conn_handle, u8 opcode, u16 att_handle, u16 len, u8 *data) {
	(void)conn_handle;

	switch(opcode) {
	case ATT_OP_HANDLE_VALUE_NTF:
	case ATT_OP_HANDLE_VALUE_IND:
		send_rxdata(opcode, att_handle, data, len);
		break;

	case ATT_OP_READ_BY_GROUP_TYPE_RSP: {
		u8 elen = data[0];
		if(elen < 6 || (u16)1 + elen > len)
			break;
		for(u16 off = 1; off + elen <= len; off += elen) {
			u8 out[6];
			u16 start = data[off] | (data[off + 1] << 8);
			u16 end = data[off + 2] | (data[off + 3] << 8);
			out[0] = 1;               /* primary service */
			out[1] = start;
			out[2] = start >> 8;
			out[3] = end;
			out[4] = end >> 8;
			out[5] = 0;
			send_disc_frame(CONN_DISCOV_SERVICE, out, sizeof(out),
					&data[off + 4], elen - 4);
			gatt_disc_cur = end + 1;
		}
		gatt_disc_continue();
		break;
	}

	case ATT_OP_READ_BY_TYPE_RSP: {
		u8 elen = data[0];
		if(elen < 5 || (u16)1 + elen > len)
			break;
		for(u16 off = 1; off + elen <= len; off += elen) {
			u8 out[6];
			u16 hdl = data[off] | (data[off + 1] << 8);
			out[0] = hdl;
			out[1] = hdl >> 8;
			out[2] = data[off + 2];   /* properties */
			out[3] = 0;
			out[4] = 0;
			out[5] = 0;
			send_disc_frame(CONN_DISCOV_CHAR, out, sizeof(out),
					&data[off + 3], elen - 3);
			gatt_disc_cur = hdl + 1;
		}
		gatt_disc_continue();
		break;
	}

	case ATT_OP_FIND_INFO_RSP: {
		u8 elen = (data[0] == 1) ? 4 : 18;
		if(data[0] > 1 || elen > len)
			break;
		for(u16 off = 1; off + elen <= len; off += elen) {
			u8 out[6];
			u16 hdl = data[off] | (data[off + 1] << 8);
			out[0] = hdl;
			out[1] = hdl >> 8;
			out[2] = 0;
			out[3] = 0;
			out[4] = 0;
			out[5] = 0;
			send_disc_frame(CONN_DISCOV_DESCR, out, sizeof(out),
					&data[off + 2], elen - 2);
			gatt_disc_cur = hdl + 1;
		}
		gatt_disc_continue();
		break;
	}

	case ATT_OP_ERROR_RSP:
		if(data[0] == ATT_OP_READ_BY_GROUP_TYPE_REQ && gatt_disc_state == GATT_DISC_SVC) {
			gatt_disc_state = GATT_DISC_CHAR;
			gatt_disc_cur = 0x0001;
			gatt_disc_continue();
		} else if(data[0] == ATT_OP_READ_BY_TYPE_REQ && gatt_disc_state == GATT_DISC_CHAR) {
			gatt_disc_state = GATT_DISC_DESCR;
			gatt_disc_cur = 0x0001;
			gatt_disc_continue();
		} else if(gatt_disc_state != GATT_DISC_IDLE) {
			gatt_disc_state = GATT_DISC_IDLE;
			send_disc_frame(CONN_DISCOV_COMPLETE, 0, 0, 0, 0);
		}
		break;

	case ATT_OP_READ_RSP:
		/* Read result of CMD_ID_CONN op 6/8: [handle_lo, handle_hi, len, value...] */
		{
			u8 out[6];
			u8 extra;
			if(len > 3) {
				extra = (len - 3 > (MYFIFO_BLK_SIZE - HEAD_CRC_ADD_LEN)) ?
						(MYFIFO_BLK_SIZE - HEAD_CRC_ADD_LEN) : (u8)(len - 3);
				memcpy(&out[3], data, 3);
			} else {
				extra = 0;
				out[3] = 0;
				out[4] = 0;
				out[5] = 0;
				if(len)
					memcpy(&out[3], data, len);
			}
			out[0] = att_handle;
			out[1] = att_handle >> 8;
			out[2] = (len > 255) ? 255 : (u8)len;
			send_ext_resp(CMD_ID_CONN, CMD_STATUS_OK, out, 6,
					extra ? data + 3 : 0, extra);
		}
		break;

	default:
		break;
	}
}

static void gatt_disc_continue(void) {
	switch(gatt_disc_state) {
	case GATT_DISC_SVC:
		if(conn_gatt_disc_services() != BLE_SUCCESS) {
			gatt_disc_state = GATT_DISC_IDLE;
			send_disc_frame(CONN_DISCOV_COMPLETE, 0, 0, 0, 0);
		}
		break;
	case GATT_DISC_CHAR:
		if(conn_gatt_disc_chars(gatt_disc_cur, 0xFFFF) != BLE_SUCCESS) {
			gatt_disc_state = GATT_DISC_IDLE;
			send_disc_frame(CONN_DISCOV_COMPLETE, 0, 0, 0, 0);
		}
		break;
	case GATT_DISC_DESCR:
		if(conn_gatt_disc_descrs(gatt_disc_cur, 0xFFFF) != BLE_SUCCESS) {
			gatt_disc_state = GATT_DISC_IDLE;
			send_disc_frame(CONN_DISCOV_COMPLETE, 0, 0, 0, 0);
		}
		break;
	default:
		break;
	}
}

/* CMD_ID_CONN op 5: start the full service/characteristic/descriptor discovery. */
static u8 gatt_disc_start(void) {
	gatt_disc_state = GATT_DISC_SVC;
	gatt_disc_cur = 0x0001;
	if(conn_gatt_disc_services() != BLE_SUCCESS) {
		gatt_disc_state = GATT_DISC_IDLE;
		return CMD_STATUS_DENIED;
	}
	return CMD_STATUS_OK;
}
#endif /* GATT_ENABLE */

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

/* ---------------------------------------------------------------------------
 * CMD_ID_LED op 3 - non-blocking blink.
 *
 * The main loop of this firmware is scan_task(), which also drains the UART.
 * A blocking blink therefore stops the device from answering the host for its
 * whole duration, so the blink is run as a small state machine instead.
 * ------------------------------------------------------------------------- */
static u8  led_blink_pending_mask;
static u8  led_blink_steps_left;
static u8  led_blink_level;
static u32 led_blink_delay_us;
static u32 led_blink_next_tick;

static void led_blink_start(u8 mask, u8 count, u8 delay_10ms) {
	if(count == 0)
		count = 1;
	if(count > 5)
		count = 5;
	if(delay_10ms == 0)
		delay_10ms = 1;
	if(delay_10ms > 20)
		delay_10ms = 20;
	/* keep the idle LED timers in scan_task from fighting the blink */
	manual_led_mask |= mask;
	led_blink_pending_mask = mask;
	led_blink_steps_left = (u8)(count * 2);
	led_blink_delay_us = (u32)delay_10ms * 10000;
	led_blink_level = 1;
	led_blink_next_tick = clock_time() | 1;
	led_write_mask(mask, mask, 0);
}

static void led_blink_task(void) {
	if(!led_blink_steps_left)
		return;
	if(!clock_time_exceed(led_blink_next_tick, led_blink_delay_us))
		return;
	led_blink_next_tick += led_blink_delay_us;
	led_blink_steps_left--;
	if(led_blink_steps_left == 0) {
		/* finished: park the LEDs off and give them back to the idle timers */
		led_write_mask(led_blink_pending_mask, 0, 0);
		manual_led_mask &= ~led_blink_pending_mask;
		led_blink_pending_mask = 0;
		return;
	}
	led_blink_level = !led_blink_level;
	led_write_mask(led_blink_pending_mask, led_blink_level ? led_blink_pending_mask : 0, 0);
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
		/* Non-blocking blink, driven from scan_task: the old implementation
		   slept up to 2 s inside the command handler, which stalls the main
		   loop (UART polling, BLE stack) and makes the host think the
		   firmware crashed. */
		u8 count = (cmd_len > 4) ? buf[4] : 3;
		u8 delay_10ms = (cmd_len > 5) ? buf[5] : 5;
		led_blink_start(mask, count, delay_10ms);
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

static void handle_vbat_command(void) {
	u8 data[4] = {0};
	u16 mv = adc_read_vbat_mv();

	data[0] = mv;
	data[1] = mv >> 8;
	/* Temperature is not available on TLSR8253 / B85: report 0x8000. */
	data[2] = 0x00;
	data[3] = 0x80;

	send_resp(CMD_ID_VBAT, CMD_STATUS_OK, data, sizeof(data));
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
			if(phy != 0) {
				status = CMD_STATUS_VALUE;
			} else if(adv_len > 31 || cmd_len < 6 + adv_len) {
				status = CMD_STATUS_ARGS;
			} else {
				if(interval < 32)
					interval = 32; // min 20 ms
				if(txadv_running_phy)
					txadv_stop();
				/* Scanning and TXADV share the radio: stop the scanner before
				   enabling the advertisement. */
				if(scanning_active)
					start_adv_scanning(0, 0, 0);
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
	} else if(op >= 5 && op <= 10) {
		// GATT client operations (ESP32 encoding: a leading 0 byte, then the
		// 16-bit little-endian handle / MTU)
#if GATT_ENABLE
		switch(op) {
		case 5:
			resp_status = gatt_disc_start();
			break;
		case 6:
		case 8:
			if(cmd_len < 5) {
				resp_status = CMD_STATUS_ARGS;
			} else {
				ble_sts_t s = conn_gatt_read((u16)buf[3] | ((u16)buf[4] << 8));
				resp_status = (s == BLE_SUCCESS) ? CMD_STATUS_OK : CMD_STATUS_DENIED;
			}
			break;
		case 7:
		case 9: {
			u8 dlen;
			if(cmd_len < 6) {
				resp_status = CMD_STATUS_ARGS;
				break;
			}
			dlen = buf[5];
			if(dlen > 20 || cmd_len < 6 + dlen) {
				resp_status = CMD_STATUS_ARGS;
				break;
			}
			{
				ble_sts_t s = conn_gatt_write((u16)buf[3] | ((u16)buf[4] << 8), &buf[6], dlen);
				resp_status = (s == BLE_SUCCESS) ? CMD_STATUS_OK : CMD_STATUS_DENIED;
			}
			break;
		}
		case 10:
			if(cmd_len < 5) {
				resp_status = CMD_STATUS_ARGS;
			} else {
				ble_sts_t s = conn_gatt_set_mtu((u16)buf[3] | ((u16)buf[4] << 8));
				resp_status = (s == BLE_SUCCESS) ? CMD_STATUS_OK : CMD_STATUS_DENIED;
			}
			break;
		default:
			resp_status = CMD_STATUS_ARGS;
			break;
		}
#else
		// GATT client not compiled in (GATT_ENABLE = 0)
		resp_status = CMD_STATUS_DENIED;
#endif
	} else {
		resp_status = CMD_STATUS_ARGS;
	}
	send_resp(CMD_ID_CONN, resp_status, data, sizeof(data));
}

// CMD_ID_TXDATA: ATT Write Without Response (Write Command) to the connected peer.
// buf[1..2] = att_handle (LE), buf[3] = data_len, buf[4..] = data
static void handle_txdata_command(u8 *buf, int cmd_len) {
	u8 data[6] = {0};
	u8 status = CMD_STATUS_DENIED;

	if(cmd_len < 4) {
		status = CMD_STATUS_ARGS;
	} else {
		u8 dlen = buf[3];
		data[0] = buf[1];
		data[1] = buf[2];
		data[2] = dlen;
		if(dlen > 20 || cmd_len < 4 + dlen) {
			status = CMD_STATUS_ARGS;
		} else {
#if GATT_ENABLE
			ble_sts_t s = conn_gatt_write_cmd((u16)buf[1] | ((u16)buf[2] << 8), &buf[4], dlen);
			status = (s == BLE_SUCCESS) ? CMD_STATUS_OK : CMD_STATUS_DENIED;
			data[3] = (s == BLE_SUCCESS) ? 0 : 1;
#else
			data[3] = 1;	/* ATT error: GATT client not available */
			status = CMD_STATUS_DENIED;
#endif
		}
	}
	send_resp(CMD_ID_TXDATA, status, data, sizeof(data));
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
			if(led_status_allowed(LED_MASK_BLUE)) {
				gpio_write(GPIO_LED_B, 1);
				leds.tb = reg_system_tick;
			}
#endif
		}
#ifdef 	GPIO_LED_R
		else if(led_status_allowed(LED_MASK_RED)) {
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
				if(pa->primary_phy == 1 && led_status_allowed(LED_MASK_BLUE)) {
					gpio_write(GPIO_LED_B, 1);
					leds.tb = reg_system_tick;
				} else if(pa->primary_phy == 3 && led_status_allowed(LED_MASK_GREEN)) {
					gpio_write(GPIO_LED_G, 1);
					leds.tg = reg_system_tick;
				}
#endif
			}
#ifdef 	GPIO_LED_R
			else if(led_status_allowed(LED_MASK_RED)) {
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
				if(periodic_adv.advPHY == 1 && led_status_allowed(LED_MASK_BLUE)) {
					gpio_write(GPIO_LED_B, 1);
					leds.tb = reg_system_tick;
				} else if(periodic_adv.advPHY == 3 && led_status_allowed(LED_MASK_GREEN)) {
					gpio_write(GPIO_LED_G, 1);
					leds.tg = reg_system_tick;
				}
#endif
			}
#ifdef 	GPIO_LED_R
			else if(led_status_allowed(LED_MASK_RED)) {
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
/* NOTE: do NOT put __attribute__((optimize(...))) on this function (or on any
   function containing a switch).  tc32-gcc 4.5.1 miscompiles the jump table of
   a switch inside a function with a per-function optimize() attribute: it emits
   the table inline as 16-bit relative offsets while the dispatch code indexes it
   with a 4 byte stride, so every case behind the first one jumps to garbage and
   the CPU wedges (see "Known toolchain pitfall" in the README). */
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
			if(led_status_allowed(LED_MASK_YELLOW)) {
				gpio_write(GPIO_LED_E, 1);
				leds.te = tt;
			}
#endif
		}
#ifdef 	TIMER_LED_R
		else if(led_status_allowed(LED_MASK_RED)) {
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
	// CMD_ID_GPIOEVT: refresh the event timestamp and push queued edge events
	// (no-op unless a pin has been armed with the CMD_ID_GPIOEVT command).
	gpio_evt_update_ms();
	gpio_evt_drain();
	// CMD_ID_LED op 3: step the non-blocking blink machine.
	led_blink_task();
	uint8_t buf[64];  // 64 bytes: enough for max CMD_ID_TXADV (6 hdr + 31 data + 2 CRC = 39)
	// CMD_ID_TXDATA worst case: 4 hdr + 20 data + 2 CRC = 26 bytes (fits in 64)
	int len = uart_read(buf, sizeof(buf));
	if(len > 2 && crcFast(buf, len) == 0) {
#ifdef 	TIMER_LED_W
		/* White LED (PB5) = "command received".  It is one of the pins the host
		   can drive with CMD_ID_GPIO, so do not touch it once the host owns it:
		   that is what made the LED look latched on after a manual write. */
		if(led_status_allowed(LED_MASK_WHITE)) {
			gpio_write(GPIO_LED_W, 1);
			leds.tw = tt;
		}
#endif
		int cmd = buf[0];
		int cmd_len = len - 2;
		bb_checkpoint(0x00010000u | ((u32)cmd << 8) | (u32)((cmd_len > 1) ? buf[1] : 0));
		if(cmd == CMD_ID_SCAN && (len == 3 + 3 || len == 5 + 3)) {
			u8  scan_mode = buf[1];
			u16 tdw_1m    = buf[2] | (buf[3] << 8);
			u16 tdw_coded = (len == 5 + 3) ? (buf[4] | (buf[5] << 8)) : 0;
#if TXADV_ENABLE
			/* Scanning and TXADV share the radio: they cannot run at the same
			   time.  Starting a scan stops a running advertisement. */
			if((scan_mode & 3) && txadv_running_phy) {
				txadv_stop();
				txadv_running_phy = 0;
			}
#endif
			start_adv_scanning(scan_mode, tdw_1m, tdw_coded);
			send_resp(cmd, mac_list.count, &buf[1], len - 3);
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
		} else if(cmd == CMD_ID_VBAT && len == 3) {
			handle_vbat_command();
		} else if(cmd == CMD_ID_PRNT) {
			/* The host may ask for the black box at any time (op 1): the report is
			   rebuilt from the live values and sent by bb_report_task() in
			   CMD_ID_PRNT chunks, exactly like the boot report. */
			if(cmd_len > 1 && buf[1] == 1)
				bb_report();
		} else if(cmd == CMD_ID_TXADV) {
			handle_txadv_command(buf, cmd_len);
		} else if(cmd == CMD_ID_CONN) {
			handle_conn_command(buf, cmd_len);
		} else if(cmd == CMD_ID_TXDATA) {
			handle_txdata_command(buf, cmd_len);
		} else if(cmd == CMD_ID_GPIOEVT) {
			handle_gpioevt_command(buf, cmd_len);
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
	} else if(len > 2) {
		/* A frame arrived but its CRC does not match: either the link is noisy
		   or the host sent two commands back to back and the UART merged them
		   into a single DMA read (the host -> device frames carry no length
		   delimiter).  Report it so the host can tell a lost frame apart from a
		   firmware that stopped answering; rate limited to avoid a report storm
		   on a bad link. */
		static u32 last_bad_frame_tick;
		if(clock_time_exceed(last_bad_frame_tick, 200000)) {
			u8 bad[2];
			last_bad_frame_tick = clock_time();
			bad[0] = CMD_OP_BAD_FRAME;
			bad[1] = (u8)((len > 0xff) ? 0xff : len);
			send_resp(buf[0], CMD_STATUS_ARGS, bad, sizeof(bad));
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
