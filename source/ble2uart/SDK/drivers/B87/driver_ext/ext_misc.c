/********************************************************************************************************
 * @file    ext_misc.c
 *
 * @brief   This is the header file for B91
 *
 * @author  BLE Group
 * @date    2020
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
#include "drivers/B87/driver.h"
#include "ext_misc.h"
/*********************************************************** STIMER START********************************************/
_attribute_ram_code_ void systimer_set_irq_capture(unsigned int sys_timer_tick)//8278 for save time,need to place in ram.
{
	if( (unsigned int)( sys_timer_tick  - (reg_system_tick + 7*CLOCK_16M_SYS_TIMER_CLK_1US )) > BIT(30) )//63us - 75us in flash / 3.5us in ram
	{
		reg_system_tick_irq = reg_system_tick + 12*CLOCK_16M_SYS_TIMER_CLK_1US; ///320 = 0x140, so not &0xfffffff8
	}
	else{
		reg_system_tick_irq = sys_timer_tick & 0xfffffff8;
	}
}
/*********************************************************** STIMER END********************************************/


/*********************************************************** FLASH START********************************************/
_attribute_data_retention_   _attribute_aligned_(4)	Flash_CapacityDef	flash_capacity;

void flash_set_capacity(Flash_CapacityDef flash_cap)
{
	flash_capacity = flash_cap;
}

Flash_CapacityDef flash_get_capacity(void)
{
	return flash_capacity;
}
/*********************************************************** FLASH END********************************************/

/*********************************************************** RANDOM START********************************************/
/**
 * @brief      This function performs to get a serial of random number.
 * @param[in]  len- the length of random number
 * @param[in]  data - the first address of buffer store random number in
 * @return     the result of a serial of random number..
 */
void generateRandomNum(int len, unsigned char *data)
{
	int i;
	unsigned int randNums = 0;
    /* if len is odd */
	for (i=0; i<len; i++ ) {
		if( (i & 3) == 0 ){
			randNums = rand();
		}

		data[i] = randNums & 0xff;
		randNums >>=8;
	}
}
/*********************************************************** RANDOM END********************************************/
