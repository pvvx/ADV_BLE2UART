#pragma once

#include "config.h"

#define SW_VERSION 0x01	 // BCD format (0x34 -> '3.4')

#define DEBUG_MSG           0  // Set to 0 to disable debug mode; set to 1 to print UART debug messages as uart_printf

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

#define GPIO_LED_W		GPIO_PB5  // Lateral White LED2, B5: Host command received from the UART
#define PB5_DATA_OUT		0
#define PB5_OUTPUT_ENABLE	1
#define PB5_INPUT_ENABLE	1
#define PB5_FUNC			AS_GPIO

#define GPIO_TX			GPIO_PB1  // UART_TX_PB1, TXD

#define GPIO_RX			GPIO_PA0  // UART_RX_PA0, RXD
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

#define RAM _attribute_data_retention_ // short version, this is needed to keep the values in ram after sleep

/////////////////// Clock  /////////////////////////////////
#define	SYS_CLK_TYPE										SYS_CLK_32M_Crystal

#if(SYS_CLK_TYPE == SYS_CLK_32M_Crystal)
	#define CLOCK_SYS_CLOCK_HZ  							32000000
#elif(SYS_CLK_TYPE == SYS_CLK_48M_Crystal)
	#define CLOCK_SYS_CLOCK_HZ  							48000000
#endif

enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};
