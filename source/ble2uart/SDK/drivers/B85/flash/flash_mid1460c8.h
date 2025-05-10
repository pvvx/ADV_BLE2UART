/********************************************************************************************************
 * @file    flash_mid1460c8.h
 *
 * @brief   This is the header file for B85
 *
 * @author  Driver Group
 * @date    2018
 *
 * @par     Copyright (c) 2018, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef __MID1460C8_H__
#define __MID1460C8_H__

/*
 * @brief     MID = 0x1460c8 Flash include GD25LD80C/GD25LD80E.
 */


/**
 * @brief   define the section of the protected memory area which is read-only and unalterable.
 * @note    when data protection, only enumeration items in mid1460c8_lock_block_e can be selected,
 *          to ensure that the values returned by the interface flash_get_lock_block_mid1460c8 are in the mid1460c8_lock_block_e.
 */
typedef enum{
	FLASH_LOCK_NONE_MID1460C8		=	0x00,
	FLASH_LOCK_LOW_1016K_MID1460C8	=	0x04,	//000000h-0FDFFFh
	FLASH_LOCK_LOW_1008K_MID1460C8	=	0x08,	//000000h-0FBFFFh
	FLASH_LOCK_LOW_992K_MID1460C8	=	0x0c,	//000000h-0F7FFFh
	FLASH_LOCK_LOW_960K_MID1460C8	=	0x10,	//000000h-0EFFFFh
	FLASH_LOCK_LOW_896K_MID1460C8	=	0x14,	//000000h-0DFFFFh
	FLASH_LOCK_LOW_768K_MID1460C8	=	0x18,	//000000h-0BFFFFh
	FLASH_LOCK_ALL_1M_MID1460C8		=	0x1c,	//000000h-0FFFFFh
}mid1460c8_lock_block_e;

/**
 * @brief     the range of bits to be modified when writing status.
 */
typedef enum{
	FLASH_WRITE_STATUS_BP_MID1460C8	=	0x1c,
}mid1460c8_write_status_bit_e;


/**
 * @brief 		This function reads the status of flash.
 * @return 		the value of status.
 * @note        Attention: Before calling the FLASH function, please check the power supply voltage of the chip.
 *              Only if the detected voltage is greater than the safe voltage value, the FLASH function can be called.
 *              Taking into account the factors such as power supply fluctuations, the safe voltage value needs to be greater
 *              than the minimum chip operating voltage. For the specific value, please make a reasonable setting according
 *              to the specific application and hardware circuit.
 *
 *              Risk description: When the chip power supply voltage is relatively low, due to the unstable power supply,
 *              there may be a risk of error in the operation of the flash (especially for the write and erase operations.
 *              If an abnormality occurs, the firmware and user data may be rewritten, resulting in the final Product failure)
 */
unsigned char flash_read_status_mid1460c8(void);

/**
 * @brief 		This function write the status of flash.
 * @param[in]  	data	- the value of status.
 * @param[in]  	mask    - mid1460c8_write_status_bit_e
 * @return 		1: success, 0: error, 2: parameter error.
 * @note        Attention: Before calling the FLASH function, please check the power supply voltage of the chip.
 *              Only if the detected voltage is greater than the safe voltage value, the FLASH function can be called.
 *              Taking into account the factors such as power supply fluctuations, the safe voltage value needs to be greater
 *              than the minimum chip operating voltage. For the specific value, please make a reasonable setting according
 *              to the specific application and hardware circuit.
 *
 *              Risk description: When the chip power supply voltage is relatively low, due to the unstable power supply,
 *              there may be a risk of error in the operation of the flash (especially for the write and erase operations.
 *              If an abnormality occurs, the firmware and user data may be rewritten, resulting in the final Product failure)
 */
unsigned char flash_write_status_mid1460c8(unsigned char data, unsigned char mask);

/**
 * @brief 		This function serves to set the protection area of the flash.
 * @param[in]   data	- refer to the mid1460c8_lock_block_e protection area definition in the .h file.
 * @return 		1: success, 0: error, 2: parameter error.
 * @note        Attention: Before calling the FLASH function, please check the power supply voltage of the chip.
 *              Only if the detected voltage is greater than the safe voltage value, the FLASH function can be called.
 *              Taking into account the factors such as power supply fluctuations, the safe voltage value needs to be greater
 *              than the minimum chip operating voltage. For the specific value, please make a reasonable setting according
 *              to the specific application and hardware circuit.
 *
 *              Risk description: When the chip power supply voltage is relatively low, due to the unstable power supply,
 *              there may be a risk of error in the operation of the flash (especially for the write and erase operations.
 *              If an abnormality occurs, the firmware and user data may be rewritten, resulting in the final Product failure)
 */
unsigned char flash_lock_mid1460c8(unsigned int data);

/**
 * @brief 		This function serves to flash release protection.
 * @return 		1: success, 0: error, 2: parameter error.
 * @note        Attention: Before calling the FLASH function, please check the power supply voltage of the chip.
 *              Only if the detected voltage is greater than the safe voltage value, the FLASH function can be called.
 *              Taking into account the factors such as power supply fluctuations, the safe voltage value needs to be greater
 *              than the minimum chip operating voltage. For the specific value, please make a reasonable setting according
 *              to the specific application and hardware circuit.
 *
 *              Risk description: When the chip power supply voltage is relatively low, due to the unstable power supply,
 *              there may be a risk of error in the operation of the flash (especially for the write and erase operations.
 *              If an abnormality occurs, the firmware and user data may be rewritten, resulting in the final Product failure)
 */
unsigned char flash_unlock_mid1460c8(void);

/**
 * @brief 		This function serves to get the protection area of the flash.
 * @return 		mid1460c8_lock_block_e
 * @note        Attention: Before calling the FLASH function, please check the power supply voltage of the chip.
 *              Only if the detected voltage is greater than the safe voltage value, the FLASH function can be called.
 *              Taking into account the factors such as power supply fluctuations, the safe voltage value needs to be greater
 *              than the minimum chip operating voltage. For the specific value, please make a reasonable setting according
 *              to the specific application and hardware circuit.
 *
 *              Risk description: When the chip power supply voltage is relatively low, due to the unstable power supply,
 *              there may be a risk of error in the operation of the flash (especially for the write and erase operations.
 *              If an abnormality occurs, the firmware and user data may be rewritten, resulting in the final Product failure)
 */
unsigned int flash_get_lock_block_mid1460c8(void);
#endif

