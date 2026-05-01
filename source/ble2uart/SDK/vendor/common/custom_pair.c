/********************************************************************************************************
 * @file    custom_pair.c
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
#include "custom_pair.h"



/**********************************************************************************
				// proc user  PAIR and UNPAIR
**********************************************************************************/

u32 flash_sector_custom_pairing = FLASH_ADR_SMP_PAIRING_512K_FLASH;


man_pair_t blm_manPair;

/* define pair slave max num,
   if exceed this max num, two methods to process new slave pairing
   method 1: overwrite the oldest one(telink demo use this method)
   method 2: not allow pairing unless unfair happened  */
#define	USER_PAIR_ACL_PERIPHR_MAX_NUM       4  //telink demo use max 4, you can change this value


typedef struct {
	u8 bond_mark;
	u8 adr_type;
	u8 address[6];
} macAddr_t;


typedef struct {
	u32 bond_flash_idx[USER_PAIR_ACL_PERIPHR_MAX_NUM];  //mark paired slave mac address in flash
	macAddr_t bond_device[USER_PAIR_ACL_PERIPHR_MAX_NUM];  //macAddr_t already defined in ble stack
	u8 curNum;
} user_salveMac_t;



/* flash erase strategy:
   never erase flash when dongle is working, for flash sector erase takes too much time(20-100 ms)
   this will lead to timing err
   so we only erase flash at initiation,  and with mark 0x00 for no use symbol
 */

#define ADR_BOND_MARK 		0x5A
#define ADR_ERASE_MARK		0x00
/* flash stored mac address struct:
   every 8 bytes is a address area: first one is mark, second no use, third - eighth is 6 byte address
   	   0     1           2 - 7
   | mark |     |    mac_address     |
   mark = 0xff, current area is invalid, pair info end
   mark = 0x01, current area is valid, load the following mac_address,
   mark = 0x00, current area is invalid (previous valid address is erased)
 */

int		user_bond_slave_flash_cfg_idx;  //new mac address stored flash idx


user_salveMac_t user_tbl_slaveMac;  //slave mac bond table

/**
 * @brief   Delete slave MAC by index.
 *          !!! Note: only internal use
 * @param   index
 * @return  none.
 */
void user_tbl_slave_mac_delete_by_index(int index)  //remove the oldest adr in slave mac table
{
	//erase the oldest with ERASE_MARK
	u8 delete_mark = ADR_ERASE_MARK;
	flash_write_page (flash_sector_custom_pairing + user_tbl_slaveMac.bond_flash_idx[index], 1, &delete_mark);

	for(int i=index; i<user_tbl_slaveMac.curNum - 1; i++){ 	//move data
		user_tbl_slaveMac.bond_flash_idx[i] = user_tbl_slaveMac.bond_flash_idx[i+1];
		memcpy( (u8 *)&user_tbl_slaveMac.bond_device[i], (u8 *)&user_tbl_slaveMac.bond_device[i+1], 8 );
	}

	user_tbl_slaveMac.curNum --;
}

/**
 * @brief     Store bonding info to flash.
 * @param[in] adr_type   address type
 * @param[in] adr        Pointer point to address buffer.
 * @return    none.
 */
int user_tbl_slave_mac_add(u8 adr_type, u8 *adr)  //add new mac address to table
{
	u8 add_new = 0;
	if(user_tbl_slaveMac.curNum >= USER_PAIR_ACL_PERIPHR_MAX_NUM){ //salve mac table is full
		//slave mac max, telink use  method 1: overwrite the oldest one
		user_tbl_slave_mac_delete_by_index(0);  //overwrite, delete index 0 (oldest) of table
		add_new = 1;  //add new
	}
	else{//slave mac table not full
		add_new = 1;
	}

	if(add_new){

		user_bond_slave_flash_cfg_idx += 8;  //inc flash idx to get the new 8 bytes area

		if( user_bond_slave_flash_cfg_idx >= FLASH_CUSTOM_PAIRING_MAX_SIZE ){ 		 //pairing information exceed Flash sector 4K size
			return 0;  //add Fail
		}

		user_tbl_slaveMac.bond_device[user_tbl_slaveMac.curNum].bond_mark = ADR_BOND_MARK;
		user_tbl_slaveMac.bond_device[user_tbl_slaveMac.curNum].adr_type = adr_type;
		memcpy(user_tbl_slaveMac.bond_device[user_tbl_slaveMac.curNum].address, adr, 6);

		flash_write_page (flash_sector_custom_pairing + user_bond_slave_flash_cfg_idx, 8, (u8 *)&user_tbl_slaveMac.bond_device[user_tbl_slaveMac.curNum] );

		user_tbl_slaveMac.bond_flash_idx[user_tbl_slaveMac.curNum] = user_bond_slave_flash_cfg_idx;  //mark flash idx
		user_tbl_slaveMac.curNum++;

		return 1;  //add OK
	}

	return 0;
}

/**
 * @brief     search mac address in the bond slave mac table:
 *            when slave paired with dongle, add this addr to table
 *            re_poweron slave, dongle will search if this AdvA in slave adv pkt is in this table
 *            if in, it will connect slave directly
 *             this function must in ramcode
 * @param[in]  adr_type   address type
 * @param[in]  adr        Pointer point to address buffer.
 * @return     0:      invalid index
 *             others valid index
 */
int user_tbl_slave_mac_search(u8 adr_type, u8 * adr)
{
	for(int i=0; i< user_tbl_slaveMac.curNum; i++){
		if( user_tbl_slaveMac.bond_device[i].adr_type == adr_type && \
			!memcmp(user_tbl_slaveMac.bond_device[i].address ,adr, 6)){  //match

			return (i+1);  //return index+1( 1 - USER_PAIR_ACL_PERIPHR_MAX_NUM)
		}
	}

	return 0;
}

/**
 * @brief      Delete bonding info.
 * @param[in]  adr_type   address type
 * @param[in]  adr        Pointer point to address buffer.
 * @return     1: delete ok
 *             0: no find
 */
int user_tbl_slave_mac_delete_by_adr(u8 adr_type, u8 *adr)  //remove adr from slave mac table by adr
{
	for(int i=0;i<user_tbl_slaveMac.curNum;i++){
		if( user_tbl_slaveMac.bond_device[i].adr_type == adr_type && \
			!memcmp(user_tbl_slaveMac.bond_device[i].address ,adr, 6)){  //match

			//erase the match adr
			u8 delete_mark = ADR_ERASE_MARK;
			flash_write_page (flash_sector_custom_pairing + user_tbl_slaveMac.bond_flash_idx[i], 1, &delete_mark);

			for(int j=i; j< user_tbl_slaveMac.curNum - 1;j++){ //move data
				user_tbl_slaveMac.bond_flash_idx[j] = user_tbl_slaveMac.bond_flash_idx[j+1];
				memcpy((u8 *)&user_tbl_slaveMac.bond_device[j], (u8 *)&user_tbl_slaveMac.bond_device[j+1], 8);
			}

			user_tbl_slaveMac.curNum --;
			return 1; //delete OK
		}
	}

	return 0;
}



/**
 * @brief      Delete all device bonding info.
 * @param      none.
 * @return     none.
 */
void user_tbl_slave_mac_delete_all(void)  //delete all the  adr in slave mac table
{
	u8 delete_mark = ADR_ERASE_MARK;
	for(int i=0; i< user_tbl_slaveMac.curNum; i++){
		flash_write_page (flash_sector_custom_pairing + user_tbl_slaveMac.bond_flash_idx[i], 1, &delete_mark);
		memset( (u8 *)&user_tbl_slaveMac.bond_device[i], 0, 8);
		//user_tbl_slaveMac.bond_flash_idx[i] = 0;  //do not  concern
	}

	user_tbl_slaveMac.curNum = 0;
}




u8 adbg_flash_clean;
#define DBG_FLASH_CLEAN   0
//when flash stored too many addr, it may exceed a sector max(4096), so we must clean this sector
// and rewrite the valid addr at the beginning of the sector(0x11000)

/**
 * @brief      clean pair flash
 * @param      none.
 * @return     none.
 */
void	user_bond_slave_flash_clean (void)
{
#if	DBG_FLASH_CLEAN
	if (user_bond_slave_flash_cfg_idx < 8*8)  //debug, max 8 area, then clean flash
#else
	if (user_bond_slave_flash_cfg_idx < (FLASH_CUSTOM_PAIRING_MAX_SIZE>>1) )  //max 2048/8 = 256,rest available sector is big enough, no need clean
#endif
	{
		return;
	}

	adbg_flash_clean = 1;

	flash_erase_sector (flash_sector_custom_pairing);

	user_bond_slave_flash_cfg_idx = -8;  //init value for no bond slave mac

	//rewrite bond table at the beginning of 0x11000
	for(int i=0; i< user_tbl_slaveMac.curNum; i++){
		//u8 add_mark = ADR_BOND_MARK;

		user_bond_slave_flash_cfg_idx += 8;  //inc flash idx to get the new 8 bytes area
		flash_write_page (flash_sector_custom_pairing + user_bond_slave_flash_cfg_idx, 8, (u8*)&user_tbl_slaveMac.bond_device[i] );

		user_tbl_slaveMac.bond_flash_idx[i] = user_bond_slave_flash_cfg_idx;  //update flash idx
	}
}

/**
 * @brief      initialize pair flash.
 * @param      none.
 * @return     none.
 */
void	user_master_host_pairing_flash_init(void)
{
	u8 flag;
	for (user_bond_slave_flash_cfg_idx=0; user_bond_slave_flash_cfg_idx<4096; user_bond_slave_flash_cfg_idx+=8)
	{ //traversing 8 bytes area in sector 0x11000 to find all the valid slave mac adr
		flash_read_page(flash_sector_custom_pairing + user_bond_slave_flash_cfg_idx, 1, &flag);
		if( flag == ADR_BOND_MARK ){  //valid adr
			if(user_tbl_slaveMac.curNum < USER_PAIR_ACL_PERIPHR_MAX_NUM){
				user_tbl_slaveMac.bond_flash_idx[user_tbl_slaveMac.curNum] = user_bond_slave_flash_cfg_idx;
				flash_read_page (flash_sector_custom_pairing + user_bond_slave_flash_cfg_idx, 8, (u8 *)&user_tbl_slaveMac.bond_device[user_tbl_slaveMac.curNum] );
				user_tbl_slaveMac.curNum ++;
			}
			else{ //slave mac in flash more than max, we think it's code bug
				irq_disable();
				while(1);
			}
		}
		else if (flag == 0xff)	//end
		{
			break;
		}
	}

	user_bond_slave_flash_cfg_idx -= 8; //back to the newest addr 8 bytes area flash ixd(if no valid addr, will be -8)

	user_bond_slave_flash_clean ();
}

/**
 * @brief   Pair management initialization for master.
 * @param   none.
 * @return  none.
 */
void user_master_host_pairing_management_init(void)
{

	#if (ACL_CENTRAL_CUSTOM_PAIR_ENABLE && ACL_CENTRAL_SMP_ENABLE)
		#error "can not use custom pair when ACL Central SMP enable !!!"
	#endif

	/* when custom pair enable, ACL Central SMP not disable, so using flash address for SMP storage is OK */
	#if (FLASH_ZB25WD40B_SUPPORT_EN || FLASH_GD25LD40C_SUPPORT_EN || FLASH_GD25LD40E_SUPPORT_EN || FLASH_P25D40SU_SUPPORT_EN)	//512K
		if(blc_flash_capacity == FLASH_SIZE_512K){
			flash_sector_custom_pairing = FLASH_ADR_SMP_PAIRING_512K_FLASH;
		}
	#endif
	#if (FLASH_ZB25WD80B_SUPPORT_EN || FLASH_GD25LD80C_SUPPORT_EN || FLASH_GD25LD80E_SUPPORT_EN || FLASH_P25Q80SU_SUPPORT_EN)	//1M
		else if(blc_flash_capacity == FLASH_SIZE_1M){
			flash_sector_custom_pairing = FLASH_ADR_SMP_PAIRING_1M_FLASH;
		}
	#endif
	#if (FLASH_P25Q16SU_SUPPORT_EN)	//2M
		else if(blc_flash_capacity == FLASH_SIZE_2M){
			flash_sector_custom_pairing = FLASH_ADR_SMP_PAIRING_2M_FLASH;
		}
	#endif

	user_master_host_pairing_flash_init();
}
