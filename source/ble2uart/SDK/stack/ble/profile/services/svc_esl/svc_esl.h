/********************************************************************************************************
 * @file    svc_esl.h
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

//ESLS
#define SERVICE_ESLS_HDL                                SERVICE_ELECTRONIC_SHELF_LABEL_HDL
#define ESLS_MAX_HDL_NUM                                0x20

//PRIMARY OTS
#define SERVICE_PRIMARY_OTS_HDL                         SERVICE_ESLS_HDL + ESLS_MAX_HDL_NUM
#define PRIMARY_OTS_MAX_HDL_NUM                         0x20

#include "esls/svc_esls.h"
#include "ots/svc_primary_ots.h"
