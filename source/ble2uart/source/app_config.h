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
#define GPIO_LED_B		GPIO_PC2
#define GPIO_LED_R		GPIO_PC3
#define GPIO_LED_G		GPIO_PC4
#define GPIO_LED_E		GPIO_PB4	// tx adv
#define GPIO_LED_W		GPIO_PB5
#define GPIO_TX			GPIO_PB1
#define GPIO_RX			GPIO_PA0

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

#define ATT_LEGACY_MTU_SIZE  23

#define RAM _attribute_data_retention_ // short version, this is needed to keep the values in ram after sleep

#include "vendor/common/default_config.h"

