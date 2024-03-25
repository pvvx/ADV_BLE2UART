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

#include"config.h"
#include "../feature_config.h"

#if (FEATURE_TEST_MODE == TEST_OTA)


#define MASTER_MAX_NUM								1
#define SLAVE_MAX_NUM								1
#define MASTER_SLAVE_MAX_NUM						(MASTER_MAX_NUM + SLAVE_MAX_NUM)


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

#define BLE_SLAVE_SMP_ENABLE						1   //1 for smp,  0 no security
#define BLE_MASTER_SMP_ENABLE						0   //1 for smp,  0 no security

#define BLE_OTA_SERVER_ENABLE						1
#define BLE_OTA_CLIENT_ENABLE						1


#if (BLE_OTA_CLIENT_ENABLE)
	#define OTA_LEGACY_PROTOCOL						0  //0: OTA extended protocol; 1: OTA legacy protocol
#endif


#define OTA_SERVER_SUPPORT_BIG_PDU_ENABLE			0  //receive big PDU from BLE master
#define OTA_CLIENT_SUPPORT_BIG_PDU_ENABLE			0  //send big PDU to BLE slave

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE								1
#define	UI_KEYBOARD_ENABLE							1

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE							0
#define UART_PRINT_DEBUG_ENABLE                     0  //printf
#define MASTER_CONNECT_SLAVE_MAC_FILTER_EN			0


#define APP_DEFAULT_HID_BATTERY_OTA_ATTRIBUTE_TABLE			1




#if (UI_KEYBOARD_ENABLE)   // if test pure power, kyeScan GPIO setting all disabled
		//---------------  KeyMatrix PB2/PB3/PB4/PB5 -----------------------------
		#define	MATRIX_ROW_PULL					PM_PIN_PULLDOWN_100K
		#define	MATRIX_COL_PULL					PM_PIN_PULLUP_10K

		#define	KB_LINE_HIGH_VALID				0   //drive pin output 0 when keyscan, scanpin read 0 is valid

		#define BTN_PAIR						0x01
		#define BTN_UNPAIR						0x02
		#define	BTN_OTA_1						0x03
		#define	BTN_OTA_2						0x04

		#define	KB_MAP_NORMAL		{	{BTN_OTA_1,		BTN_OTA_2},	 \
										{BTN_PAIR,		BTN_UNPAIR}, }


		//////////////////// KEY CONFIG (EVK board) ///////////////////////////
		#define  KB_DRIVE_PINS  		{GPIO_PB4, GPIO_PB5}
		#define  KB_SCAN_PINS   		{GPIO_PB2, GPIO_PB3}

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




#if UI_LED_ENABLE
	/**
	 *  @brief  Definition gpio for led
	 */
	#define	GPIO_LED_BLUE			GPIO_PD2
	#define	GPIO_LED_GREEN			GPIO_PD3
	#define	GPIO_LED_WHITE			GPIO_PD4
	#define	GPIO_LED_RED			GPIO_PD5

	#define LED_ON_LEVAL 						1 		//gpio output high voltage to turn on led

	#define PD2_FUNC				AS_GPIO
	#define PD3_FUNC				AS_GPIO
	#define PD4_FUNC				AS_GPIO
	#define PD5_FUNC				AS_GPIO

	#define	PD2_OUTPUT_ENABLE		1
	#define	PD3_OUTPUT_ENABLE		1
	#define PD4_OUTPUT_ENABLE		1
	#define	PD5_OUTPUT_ENABLE		1
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





#if(DEBUG_GPIO_ENABLE)


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





#include "../../common/default_config.h"

#endif
