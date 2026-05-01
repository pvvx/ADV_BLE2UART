/********************************************************************************************************
 * @file    app_sensor_dummy_type1.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    05,2024
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app_sensor.h"

/**
 * @brief      Get the interface for the Dummy Sensor Type 1.
 * @param[in]  none - No input parameters.
 * @return     const app_sensor_iface_t* - Pointer to the sensor interface for Dummy Sensor Type 1.
 */
const app_sensor_iface_t *app_sensor_dummy_type1_get_iface(void);
