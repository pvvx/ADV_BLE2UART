/********************************************************************************************************
 * @file    svc_esls.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    07,2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#pragma once

#ifndef ESLS_DISPLAYS_SUPPORTED
#define ESLS_DISPLAYS_SUPPORTED             1
#endif

#ifndef ESLS_SENSORS_SUPPORTED
#define ESLS_SENSORS_SUPPORTED              1
#endif

#ifndef ESLS_LEDS_SUPPORTED
#define ESLS_LEDS_SUPPORTED                 1
#endif

void blc_svc_addEslsGroup(void);
void blc_svc_removeEslsGroup(void);
void blc_svc_eslsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback);
