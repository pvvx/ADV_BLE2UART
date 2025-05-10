/********************************************************************************************************
 * @file    app_ui.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2020
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
#include "app_config.h"

#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"
#include "../common/blt_led.h"

#include "rc_ir.h"
#include "rc_ir_learn.h"
#include "app_audio.h"


int	central_pairing_enable = 0;
_attribute_data_retention_	u8 		user_key_mode;
int central_unpair_enable = 0;
u8      ir_hw_initialed = 0;   		//note: can not be retention variable
_attribute_data_retention_	int 	ir_not_released;
int central_disconnect_connhandle;   //mark the central connection which is in un_pair disconnection flow


extern	u32 	latest_user_event_tick;
extern  u8 		ota_is_working;
#if (REMOTE_IR_ENABLE)
	//ir key
	#define TYPE_IR_SEND			1
	#define TYPE_IR_RELEASE			2

	///////////////////// key mode //////////////////////
	#define KEY_MODE_BLE	   		0    //ble key
	#define KEY_MODE_IR        		1    //ir  key


	static const u8 kb_map_ble[30] = 	KB_MAP_BLE;
	static const u8 kb_map_ir[30] = 	KB_MAP_IR;


	void ir_dispatch(u8 type, u8 syscode ,u8 ircode){

		if(!ir_hw_initialed){
			ir_hw_initialed = 1;
			rc_ir_init();
		}
		#if(MCU_CORE_TYPE==MCU_CORE_TC321X)
			ir_learn_ana_tx_en();
		#endif
		if(type == TYPE_IR_SEND){
			ir_nec_send(syscode,~(syscode),ircode);

		}
		else if(type == TYPE_IR_RELEASE){
			ir_send_release();
		}
	}


#endif

static const u16 vk_consumer_map[16] = {
		MKEY_VOL_UP,
		MKEY_VOL_DN,
		MKEY_MUTE,
		MKEY_CHN_UP,

		MKEY_CHN_DN,
		MKEY_POWER,
		MKEY_AC_SEARCH,
		MKEY_RECORD,

		MKEY_PLAY,
		MKEY_PAUSE,
		MKEY_STOP,
		MKEY_FAST_FORWARD,  //can not find fast_backword in <<HID Usage Tables>>

		MKEY_FAST_FORWARD,
		MKEY_AC_HOME,
		MKEY_AC_BACK,
		MKEY_MENU,
};





/////////////////////////// led management /////////////////////
#if (UI_LED_ENABLE)

	enum{
		LED_POWER_ON = 0,
		LED_AUDIO_ON,	//1
		LED_AUDIO_OFF,	//2
		LED_SHINE_SLOW, //3
		LED_SHINE_FAST, //4
		LED_SHINE_OTA,  //5
		LED_SHINE_IR_LEARN_OK //6
	};

	const led_cfg_t led_cfg[] = {
			{1000,    0,      1,      0x00,	 },    //power-on, 1s on
			{100,	  0 ,	  0xff,	  0x02,  },    //audio on, long on
			{0,	      100 ,   0xff,	  0x02,  },    //audio off, long off
			{500,	  500 ,   2,	  0x04,	 },    //1Hz for 3 seconds
			{250,	  250 ,   4,	  0x04,  },    //2Hz for 3 seconds
			{250,	  250 ,   200,	  0x08,  },    //2Hz for 50 seconds
			{1000,	  1000,	  3,	  0x10,	 },	   //IR learn OK, 3s on
	};

#endif
#if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_	int 	key_not_released;
_attribute_ble_data_retention_	u8 		key_type;

/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{

	latest_user_event_tick = clock_time();  //record latest key change time
#if BLE_AUDIO_ENABLE
	#if (TL_AUDIO_MODE == TL_AUDIO_RCU_ADPCM_GATT_TELINK)			//GATT Telink
		if(key_voice_press){  //clear voice key press flg
			key_voice_press = 0;
		}
	#endif
#endif



	u8 key0 = kb_event.keycode[0];
	u8 key_value;
	u8 key_buf[8] = {0,0,0,0,0,0,0,0};

	key_not_released = 1;
	if (kb_event.cnt == 2)   //two key press
	{

	}
	else if(kb_event.cnt == 1)
	{
		if(key0 == KEY_MODE_SWITCH)
		{
			user_key_mode = !user_key_mode;

			#if (REMOTE_IR_ENABLE)
				if(user_key_mode == KEY_MODE_BLE){
					analog_write(USED_DEEP_ANA_REG, analog_read(USED_DEEP_ANA_REG)&(~IR_MODE_DEEP_FLG));
				}
				else{
					analog_write(USED_DEEP_ANA_REG, analog_read(USED_DEEP_ANA_REG)|IR_MODE_DEEP_FLG);
				}
			#endif

			#if (UI_LED_ENABLE)
				device_led_setup(led_cfg[LED_SHINE_SLOW + user_key_mode]);
			#endif
		}
		else if (key0 == VOICE)
		{
#if (BLE_AUDIO_ENABLE)
			key_voice_is_press();
#endif
		}

#if (REMOTE_IR_ENABLE)
		#if (REMOTE_IR_LEARN_ENABLE)
		else if(key0 == IR_POWER)
		{
			if(!get_ir_learn_state())
			{
				ir_learn_send_init();
				ir_learn_copy_result(&ir_learn_result);
				ir_learn_send(&ir_learn_result);
				device_led_setup(led_cfg[LED_SHINE_FAST]);
			}
			else
			{
				ir_learn_start();
				device_led_setup(led_cfg[LED_AUDIO_ON]);
			}
		}
		#endif
		else if(user_key_mode == KEY_MODE_BLE)
		{

			key_value = kb_map_ble[key0];
			if(key_value >= 0xf0 ){
				key_type = CONSUMER_KEY;
				u16 consumer_key = vk_consumer_map[key_value & 0x0f];
				for(int i=ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++){ //slave index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
					if(conn_dev_list[i].conn_state){
						blc_gatt_pushHandleValueNotify ( conn_dev_list[i].conn_handle, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
					}
				}

			}
			else
			{
				key_type = KEYBOARD_KEY;
				key_buf[2] = key_value;
				for(int i=ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++){ //slave index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
					if(conn_dev_list[i].conn_state){
						blc_gatt_pushHandleValueNotify ( conn_dev_list[i].conn_handle, HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
					}
				}
			}

		}
		else if(user_key_mode == KEY_MODE_IR)
		{  //IR mode
			key_value = kb_map_ir[key0];
			key_type = IR_KEY;
			if(!ir_not_released){
				ir_dispatch(TYPE_IR_SEND, 0x88, key_value);
				ir_not_released = 1;
			}
		}
		else
		{
			key_type = IDLE_KEY;
		}
#else
		else
		{
			key_value = key0;
			if(key_value >= 0xf0 ){
				key_type = CONSUMER_KEY;
				u16 consumer_key = vk_consumer_map[key_value & 0x0f];
				for(int i=ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++){ //slave index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
					if(conn_dev_list[i].conn_state){
						blc_gatt_pushHandleValueNotify ( conn_dev_list[i].conn_handle, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
					}
				}

			}
			else
			{
				key_type = KEYBOARD_KEY;
				key_buf[2] = key_value;
				for(int i=ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++){ //slave index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
					if(conn_dev_list[i].conn_state){
						blc_gatt_pushHandleValueNotify ( conn_dev_list[i].conn_handle, HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
					}
				}
			}
		}

#endif

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
			for(int i=ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++){ //slave index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
				if(conn_dev_list[i].conn_state){
					blc_gatt_pushHandleValueNotify ( conn_dev_list[i].conn_handle, HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
				}
			}
		}
#if (REMOTE_IR_ENABLE)
		else if(key_type == IR_KEY)
		{
			if(ir_not_released){
				ir_not_released = 0;
				ir_dispatch(TYPE_IR_RELEASE, 0, 0);  //release
			}
		}
#endif
		else if(key_type == PAIR_UNPAIR_KEY){
			if(central_pairing_enable){
				central_pairing_enable = 0;
			}

			if(central_unpair_enable){
				central_unpair_enable = 0;
			}
		}

#if (BLE_AUDIO_ENABLE)
		key_voice_is_release();
#endif
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
    (void)e;(void)p;(void)n;
#if(REMOTE_IR_LEARN_ENABLE)
	static u8	led_open = 1;
	if(!get_ir_learn_state() )
	{
		if(led_open)
		{
			led_open = 0;
			device_led_setup(led_cfg[LED_SHINE_IR_LEARN_OK]);
		}
	}
	else if(get_ir_learn_state() > IR_LEARN_SUCCESS)
	{
		device_led_setup(led_cfg[LED_AUDIO_OFF]);
	}
#endif

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

#if(REMOTE_IR_LEARN_ENABLE)
	ir_learn_detect();
#endif

#if (BLE_AUDIO_ENABLE)
	audio_state_check();
#endif

}




#endif   //end of UI_KEYBOARD_ENABLE

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


void app_ui_init_normal(void)
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


	#if (BLE_AUDIO_ENABLE)
		#if (BLE_DMIC_ENABLE)
			dmic_gpio_reset();
		#else
			amic_gpio_reset();
		#endif
	#endif


#if (UI_LED_ENABLE)
	device_led_init(GPIO_LED_GREEN, LED_ON_LEVEL);  //LED initialization
	device_led_setup(led_cfg[LED_POWER_ON]);
#endif


#if (REMOTE_IR_ENABLE)
	user_key_mode = analog_read(USED_DEEP_ANA_REG) & IR_MODE_DEEP_FLG ? KEY_MODE_IR : KEY_MODE_BLE;
#endif


}

void app_ui_init_deepRetn(void)
{
	#if (UI_KEYBOARD_ENABLE)
		/////////// keyboard gpio wakeup init ////////
		u32 pin[] = KB_DRIVE_PINS;
		for (unsigned int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
		{
			cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high wakeup deepsleep
		}
	#endif

	#if (BLE_AUDIO_ENABLE)
		amic_gpio_reset();
	#endif

#if (UI_LED_ENABLE)
	device_led_init(GPIO_LED_GREEN, LED_ON_LEVEL);  //LED initialization
#endif

#if (REMOTE_IR_ENABLE)
	user_key_mode = analog_read(USED_DEEP_ANA_REG) & IR_MODE_DEEP_FLG ? KEY_MODE_IR : KEY_MODE_BLE;
#endif
}

/**
 * @brief      this function is used to register the function for OTA start.
 * @param[in]  none
 * @return     none
 */
void app_enter_ota_mode(void)
{
	ota_is_working = 1;

#if (UI_LED_ENABLE)
	device_led_init(GPIO_LED_GREEN, LED_ON_LEVEL);  //LED initialization
#endif
}


