/********************************************************************************************************
 * @file    app_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

#include"config.h"
#include "../feature_config.h"

#if (FEATURE_TEST_MODE == TEST_BLE_PHY)


#define ACL_CENTRAL_MAX_NUM								1
#define ACL_PERIPHR_MAX_NUM								1

///////////////////////// PHYTEST CONFIGURATION ////////////////////////////
#define 		PHYTEST_MODE_THROUGH_2_WIRE_UART		1   //Direct Test Mode through a 2-wire UART interface
#define 		PHYTEST_MODE_OVER_HCI_WITH_USB			2   //Direct Test Mode over HCI(USB hardware interface)
#define 		PHYTEST_MODE_OVER_HCI_WITH_UART			3   //Direct Test Mode over HCI(UART hardware interface)

#define         BLE_PHYTEST_MODE					    PHYTEST_MODE_OVER_HCI_WITH_UART


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE						1   //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE						0   //1 for smp,  0 no security


///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE							0
#define UART_PRINT_DEBUG_ENABLE                     1 //printf

#define APP_LOG_EN							1
#define APP_SMP_LOG_EN						0
#define APP_KEY_LOG_EN						1
#define APP_CONTR_EVENT_LOG_EN				1  //controller event log
#define APP_HOST_EVENT_LOG_EN				1  //host event log

#define APP_DEFAULT_HID_BATTERY_OTA_ATTRIBUTE_TABLE			1


/////////////////////// Board Select Configuration ///////////////////////////////
#if (CHIP_TYPE == CHIP_TYPE_825x)
	//Only support BOARD_825X_EVK_C1T139A30
	#define BOARD_SELECT							BOARD_825X_EVK_C1T139A30
#elif (CHIP_TYPE == CHIP_TYPE_827x)
	//Only support BOARD_827X_EVK_C1T197A30
	#define BOARD_SELECT							BOARD_827X_EVK_C1T197A30
#elif (CHIP_TYPE == CHIP_TYPE_TC321X)
	//Only support BOARD_TC321X_EVK_C1T357A20
#define BOARD_SELECT								BOARD_TC321X_EVK_C1T357A20
#endif

#if (CHIP_TYPE == CHIP_TYPE_TC321X)
#define UART_NUM							UART0
#define UART_CONVERT(...)					UART_NUM,__VA_ARGS__
#define UART_TX_PIN							GPIO_PD0
#define UART_RX_PIN							GPIO_PD1
#else
#define UART_NUM
#define UART_CONVERT(...)					__VA_ARGS__
#define UART_TX_PIN							UART_TX_PB1
#define UART_RX_PIN							UART_RX_PB0
#endif

#define	UART_BAUDRATE       				(115200)

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE								0
#define	UI_KEYBOARD_ENABLE							0


/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ  							32000000



/////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
#if (UART_PRINT_DEBUG_ENABLE)
	#define DEBUG_INFO_TX_PIN           		GPIO_PA0
	#define PULL_WAKEUP_SRC_PA0         		PM_PIN_PULLUP_10K
	#define PA0_OUTPUT_ENABLE         			1
	#define PA0_DATA_OUT                     	1 //must
#endif

///////////////////////// PHYTEST CONFIGURATION ////////////////////////////

/*----------------------------------------------*
 *	HCI TX FIFO  = 2 Bytes LEN + n Bytes Data.	*
 *												*
 *	T_txdata_buf = 4 Bytes LEN + n Bytes Data.	*
 *												*
 *	HCI_TXFIFO_SIZE = 2 + n.					*
 *												*
 *	UART_DATA_LEN = n.							*
 *												*
 *	UART_DATA_LEN = HCI_TXFIFO_SIZE - 2.		*
 * ---------------------------------------------*/
#define HCI_RXFIFO_SIZE		72
#define HCI_RXFIFO_NUM		2

#define HCI_TXFIFO_SIZE		72
#define HCI_TXFIFO_NUM		8

#define UART_DATA_LEN    	(HCI_TXFIFO_SIZE - 2)      // data max 252



#include "../../common/default_config.h"

#endif
