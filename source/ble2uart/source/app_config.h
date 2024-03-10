#pragma once

#include"config.h"

#define SW_VERSION 0x01	 // BCD format (0x34 -> '3.4')

#define BLE_DEVICE_ENABLE	0
#define BLE_MASTER_ENABLE	0

#define MODULE_WATCHDOG_ENABLE		0	// WDT not use!
#define WATCHDOG_INIT_TIMEOUT		250  // ms

#define USE_TIME_ADJUST		0 // = 1 time correction enabled

#define UART_PRINT_DEBUG_ENABLE 	0

//// TB-03F-KIT
#define HW_VERSION 16 // DIY TB-03F-Kit
// PC2,3,4 - LED_RGB
// PB4,5 - LED1, LED2
// PB1	- UART TX
// PA0  - UART RX
// PA7  - KEY

#define	GPIO_KEY1				GPIO_PA7
#define PA7_FUNC					AS_GPIO
#define PA7_OUTPUT_ENABLE			0
#define PA7_INPUT_ENABLE			1
#define	PULL_WAKEUP_SRC_PA7			PM_PIN_PULLUP_10K

#define GPIO_LED_B		GPIO_PC2
#define PC2_DATA_OUT		0
#define PC2_OUTPUT_ENABLE	1
#define PC2_INPUT_ENABLE	1
#define PC2_FUNC			AS_GPIO
#define PWM_LED_B		PWM0_ID

#define GPIO_LED_R		GPIO_PC3
#define PC3_DATA_OUT		0
#define PC3_OUTPUT_ENABLE	1
#define PC3_INPUT_ENABLE	1
#define PC3_FUNC			AS_GPIO
#define PWM_LED_R		PWM1_ID

#define GPIO_LED_G		GPIO_PC4
#define PC4_DATA_OUT		0
#define PC4_OUTPUT_ENABLE	1
#define PC4_INPUT_ENABLE	1
#define PC4_FUNC			AS_GPIO
#define PWM_LED_G		PWM2_ID

#define GPIO_LED_E		GPIO_PB4	// tx adv
#define PB4_DATA_OUT		0
#define PB4_OUTPUT_ENABLE	1
#define PB4_INPUT_ENABLE	1
#define PB4_FUNC			AS_GPIO

#define GPIO_LED_W		GPIO_PB5
#define PB5_DATA_OUT		0
#define PB5_OUTPUT_ENABLE	1
#define PB5_INPUT_ENABLE	1
#define PB5_FUNC			AS_GPIO

#define GPIO_TX			GPIO_PB1

#define GPIO_RX			GPIO_PA0
#define PA0_OUTPUT_ENABLE	0
#define PULL_WAKEUP_SRC_PA0 PM_PIN_PULLUP_1M
#define PA0_FUNC		AS_GPIO

#if UART_PRINT_DEBUG_ENABLE
#define PRINT_BAUD_RATE 1500000 // real 1000000
#define DEBUG_INFO_TX_PIN	GPIO_PB1
#define PB1_DATA_OUT		1
#define PB1_OUTPUT_ENABLE	1
#define PULL_WAKEUP_SRC_PB1 PM_PIN_PULLUP_1M
#define PB1_FUNC		AS_GPIO
#endif // UART_PRINT_DEBUG_ENABLE

/* Power 3.3V, RX RF + TX ADV 1 sec, max:
 * 48MHz 7.3 mA
 * 32MHz 6.6 mA
 * 24MHz 6.2 mA
 * 16MHz 5.8 mA
 */
#define CLOCK_SYS_CLOCK_HZ  	32000000 // 16000000, 24000000, 32000000, 48000000
enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};

//#define	CONN_MAX_NUM_CONFIG							CONN_MAX_NUM_M0_S4
#if BLE_MASTER_ENABLE
#define MASTER_MAX_NUM								1
#else
#define MASTER_MAX_NUM								0
#endif
#define SLAVE_MAX_NUM								1

#define	FLASH_SIZE_CONFIG		   				FLASH_SIZE_512K

#define PM_DEEPSLEEP_RETENTION_ENABLE				BLE_APP_PM_ENABLE

#define ATT_LEGACY_MTU_SIZE  23


//#define pm_wait_ms(t) cpu_stall_wakeup_by_timer0(t*CLOCK_SYS_CLOCK_1MS);
//#define pm_wait_us(t) cpu_stall_wakeup_by_timer0(t*CLOCK_SYS_CLOCK_1US);

#define RAM _attribute_data_retention_ // short version, this is needed to keep the values in ram after sleep

#include "default_config.h"

