/********************************************************************************************************
 * @file    app_audio.c
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
#include "app_config.h"
#include "application/audio/tl_audio.h"
#include "application/audio/audio_config.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "app_att.h"
#include "app_audio.h"
#include "vendor/common/blt_led.h"
#include "battery_check.h"

_attribute_data_retention_	u8		ui_mic_enable = 0;
_attribute_data_retention_	u8 		key_voice_press = 0;
_attribute_data_retention_	int     ui_mtu_size_exchange_req = 0;

_attribute_data_retention_	u32     audioProcStart_tick = 0;
_attribute_data_retention_	u32     audioProcDelay_us = 0;




#if (BLE_AUDIO_ENABLE)

/**
 * @brief      the func serve to init amic
 * @param[in]  none
 * @return     none
 */
void amic_gpio_reset (void)
{
	gpio_set_func(GPIO_AMIC_BIAS, AS_GPIO);
	gpio_set_input_en(GPIO_AMIC_BIAS, 0);
	gpio_set_output_en(GPIO_AMIC_BIAS, 1);
	gpio_write(GPIO_AMIC_BIAS, 0);

	gpio_set_func(GPIO_AMIC_SP, AS_GPIO);
	gpio_set_input_en(GPIO_AMIC_SP, 0);
	gpio_set_output_en(GPIO_AMIC_SP, 1);
	gpio_write(GPIO_AMIC_SP, 0);

	gpio_set_func(GPIO_AMIC_SN, AS_GPIO);
	gpio_set_input_en(GPIO_AMIC_SN, 0);
	gpio_set_output_en(GPIO_AMIC_SN, 1);
	gpio_write(GPIO_AMIC_SN, 0);

}


/**
 * @brief      This function is the microphone delay function.
 * @param[in]  delay_time: microphone delay duration, unit is us.
 * @return     none
 */
void audio_proc_delay(u32 delay_time_us)
{
	audioProcDelay_us = delay_time_us;
}


#if (TL_AUDIO_MODE == TL_AUDIO_RCU_ADPCM_GATT_TELINK)					//GATT Telink

u32 	key_voice_pressTick = 0;
u32 	audio_stick = 0;
_attribute_data_retention_	u8  audio_start = 0;

extern u8		buffer_mic_pkt_wptr;
extern u8		buffer_mic_pkt_rptr;

/**
 * @brief      for open the audio and mtu size exchange
 * @param[in]  en   0:close the micphone  1:open the micphone
 * @return     none
 */
void ui_enable_mic (int en)
{
#if (APP_BATT_CHECK_ENABLE)
	battery_set_detect_enable(!en);
#endif
	ui_mic_enable = en;

	//AMIC Bias output
	gpio_set_output_en (GPIO_AMIC_BIAS, en);
	gpio_write (GPIO_AMIC_BIAS, en);

	#if (UI_LED_ENABLE)
		extern const led_cfg_t led_cfg[];
		device_led_setup(led_cfg[en ? 1 : 2]);
	#endif

	if(en){  //audio on

		///////////////////// AUDIO initialization///////////////////
		//buffer_mic set must before audio_init !!!
		audio_init();
		buffer_mic_pkt_rptr = buffer_mic_pkt_wptr = 0;
		//Amic config
		audio_set_mute_pga(0);  ////enable audio need follow this step: 1 enable bias; 2 disable mute_pga;
		gpio_set_output_en(GPIO_AMIC_SP, 0);
		gpio_set_output_en(GPIO_AMIC_SN, 0);
		audio_codec_stream0_input_t audio_codec_stream0_input =
		{
			.input_src = AMIC_STREAM0_MONO_L,
			.sample_rate = AUDIO_16K,
			.data_width = CODEC_BIT_16_DATA,
			.fifo_chn = FIFO0,
			.data_buf = buffer_mic,
			.data_buf_size = TL_MIC_BUFFER_SIZE,
		};
		audio_codec_stream0_input_init(&audio_codec_stream0_input);
		audio_dfifo_config(FIFO0,(u16*)buffer_mic,TL_MIC_BUFFER_SIZE);
		audio_txfifo_en(FIFO0);


		#if (IIR_FILTER_ENABLE)
			//only used for debugging EQ Filter parameters, removed after mass production
			extern void filter_setting();
			filter_setting();
		#endif
			if(audioProcDelay_us)
			{
				audioProcStart_tick = clock_time()|1;
			}
	}
	else{  //audio off
		audio_codec_adc_power_down();
		amic_gpio_reset();
		buffer_mic_pkt_rptr = buffer_mic_pkt_wptr = 0;
	}
}


/**
 * @brief      this function is used to define what to do when voice key is pressed
 * @param[in]  none
 * @return     none
 */
void key_voice_is_press(void)
{
	if(ui_mic_enable){  //if voice on, voice off
		ui_enable_mic (0);
	}
	else{ //if voice not on, mark voice key press tick
		key_voice_press = 1;
		key_voice_pressTick = clock_time();
	}
}

/**
 * @brief      this function is used to define what to do when voice key is released
 * @param[in]  none
 * @return     none
 */
void key_voice_is_release(void)
{
	if(ui_mic_enable){
		ui_enable_mic(0);
	}
}

/**
 * @brief      this function is used to check audio state
 * @param[in]  none
 * @return     none
 */
void audio_state_check(void)
{
	 //long press voice 1 second
	for(int i = ACL_CENTRAL_MAX_NUM; i < ACL_PERIPHR_MAX_NUM+ACL_CENTRAL_MAX_NUM; i++){
		if(conn_dev_list[i].conn_state != 0){
			if(key_voice_press && !ui_mic_enable && \
				clock_time_exceed(key_voice_pressTick, 1000000)){
				voice_press_proc();
			}
		}
	}
}

/**
 * @brief      for open the audio and mtu size exchange
 * @param[in]  none
 * @return     none
 */
void voice_press_proc(void)
{
	key_voice_press = 0;
	audio_stick = clock_time()|1;
	ui_enable_mic (1);
}

/**
 * @brief      audio task in loop for encode and transmit encode data
 * @param[in]  none
 * @return     none
 */
void task_audio (void)
{
	static u32 audioProcTick = 0;
	if(clock_time_exceed(audioProcTick, 500)){
		audioProcTick = clock_time();
	}
	else{
		return;
	}
	if(audioProcStart_tick&&clock_time_exceed(audioProcStart_tick, audioProcDelay_us)){
		memset(buffer_mic, 0, TL_MIC_BUFFER_SIZE);
		audioProcStart_tick = 0;
	}
	else if(audioProcStart_tick!=0){
		return;
	}
	///////////////////////////////////////////////////////////////
	proc_mic_encoder ();

	//////////////////////////////////////////////////////////////////
	for(int i = ACL_CENTRAL_MAX_NUM; i < ACL_PERIPHR_MAX_NUM+ACL_CENTRAL_MAX_NUM; i++){
		if(conn_dev_list[i].conn_state != 0){
			if(blc_ll_getTxFifoNumber(conn_dev_list[i].conn_handle) < 9)
			{
				int *p = mic_encoder_data_buffer ();
				if (p)					//around 3.2 ms @16MHz clock
				{
					if( BLE_SUCCESS ==  blc_gatt_pushHandleValueNotify (conn_dev_list[i].conn_handle,AUDIO_MIC_INPUT_DP_H, (u8*)p, ADPCM_PACKET_LEN)){
						mic_encoder_data_read_ok();
					}
				}
			}
			break;
		}
	}


}


/**
 * @brief      audio proc in main loop
 * @param[in]  none
 * @return     none
 */
void proc_audio(void){
	if(ui_mic_enable){
		if(audio_start || (audio_stick && clock_time_exceed(audio_stick, 1*1000))){// for 8258
			audio_start = 1;
			task_audio();
		}

	}
	else{
		audio_start = 0;
	}
}

#endif

#endif
