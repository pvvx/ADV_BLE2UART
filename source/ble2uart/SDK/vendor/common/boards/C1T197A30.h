/********************************************************************************************************
 * @file    C1T197A30.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifndef C1T197A30_H_
#define C1T197A30_H_

#include "common/config/user_config.h"

#if(BOARD_SELECT == BOARD_827X_EVK_C1T197A30)



/**
 *  @brief  Keyboard Configuration
 */
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



/**
 *  @brief  LED Configuration
 */
#if UI_LED_ENABLE
	/**
	 *  @brief  Definition gpio for led
	 */
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

	#define LED_ON_LEVAL 			1 		//gpio output high voltage to turn on led

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
	#define GPIO_CHN10							GPIO_PB6
	#define GPIO_CHN11							GPIO_PB7
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
	#define PB6_OUTPUT_ENABLE					1
	#define PB7_OUTPUT_ENABLE					1
	#define PC4_OUTPUT_ENABLE					1
	#define PC5_OUTPUT_ENABLE					1
	#define PC6_OUTPUT_ENABLE					1
	#define PC7_OUTPUT_ENABLE					1

#endif  //end of DEBUG_GPIO_ENABLE

/**
 *  @brief  Battery_check Configuration
 */
#if (APP_BATT_CHECK_ENABLE)
	#if 0
		//use VBAT(only 827x) , then adc measure this VBAT voltage
		#define ADC_INPUT_PCHN					VBAT    //corresponding  ADC_InputPchTypeDef in adc.h
	#else
		//Telink device: you must choose one gpio with adc function to output high level(voltage will equal to vbat), then use adc to measure high level voltage
		//use PB7 output high level, then adc measure this high level voltage
		#define GPIO_VBAT_DETECT				GPIO_PB7
		#define PB7_FUNC						AS_GPIO
		#define PB7_INPUT_ENABLE				0
		#define ADC_INPUT_PCHN					B7P    //corresponding  ADC_InputPchTypeDef in adc.h
	#endif
#endif


#endif /* BOARD_SELECT == BOARD_827X_EVK_C1T197A30 */

#endif /* C1T197A30_H_ */
