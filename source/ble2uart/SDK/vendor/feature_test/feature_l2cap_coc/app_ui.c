/********************************************************************************************************
 * @file    app_ui.c
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

#include "app_config.h"
#include "app.h"
#include "../default_att.h"
#include "app_ui.h"

#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"

#include "app_l2cap_coc.h"


#if (FEATURE_TEST_MODE == TEST_L2CAP_COC)

int	central_pairing_enable = 0;
int central_unpair_enable = 0;

int central_disconnect_connhandle;   //mark the central connection which is in un_pair disconnection flow
















#if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_	int 	key_not_released;


#define CONSUMER_KEY   	   			1
#define KEYBOARD_KEY   	   			2
#define PAIR_UNPAIR_KEY   	   		3

_attribute_ble_data_retention_	u8 		key_type;
_attribute_ble_data_retention_  int     tst_flg;
/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{

	u8 key0 = kb_event.keycode[0];
//	u8 key_buf[8] = {0,0,0,0,0,0,0,0};

	key_not_released = 1;
	if (kb_event.cnt == 2)   //two key press
	{

	}
	else if(kb_event.cnt == 1)
	{
		if(key0 >= CR_VOL_UP )  //volume up/down
		{

			if(key0 == CR_VOL_UP){
				app_createLeCreditBasedConnect();
//				app_createCreditBasedConnect();
			}
			else if(key0 == CR_VOL_DN){
				app_sendCocData();
			}
		}
		else{
			key_type = PAIR_UNPAIR_KEY;

			if(key0 == BTN_PAIR)   //Manual pair triggered by Key Press
			{
				central_pairing_enable = 1;
			}
			else if(key0 == BTN_UNPAIR) //Manual un_pair triggered by Key Press
			{
				app_disconnCocConnect();
			}

		}

	}
	else   //kb_event.cnt == 0,  key release
	{
		key_not_released = 0;
		if(key_type == CONSUMER_KEY)
		{

		}
		else if(key_type == KEYBOARD_KEY)
		{

		}
		else if(key_type == PAIR_UNPAIR_KEY){

		}
	}


}



_attribute_ble_data_retention_		static u32 keyScanTick = 0;

/**
 * @brief      keyboard task handler
 * @param[in]  e    - event type
 * @param[in]  p    - Pointer point to event parameter.
 * @param[in]  n    - the length of event parameter.
 * @return     none.
 */
void proc_keyboard(u8 e, u8 *p, int n)
{
	if(clock_time_exceed(keyScanTick, 10 * 1000)){  //keyScan interval: 10mS
		keyScanTick = clock_time();
	}
	else{
		return;
	}

	kb_event.keycode[0] = 0;
	int det_key = kb_scan_key (0, 1);


	if (det_key){

		key_change_proc();
	}

}




#endif   //end of UI_KEYBOARD_ENABLE






















/**
 * @brief   BLE Unpair handle for master
 * @param   none.
 * @return  none.
 */
void proc_master_role_unpair(void)
{
	//terminate and un_pair process, Telink demonstration effect: triggered by "un_pair" key press
	if(central_unpair_enable){

		dev_char_info_t* dev_char_info = dev_char_info_search_by_connhandle(central_unpair_enable); //connHandle has marked on on central_unpair_enable

		if( dev_char_info ){ //un_pair device in still in connection state

			if(blc_ll_disconnect(central_unpair_enable, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS){

				central_disconnect_connhandle = central_unpair_enable; //mark conn_handle

				central_unpair_enable = 0;  //every "un_pair" key can only triggers one connection disconnect

				// delete this device information(mac_address and distributed keys...) on FLash
				#if (ACL_CENTRAL_SMP_ENABLE)
					blc_smp_deleteBondingSlaveInfo_by_PeerMacAddress(dev_char_info->peer_adrType, dev_char_info->peer_addr);
				#endif
			}

		}
		else{ //un_pair device can not find in device list, it's not connected now

			central_unpair_enable = 0;  //every "un_pair" key can only triggers one connection disconnect
		}

	}
}



#endif









