/********************************************************************************************************
 * @file    app_esl.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    10,2023
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

/**
 * @brief      Handle the controller event for the ESL (Electronic Shelf Label) system.
 * @param[in]  h - The event handle associated with the controller event.
 * @param[in]  p - Pointer to the event data buffer.
 * @param[in]  n - The length of the event data buffer.
 * @return     none.
 */
void app_esl_handle_controller_event(u32 h, u8 *p, int n);

/**
 * @brief      Handle the host event for the ESL system.
 * @param[in]  h - The event handle associated with the host event.
 * @param[in]  p - Pointer to the event data buffer.
 * @param[in]  n - The length of the event data buffer.
 * @return     none.
 */
void app_esl_handle_host_event(u32 h, u8 *p, int n);

/**
 * @brief      Initialize the ESL system.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_esl_init(void);

/**
 * @brief      Main loop for the ESL system. Should be called periodically to process ESL tasks.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_esl_loop(void);

/**
 * @brief      Check if the ESL system is currently busy with a task.
 * @param[in]  none - No input parameters.
 * @return     bool - true: ESL system is busy, false: ESL system is idle.
 */
bool app_esl_task_is_busy(void);
