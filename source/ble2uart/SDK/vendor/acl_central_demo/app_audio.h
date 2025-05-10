/********************************************************************************************************
 * @file    app_audio.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef APP_AUDIO_H_
#define APP_AUDIO_H_

#include "tl_common.h"
#include "app_config.h"

_attribute_ram_code_ void   abuf_dec_usb ();
_attribute_ram_code_ void	usb_endpoints_irq_handler (void);
_attribute_ram_code_ void	proc_audio (void);

void	abuf_init ();
void	abuf_mic_add (u32 *p);
void	abuf_mic_dec ();
void	att_mic (u16 conn, u8 *p);



#endif

