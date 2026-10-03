#pragma once

#include "config.h"

#define SW_VERSION 0x15	// BCD format (0x34 -> '3.4')

#define DEBUG_MSG           0  // Set to 0 to disable debug mode; set to 1 to print UART debug messages as uart_printf

// CMD_ID_TXADV (0x0B): custom advertisement transmitter (legacy 1M
// ADV_NONCONN_IND).  It requires the LEGACY advertising module and the
// peripheral (slave) role.
//
// Scanning and TXADV share the radio, so they cannot run at the same time: the
// command handlers stop one before starting the other (see the CMD_ID_SCAN and
// CMD_ID_TXADV branches of scan_task()).
//
// KNOWN COST (measured on the TB-03F-KIT, native V4.0.2.5 library, interleaved
// A/B): linking this module costs the Coded-PHY scan.  With the module the
// extended scan (CMD_ID_SCAN mode 3) drops the first Coded advertisement, stops
// answering and reboots; without it the same source scans 1M + Coded (and 2M)
// normally.  The trigger is the 2308 bytes of library BSS the module pulls in,
// not the initialisation call (which is only reached when TXADV starts).
//
// DEFAULT IS 0: the Coded-PHY scan is the product feature, so 0x0B answers
// CMD_STATUS_DENIED out of the box.  Switch it on for a build that broadcasts:
//   make EXTRA_FLAGS="-DTXADV_ENABLE=1"
//   TXADV_ENABLE = 1: legacy 1M broadcast transmitter, Coded-PHY scan not usable
//                     (the 1M scan is unaffected);
//   TXADV_ENABLE = 0 (default): full 1M + Coded scan, 0x0B -> CMD_STATUS_DENIED.
//
// The extended advertising module (the other way to get Coded TX) is not usable
// here either: it wedges the RF ISR as soon as an extended advertising set is
// enabled.
#ifndef TXADV_ENABLE
#define TXADV_ENABLE		0
#endif

// CMD_ID_CONN ops 5..10 (service discovery, read/write, MTU), CMD_ID_TXDATA
// (0x0D) and CMD_ID_RXDATA (0x0E): GATT client.
// The L2CAP/ATT/GATT host objects are already linked by the calls the scanner
// needs anyway (blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler),
// blc_gap_init(), blc_l2cap_initAclConnMasterMtuBuffer()), so the client costs
// only ~4 bytes of BSS (measured 0x84D57C -> 0x84D580 with TXADV disabled) and
// is enabled by default.  It can safely be combined with TXADV_ENABLE.
#ifndef GATT_ENABLE
#define GATT_ENABLE			1
#endif

// System Timer guard (see sched_timer_guard() in scanning.c): the V4.0.2.5
// library masks the System Timer interrupt off as soon as nothing needs it and
// never re-arms the tick, after which the main loop never resumes (measured:
// death inside blc_sdk_main_loop with FLD_IRQ_SYSTEM_TIMER missing from the
// mask).  Set to 0 to reproduce the unguarded behaviour.
//
// DEFAULT 0 since fw 0.14: the guard also re-asserts bit 19 of g_scheMng+0x14,
// which is the bit the library uses for a pending auxiliary scan task, and the
// firmware froze only while that bit was forced on (Bs=00080000) whereas the
// ext_adv_test build - same module list, no bit - runs.
#ifndef SCHED_TIMER_GUARD
#define SCHED_TIMER_GUARD		0
#endif

// BLE_DEVICE_ENABLE = 1: CMD_ID_TXADV needs the peripheral (slave) role.
// The V4.0.2.5 library refuses the legacy advertising API -- blc_ll_setAdvEnable
// returns 0x0D HCI_ERR_CONN_REJ_LIMITED_RESOURCES -- unless the peripheral role
// module is initialised, and app_buffer.c emits app_acl_slvTxfifo / mtu_s_*
// only when this macro is 1.
#define BLE_DEVICE_ENABLE	1
#define BLE_MASTER_ENABLE	1  // enable central/master role for CMD_ID_CONN

#define MODULE_WATCHDOG_ENABLE		0	// WDT not use!
#define WATCHDOG_INIT_TIMEOUT		250  // ms

#define USE_TIME_ADJUST		0 // = 1 time correction enabled

#define UART_PRINT_DEBUG_ENABLE 	0

#define APP_BATT_CHECK_ENABLE	1
#if (CHIP_TYPE == CHIP_TYPE_TC321X)
#define USED_DEEP_ANA_REG	PM_ANA_REG_POWER_ON_CLR_BUF1
#else
#define USED_DEEP_ANA_REG	DEEP_ANA_REG0
#endif
#define LOW_BATT_FLG		BIT(0)

//// TB-03F-KIT
#define HW_VERSION 16 // DIY TB-03F-Kit
// PC2,3,4 - LED_RGB
// PB4,5 - LED1, LED2
// PB1	- UART TX
// PA0  - UART RX
// PA7  - KEY

// TB-03F-KIT           TLSR8253F512 PIN and description of the application function
#define	KEY_USER		GPIO_PA7  // SWS (UART_DTR)

#define	GPIO_KEY1				GPIO_PA7
#define PA7_FUNC					AS_GPIO
#define PA7_OUTPUT_ENABLE			0
#define PA7_INPUT_ENABLE			1
#define	PULL_WAKEUP_SRC_PA7			PM_PIN_PULLUP_10K

#define GPIO_LED_B		GPIO_PC2  // RGB Blue LED1, C2: Coded PHY advertisement received
#define PC2_DATA_OUT		0
#define PC2_OUTPUT_ENABLE	1
#define PC2_INPUT_ENABLE	1
#define PC2_FUNC			AS_GPIO
#define PWM_LED_B		PWM0_ID

#define GPIO_LED_R		GPIO_PC3  // RGB Red LED1, C3 (PWM1): FIFO overflow errors, when the UART troughtput is not enough to process all received BLE advertisements
#define PC3_DATA_OUT		0
#define PC3_OUTPUT_ENABLE	1
#define PC3_INPUT_ENABLE	1
#define PC3_FUNC			AS_GPIO
#define PWM_LED_R		PWM1_ID

#define GPIO_LED_G		GPIO_PC4  // RGB Green LED1, C4 (PWM2): PHY 1M advertisement received
#define PC4_DATA_OUT		0
#define PC4_OUTPUT_ENABLE	1
#define PC4_INPUT_ENABLE	1
#define PC4_FUNC			AS_GPIO
#define PWM_LED_G		PWM2_ID

#define GPIO_LED_E		GPIO_PB4  // Lateral small Yellow LED3, B4: Advertisement succesfully sent to the UART FIFO
#define PB4_DATA_OUT		0
#define PB4_OUTPUT_ENABLE	1
#define PB4_INPUT_ENABLE	1
#define PB4_FUNC			AS_GPIO
#define PWM_LED_E		PWM4_ID  // LED3 (yellow) is wired to PWM4

#define GPIO_LED_W		GPIO_PB5  // Lateral White LED2, B5: Host command received from the UART
#define PB5_DATA_OUT		0
#define PB5_OUTPUT_ENABLE	1
#define PB5_INPUT_ENABLE	1
#define PB5_FUNC			AS_GPIO
#define PWM_LED_W		PWM5_ID  // LED2 (white) is wired to PWM5

#define GPIO_TX			GPIO_PB1  // UART_TX_PB1, TXD
// PB1        - UART TX to CH340C RXD

#define GPIO_VBAT_DETECT	GPIO_PB7  // temporary ADC pad for VBAT/3V3 measurement; keep PB7 free

#define GPIO_RX			GPIO_PA0  // UART_RX_PA0, RXD
// PA0  - UART RX from CH340C TXD
#define PA0_OUTPUT_ENABLE	0
#define PA0_INPUT_ENABLE	1
#define PULL_WAKEUP_SRC_PA0 PM_PIN_PULLUP_1M
#define PA0_FUNC		AS_GPIO

#define UART_CH340_TX_PIN	UART_TX_PB1
#define UART_CH340_RX_PIN	UART_RX_PA0

#if UART_PRINT_DEBUG_ENABLE
#define PRINT_BAUD_RATE 1500000 // real 1000000
#define DEBUG_INFO_TX_PIN	GPIO_PB1
#define PB1_DATA_OUT		1
#define PB1_OUTPUT_ENABLE	1
#define PULL_WAKEUP_SRC_PB1 PM_PIN_PULLUP_1M
#define PB1_FUNC		AS_GPIO
#endif // UART_PRINT_DEBUG_ENABLE

#define PC2_DATA_OUT		0
#define PC2_OUTPUT_ENABLE	1
#define PC2_INPUT_ENABLE	1
#define PC2_FUNC			AS_GPIO

/* Power 3.3V, RX RF + TX ADV 1 sec, max:
 * 48MHz 7.3 mA
 * 32MHz 6.6 mA
 * 24MHz 6.2 mA
 * 16MHz 5.8 mA
 */

#define ATT_LEGACY_MTU_SIZE  23

#define MASTER_MAX_NUM	1
#define SLAVE_MAX_NUM	1   // CMD_ID_TXADV uses the legacy advertising API: advertising
                           // is a peripheral-role activity and without this the
                           // enable call answers 0x0D (LIMITED_RESOURCES)
#define RAM _attribute_data_retention_ // short version, this is needed to keep the values in ram after sleep

/////////////////// Clock  /////////////////////////////////
#define	SYS_CLK_TYPE										SYS_CLK_32M_Crystal

#if(SYS_CLK_TYPE == SYS_CLK_32M_Crystal)
	#define CLOCK_SYS_CLOCK_HZ  							32000000
#elif(SYS_CLK_TYPE == SYS_CLK_48M_Crystal)
	#define CLOCK_SYS_CLOCK_HZ  							48000000
#endif

/* CLOCK_SYS_CLOCK_1S/1MS/1US provided by SDK vendor/common/app_common.h */

#include "vendor/common/default_config.h"
