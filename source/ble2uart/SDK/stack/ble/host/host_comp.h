/********************************************************************************************************
 * @file    host_comp.h
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
#ifndef STACK_BLE_HOST_HOST_COMP_H_
#define STACK_BLE_HOST_HOST_COMP_H_

/* for BLE Host old MICRO name compatible with new SDK */
#define GAP_EVT_SMP_TK_DISPALY							GAP_EVT_SMP_TK_DISPLAY
#define GAP_EVT_MASK_SMP_TK_DISPALY						GAP_EVT_MASK_SMP_TK_DISPLAY
#define PAIRING_FAIL_REASON_INVAILD_PARAMETER			PAIRING_FAIL_REASON_INVALID_PARAMETER
#define PAIRING_FAIL_REASON_NUMUERIC_FAILED				PAIRING_FAIL_REASON_NUMERIC_FAILED
#define PAIRING_FAIL_REASON_PAIRING_TIEMOUT				PAIRING_FAIL_REASON_PAIRING_TIMEOUT
#define GATT_ERR_SERVICE_DISCOVERY_TIEMOUT				GATT_ERR_SERVICE_DISCOVERY_TIMEOUT

#define GAP_EVT_GATT_HANDLE_VLAUE_CONFIRM				GAP_EVT_GATT_HANDLE_VALUE_CONFIRM
#define GAP_EVT_MASK_GATT_HANDLE_VLAUE_CONFIRM			GAP_EVT_MASK_GATT_HANDLE_VALUE_CONFIRM

/* for BLE Host old API name compatible with new SDK */
#define blc_smp_setBondingInfoIndexUpdateMethod    //not used in this SDK
#define blc_gatt_pushAttHdlValueCfm blc_gatt_pushConfirm


#endif /* STACK_BLE_HOST_HOST_COMP_H_ */
