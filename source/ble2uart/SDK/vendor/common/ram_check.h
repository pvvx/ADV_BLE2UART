/********************************************************************************************************
 * @file    ram_check.h
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
#ifndef RAM_CHECK_H_
#define RAM_CHECK_H_


/**
 * @brief		This function is used to check whether the firmware in RAM is OK.
 * @param[in]	none
 * @return      none
 */
void blc_app_ram_check(u32 ramStartAddr, u32 ramEndAddr, u32 flashStartAddr);

#endif /* RAM_FW_CHECK_H_ */
