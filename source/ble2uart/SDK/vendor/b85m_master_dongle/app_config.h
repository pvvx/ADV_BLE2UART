/********************************************************************************************************
 * @file	app_config.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions
 *              in binary form must reproduce the above copyright notice, this list of
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *
 *              3. Neither the name of TELINK, nor the names of its contributors may be
 *              used to endorse or promote products derived from this software without
 *              specific prior written permission.
 *
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or
 *              relating to such deletion(s), modification(s) or alteration(s).
 *
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************************************/
#pragma once

#include "config.h"

#define	CONN_MAX_NUM_CONFIG							CONN_MAX_NUM_M4_S0
#define MASTER_MAX_NUM								4
#define SLAVE_MAX_NUM								0


///////////////////////// Feature Configuration////////////////////////////////////////////////
/*
 *	Note: If change IC type, need to confirm the flash size.
	IC Type			Flash Size
	8253			512 KB
	8258			512 KB / 1 MB
	8273			512 KB
	8278			1 MB
 */
#if(CHIP_TYPE == CHIP_TYPE_825x)
	/* Note: if change to TLSR8258F1K, need pay attention to select the correct flash size */
	#define	FLASH_SIZE_CONFIG		   				FLASH_SIZE_512K  //very important, user need confirm !!!
#elif(CHIP_TYPE == CHIP_TYPE_827x)
	/* Note: if change to TLSR8273, need pay attention to select the correct flash size */
	#define	FLASH_SIZE_CONFIG		   				FLASH_SIZE_1M    //very important, user need confirm !!!
#else
#endif

#define BLE_MASTER_SMP_ENABLE						1	//1 for smp,  0 no security
#define BLE_MASTER_SIMPLE_SDP_ENABLE				1	//simple service discovery for BLE master
#define APPLICATION_DONGLE							1   //0
#define BLE_SINGLE_MASTER_EN						0	//only for single connect sdk

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#if(CHIP_TYPE == CHIP_TYPE_825x)
	#define HW_C1T139A30_V1_2_EVB					1
	#define HW_C1T139A3_V2_0A_DONGLE				2
	#define HARDWARE_BOARD_SELECT					HW_C1T139A30_V1_2_EVB//HW_C1T213A20_V1_0_80_EVB
#else
	#define HW_C1T197A30_V1_1_EVB					1
	#define HW_C1T201A3_V1_0_DONGLE					2
	#define HARDWARE_BOARD_SELECT					HW_C1T197A30_V1_1_EVB//HW_C1T213A20_V1_0_80_EVB
#endif


#if(HARDWARE_BOARD_SELECT == HW_C1T139A30_V1_2_EVB || HARDWARE_BOARD_SELECT == HW_C1T197A30_V1_1_EVB)
	#define	UI_KEYBOARD_ENABLE						1
	#define	UI_BUTTON_ENABLE						0
#elif(HARDWARE_BOARD_SELECT == HW_C1T139A3_V2_0A_DONGLE || HARDWARE_BOARD_SELECT==HW_C1T201A3_V1_0_DONGLE)
	#define	UI_KEYBOARD_ENABLE						0
	#define	UI_BUTTON_ENABLE						1
#endif

#define UI_LED_ENABLE								1

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE							0
#define UART_PRINT_DEBUG_ENABLE                     0  //printf

#define MASTER_CONNECT_SLAVE_MAC_FILTER_EN			0





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
	#define	MIC_ENOCDER_ENABLE		0

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










#if (UI_KEYBOARD_ENABLE)   // if test pure power, kyeScan GPIO setting all disabled
		//---------------  KeyMatrix PB2/PB3/PB4/PB5 -----------------------------
		#define	MATRIX_ROW_PULL					PM_PIN_PULLDOWN_100K
		#define	MATRIX_COL_PULL					PM_PIN_PULLUP_10K

		#define	KB_LINE_HIGH_VALID				0   //drive pin output 0 when keyscan, scanpin read 0 is valid

		#define BTN_PAIR						0x01
		#define BTN_UNPAIR						0x02

		#define	CR_VOL_UP						0xf0  ////
		#define	CR_VOL_DN						0xf1


		/**
		 *  @brief  Normal keyboard map
		 */
		#define		KB_MAP_NORMAL	{	{CR_VOL_UP,	CR_VOL_DN} ,	 \
										{BTN_PAIR,	BTN_UNPAIR},     }



		//////////////////// KEY CONFIG (EVK board) ///////////////////////////
		#define  KB_DRIVE_PINS  		{GPIO_PB4, GPIO_PB5}
		#define  KB_SCAN_PINS  			{GPIO_PB2, GPIO_PB3}

		//drive pin as gpio
		#define	PB4_FUNC				AS_GPIO
		#define	PB5_FUNC				AS_GPIO

		//drive pin need 100K pulldown
		#define	PULL_WAKEUP_SRC_PB4		MATRIX_ROW_PULL
		#define	PULL_WAKEUP_SRC_PB5		MATRIX_ROW_PULL

		//drive pin open input to read gpio wakeup level
		#define PB4_INPUT_ENABLE		1
		#define PB5_INPUT_ENABLE		1

		//scan pin as gpio
		#define	PB2_FUNC				AS_GPIO
		#define	PB3_FUNC				AS_GPIO

		//scan  pin need 10K pullup
		#define	PULL_WAKEUP_SRC_PB2		MATRIX_COL_PULL
		#define	PULL_WAKEUP_SRC_PB3		MATRIX_COL_PULL

		//scan pin open input to read gpio level
		#define PB2_INPUT_ENABLE		1
		#define PB3_INPUT_ENABLE		1
#endif

#if (UI_BUTTON_ENABLE)
	//---------------  Button ----------------------------------
	#define	SW1_GPIO				GPIO_PD6
	#define	SW2_GPIO				GPIO_PD5
	#define PD6_FUNC				AS_GPIO
	#define PD5_FUNC				AS_GPIO
	#define PD6_INPUT_ENABLE		1
	#define PD5_INPUT_ENABLE		1
	#define PULL_WAKEUP_SRC_PD6     PM_PIN_PULLUP_10K
	#define PULL_WAKEUP_SRC_PD5     PM_PIN_PULLUP_10K
#endif


#if UI_LED_ENABLE
	#if(HARDWARE_BOARD_SELECT == HW_C1T139A30_V1_2_EVB || HARDWARE_BOARD_SELECT == HW_C1T197A30_V1_1_EVB)
		#define	GPIO_LED_BLUE			GPIO_PD2
		#define	GPIO_LED_GREEN			GPIO_PD3
		#define	GPIO_LED_WHITE			GPIO_PD4
		#define	GPIO_LED_RED			GPIO_PD5


		#define PD2_FUNC				AS_GPIO
		#define PD3_FUNC				AS_GPIO
		#define PD4_FUNC				AS_GPIO
		#define PD5_FUNC				AS_GPIO

		#define	PD2_OUTPUT_ENABLE		1
		#define	PD3_OUTPUT_ENABLE		1
		#define PD4_OUTPUT_ENABLE		1
		#define	PD5_OUTPUT_ENABLE		1
	#elif(HARDWARE_BOARD_SELECT == HW_C1T139A3_V2_0A_DONGLE || HARDWARE_BOARD_SELECT == HW_C1T201A3_V1_0_DONGLE)
		#define GPIO_LED_GREEN			GPIO_PA2
		#define GPIO_LED_RED			GPIO_PA3
		#define GPIO_LED_YELLOW     	GPIO_PA4
		#define GPIO_LED_BLUE			GPIO_PB0
		#define GPIO_LED_WHITE			GPIO_PB1


		#define PA2_FUNC				AS_GPIO
		#define PA3_FUNC				AS_GPIO
		#define PA4_FUNC				AS_GPIO
		#define PB0_FUNC				AS_GPIO
		#define PB1_FUNC				AS_GPIO

		#define	PA2_OUTPUT_ENABLE		1
		#define	PA3_OUTPUT_ENABLE		1
		#define PA4_OUTPUT_ENABLE		1
		#define	PB0_OUTPUT_ENABLE		1
		#define	PB1_OUTPUT_ENABLE		1
	#endif

	#define LED_ON_LEVAL 				1 		//gpio output high voltage to turn on led
#endif




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



/////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
#if (UART_PRINT_DEBUG_ENABLE)
	//the baud rate should not bigger than 1M(system timer clock is constant 16M)
	#define PRINT_BAUD_RATE             		1000000//1M baud rate,should Not bigger than 1Mb/s
	#define DEBUG_INFO_TX_PIN           		GPIO_PA0
	#define PULL_WAKEUP_SRC_PA0         		PM_PIN_PULLUP_10K
	#define PA0_OUTPUT_ENABLE         			1
	#define PA0_DATA_OUT                     	1 //must
	#include "application/print/u_printf.h"
#endif





#if( DEBUG_GPIO_ENABLE && (HARDWARE_BOARD_SELECT == HW_C1T139A30_V1_2_EVB || HARDWARE_BOARD_SELECT == HW_C1T197A30_V1_1_EVB))

	#define GPIO_CHN0							GPIO_PD0
	#define GPIO_CHN1							GPIO_PD1
	#define GPIO_CHN2							GPIO_PD6
    #define GPIO_CHN3							GPIO_PD7
	#define GPIO_CHN4							GPIO_PA2
	#define GPIO_CHN5							GPIO_PA3
	#define GPIO_CHN6							GPIO_PA4
	#define GPIO_CHN7							GPIO_PB0
	#define GPIO_CHN8							GPIO_PC0
	#define GPIO_CHN9							GPIO_PC1
	#define GPIO_CHN10							GPIO_PC2
	#define GPIO_CHN11							GPIO_PC3
	#define GPIO_CHN12							GPIO_PC4
	#define GPIO_CHN13							GPIO_PC5
	#define GPIO_CHN14							GPIO_PC6
	#define GPIO_CHN15							GPIO_PC7

	#define PD0_OUTPUT_ENABLE					1
	#define PD1_OUTPUT_ENABLE					1
	#define PD6_OUTPUT_ENABLE					1
	#define PD7_OUTPUT_ENABLE					1
	#define PA2_OUTPUT_ENABLE					1
	#define PA3_OUTPUT_ENABLE					1
	#define PA4_OUTPUT_ENABLE					1
	#define PB0_OUTPUT_ENABLE					1
	#define PC0_OUTPUT_ENABLE					1
	#define PC1_OUTPUT_ENABLE					1
	#define PC2_OUTPUT_ENABLE					1
	#define PC3_OUTPUT_ENABLE					1
	#define PC4_OUTPUT_ENABLE					1
	#define PC5_OUTPUT_ENABLE					1
	#define PC6_OUTPUT_ENABLE					1
	#define PC7_OUTPUT_ENABLE					1

#endif  //end of DEBUG_GPIO_ENABLE





#include "../common/default_config.h"
