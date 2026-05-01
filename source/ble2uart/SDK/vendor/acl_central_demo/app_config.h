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

#include "config.h"


#define ACL_CENTRAL_MAX_NUM						    4
#define ACL_PERIPHR_MAX_NUM							0


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_CENTRAL_SMP_ENABLE						1	//1 for smp,  0 no security
#define ACL_CENTRAL_SIMPLE_SDP_ENABLE				1	//simple service discovery for BLE master
#define APPLICATION_DONGLE							0   //0



/* Flash Protection:
 * 1. Flash protection is enabled by default in SDK. User must enable this function on their final mass production application.
 * 2. User should use "Unlock" command in Telink BDT tool for Flash access during development and debugging phase.
 * 3. Flash protection demonstration in SDK is a reference design based on sample code. Considering that user's final application may
 *    different from sample code, for example, user's final firmware size is bigger, or user have a different OTA design, or user need
 *    store more data in some other area of Flash, all these differences imply that Flash protection reference design in SDK can not
 *    be directly used on user's mass production application without any change. User should refer to sample code, understand the
 *    principles and methods, then change and implement a more appropriate mechanism according to their application if needed.
 */
#define APP_FLASH_PROTECTION_ENABLE						1

/* User must check battery voltage on mass production application to prevent abnormal writing or erasing Flash at a low voltage !!! */
#define APP_BATT_CHECK_ENABLE							0


/////////////////////// Sample Board Select Configuration ///////////////////////////////
#if (CHIP_TYPE == CHIP_TYPE_825x)
	//Only support BOARD_825X_EVK_C1T139A30 & BOARD_825X_DONGLE_C1T139A3
	#define BOARD_SELECT							BOARD_825X_DONGLE_C1T139A3
#elif (CHIP_TYPE == CHIP_TYPE_827x)
	//Only support BOARD_827X_EVK_C1T197A30 & BOARD_827X_DONGLE_C1T201A3
	#define BOARD_SELECT							BOARD_827X_EVK_C1T197A30
#endif


///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define	UI_LED_ENABLE								1


///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE							0
#define DEBUG_SIHUI_GPIO_ENABLE						0

#define UART_PRINT_DEBUG_ENABLE                     1  //printf

#define APP_LOG_EN							        1
#define APP_CONTR_EVT_LOG_EN                        1   //controller event
#define APP_HOST_EVT_LOG_EN							1
#define APP_PAIR_LOG_EN                             1
#define APP_SMP_LOG_EN						        1
#define APP_KEY_LOG_EN						        1
#define APP_FLASH_INIT_LOG_EN				        1
#define APP_FLASH_PROT_LOG_EN				        1





#if(APPLICATION_DONGLE)
	#define ID_VENDOR				0x248a			// for report
	#define ID_PRODUCT_BASE			0x880c			//AUDIO_HOGP
	#define STRING_VENDOR			L"Telink"
	#define STRING_PRODUCT			L"BLE Master Dongle"
	#define STRING_SERIAL			L"TLSR9518"

	//////////////////// Audio /////////////////////////////////////
	#define MIC_RESOLUTION_BIT		16
	#define MIC_SAMPLE_RATE			16000
	#define MIC_CHANNEL_COUNT		1
	#define	MIC_ENCODER_ENABLE		0

	#define PA5_FUNC				AS_USB//AS_USB_DM
	#define PA6_FUNC				AS_USB//AS_USB_DP
	#define PA5_INPUT_ENABLE		1
	#define PA6_INPUT_ENABLE		1

	#define	USB_PRINTER_ENABLE 		1
	#define	USB_SPEAKER_ENABLE 		0
	#define UI_AUDIO_ENABLE			0

	#define AUDIO_HOGP				0

	#define	USB_MIC_ENABLE 			1
	#define	USB_MOUSE_ENABLE 		1
	#define	USB_KEYBOARD_ENABLE 	1
	#define	USB_SOMATIC_ENABLE      0   //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
	#define USB_CUSTOM_HID_REPORT	1
#endif

#if(UI_AUDIO_ENABLE)
	#define	MIC_ADPCM_FRAME_SIZE	128
	#define	MIC_SHORT_DEC_SIZE		248

	#define	MIC_ADPCM_FRAME_SIZE_NUM	4
	#define	MIC_SHORT_DEC_SIZE_NUM		4
#endif





/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ  							32000000

/////////////////// DEEP SAVE FLG //////////////////////////////////
#if (CHIP_TYPE == CHIP_TYPE_TC321X)
	#define USED_DEEP_ANA_REG				PM_ANA_REG_WD_CLR_BUF1
#else
	#define USED_DEEP_ANA_REG               DEEP_ANA_REG0 //u8,can save 8 bit info when deep
#endif
#define	LOW_BATT_FLG					    BIT(0) //if 1: low battery


/////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
#if (UART_PRINT_DEBUG_ENABLE)
	#define DEBUG_INFO_TX_PIN           		GPIO_PA0
	#define PULL_WAKEUP_SRC_PA0         		PM_PIN_PULLUP_10K
	#define PA0_OUTPUT_ENABLE         			1
	#define PA0_DATA_OUT                     	1 //must
#endif





#include "../common/default_config.h"
