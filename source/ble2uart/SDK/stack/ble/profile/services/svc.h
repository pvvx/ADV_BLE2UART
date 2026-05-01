/********************************************************************************************************
 * @file    svc.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#define SERVICE_GATT_START_HANDLE                       0x0001

#define SERVICE_ELECTRONIC_SHELF_LABEL_HDL              0x0820

//Telink private Service all 128 uuid
#define SERVICE_TELINK_PRIVATE_START_HDL                0x8000


#include "svc_esl/svc_esl.h"
#include "svc_telink/svc_telink.h"

#include "svc_uuid.h"


