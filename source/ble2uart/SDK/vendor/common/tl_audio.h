/********************************************************************************************************
 * @file    tl_audio.h
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
#ifndef _TL_AUDIO_H_
#define _TL_AUDIO_H_

#include "tl_common.h"

#ifndef		TL_MIC_PACKET_BUFFER_NUM
#define		TL_MIC_PACKET_BUFFER_NUM		4
#endif


#ifndef		TL_MIC_BUFFER_SIZE
#define		TL_MIC_BUFFER_SIZE				0
#endif


void	adpcm_to_pcm (signed short *ps, signed short *pd, int len);

#endif
