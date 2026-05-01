/********************************************************************************************************
 * @file    ram_check.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "ram_check.h"


/**
 * @brief		This is detect initialization function.
 * @param[in]	none
 * @return      none
 */
void blc_app_ram_check(u32 ramStartAddr, u32 ramEndAddr, u32 flashStartAddr)
{
	//Check whether the contents of flash and RAM are consistent
	u32 err_ramcode = 0;
	u8 check_buf[256];

	u32 ram_size_mod = (ramEndAddr - ramStartAddr) % 256;

	while(ramStartAddr < (ramEndAddr - ram_size_mod))
	{
		flash_read_page(flashStartAddr, 256, check_buf);

		if(memcmp((u8*)(ramStartAddr), check_buf, 256)){
			err_ramcode = 1;
		}
		ramStartAddr += 256;
		flashStartAddr += 256;
	}
	if(ram_size_mod)
	{
		flash_read_page(flashStartAddr, ram_size_mod, check_buf);

		if(memcmp((u8*)(ramStartAddr), check_buf, ram_size_mod)){
			err_ramcode = 1;
		}
	}

	if(err_ramcode){
		start_reboot();
	}
}

