/********************************************************************************************************
 * @file    svc_primary_ots.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    04,2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifndef OTS_CREATE_OBJECT_SUPPORTED
#define OTS_CREATE_OBJECT_SUPPORTED             0
#endif

#ifndef OTS_MULTIPLE_OBJECTS_SUPPORTED
#define OTS_MULTIPLE_OBJECTS_SUPPORTED          1
#endif

#ifndef OTS_OBJECT_FIRST_CREATED_SUPPORTED
#define OTS_OBJECT_FIRST_CREATED_SUPPORTED      0
#endif

#ifndef OTS_OBJECT_LAST_MODIFIED_SUPPORTED
#define OTS_OBJECT_LAST_MODIFIED_SUPPORTED      0
#endif

#ifndef OTS_SERVER_RTC_SUPPORTED
#define OTS_SERVER_RTC_SUPPORTED                0
#endif

#ifndef OTS_OBJECT_LIST_FILTER_SUPPORTED
#define OTS_OBJECT_LIST_FILTER_SUPPORTED        0
#endif

#ifndef OTS_OBJECT_PROPERTIES_WRITE_SUPPORTED
#define OTS_OBJECT_PROPERTIES_WRITE_SUPPORTED   0
#endif

#ifndef OTS_OBJECT_CHANGED_SUPPORTED
#define OTS_OBJECT_CHANGED_SUPPORTED            0
#endif

void blc_svc_addPrimaryOtsGroup(void);
void blc_svc_removePrimaryOtsGroup(void);
void blc_svc_primaryOtsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback);

