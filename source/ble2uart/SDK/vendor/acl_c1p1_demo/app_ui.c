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

#include "app.h"
#include "app_att.h"
#include "app_ui.h"

#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"



int	central_pairing_enable = 0;
int central_unpair_enable = 0;
extern u8 		ota_is_working;

_attribute_ble_data_retention_ int central_disconnect_connhandle;   //mark the central connection which is in un_pair disconnection flow




#if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_	int 	key_not_released;


#define CONSUMER_KEY   	   			1
#define KEYBOARD_KEY   	   			2
#define PAIR_UNPAIR_KEY   	   		3

_attribute_ble_data_retention_	u8 		key_type;

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
			key_type = CONSUMER_KEY;
			u16 consumer_key;
			if(key0 == CR_VOL_UP){  	//volume up
				consumer_key = MKEY_VOL_UP;
				tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]UI send Vol+", 0, 0);
			}
			else if(key0 == CR_VOL_DN){ //volume down
				consumer_key = MKEY_VOL_DN;
				tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]UI send Vol-", 0, 0);
			}


			/*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, when Vol+/Vol- key pressed, we
			send media key "Vol+" or "Vol-" to master for all slave in connection.
			For users, you should known that this is not a good method, you should manage your device and GATT data transfer
			according to  conn_dev_list[]
			 * */
			for(int i=ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++){ //slave index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
				if(conn_dev_list[i].conn_state){
					blc_gatt_pushHandleValueNotify (conn_dev_list[i].conn_handle, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
				}
			}
		}
		else{
			key_type = PAIR_UNPAIR_KEY;

			if(key0 == BTN_PAIR)   //Manual pair triggered by Key Press
			{
				central_pairing_enable = 1;
				tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]UI PAIR begin", 0, 0);
			}
			else if(key0 == BTN_UNPAIR) //Manual un_pair triggered by Key Press
			{
				/*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, only one "un_pair" key is
				 available. When "un_pair" key pressed, we will choose and un_pair one device in connection state */
				if(acl_conn_central_num){ //at least 1 master connection exist

					if(!central_disconnect_connhandle){  //if one master un_pair disconnection flow not finish, here new un_pair not accepted

						/* choose one master connection to disconnect */
						for(int i=0; i < ACL_CENTRAL_MAX_NUM; i++){ //slave index is from 0 to "ACL_CENTRAL_MAX_NUM - 1"
							if(conn_dev_list[i].conn_state){
								central_unpair_enable = conn_dev_list[i].conn_handle;  //mark connHandle on central_unpair_enable
								tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]UI UNPAIR", &central_unpair_enable, 2);
								break;
							}
						}
					}
				}
			}

		}

	}
	else   //kb_event.cnt == 0,  key release
	{
		key_not_released = 0;
		if(key_type == CONSUMER_KEY)
		{
			u16 consumer_key = 0;
			//Here is just Telink Demonstration effect. for all slave in connection, send release for previous "Vol+" or "Vol-" to master
			for(int i=ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++){ //slave index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
				if(conn_dev_list[i].conn_state){
					blc_gatt_pushHandleValueNotify ( conn_dev_list[i].conn_handle, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
				}
			}
		}
		else if(key_type == KEYBOARD_KEY)
		{

		}
		else if(key_type == PAIR_UNPAIR_KEY){
			if(central_pairing_enable){
				central_pairing_enable = 0;
				tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]UI PAIR end", 0, 0);
			}

			if(central_unpair_enable){
				central_unpair_enable = 0;
			}
		}
	}


}


#define GPIO_WAKEUP_KEYPROC_CNT				3
_attribute_ble_data_retention_	static u32 keyScanTick = 0;
_attribute_ble_data_retention_	static int gpioWakeup_keyProc_cnt = 0;

/**
 * @brief      keyboard task handler
 * @param[in]  e    - event type
 * @param[in]  p    - Pointer point to event parameter.
 * @param[in]  n    - the length of event parameter.
 * @return     none.
 */
void proc_keyboard(u8 e, u8 *p, int n)
{
	//when key press GPIO wake_up sleep, process key_scan at least GPIO_WAKEUP_KEYPROC_CNT times
	if(e == BLT_EV_FLAG_GPIO_EARLY_WAKEUP){
		gpioWakeup_keyProc_cnt = GPIO_WAKEUP_KEYPROC_CNT;
	}
	else if(gpioWakeup_keyProc_cnt){
		gpioWakeup_keyProc_cnt --;
	}

	if(gpioWakeup_keyProc_cnt || clock_time_exceed(keyScanTick, 10 * 1000)){ //keyScan interval: 10mS
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




/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_ENTER"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void  app_set_kb_wakeup (u8 e, u8 *p, int n)
{
#if (BLE_APP_PM_ENABLE)
	/* suspend time > 50ms.add GPIO wake_up */
	if(((u32)(blc_pm_getWakeupSystemTick() - clock_time())) > 100 * SYSTEM_TIMER_TICK_1MS){
		blc_pm_setWakeupSource(PM_WAKEUP_PAD);  //GPIO PAD wake_up
	}
#endif
}


/**
 * @brief      keyboard initialization
 * @param[in]  none
 * @return     none.
 */
void keyboard_init(void)
{
#if (BLE_APP_PM_ENABLE)
	/////////// keyboard GPIO wakeup init ////////
	u32 pin[] = KB_DRIVE_PINS;
	for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++){
		cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high level wakeup deepsleep
	}

	blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_SLEEP_ENTER, &app_set_kb_wakeup);
	blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboard);
#endif
}



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


				#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
					// delete ATT handle storage on flash
					dev_char_info_delete_peer_att_handle_by_peer_mac(dev_char_info->peer_adrType, dev_char_info->peer_addr);
				#endif


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



#endif   //end of UI_KEYBOARD_ENABLE


/**
 * @brief      this function is used to register the function for OTA start.
 * @param[in]  none
 * @return     none
 */
void app_enter_ota_mode(void)
{
	ota_is_working = 1;

	#if(UI_LED_ENABLE)  //this is only for debug
		gpio_write(GPIO_LED_BLUE, 1);
	#endif
}







