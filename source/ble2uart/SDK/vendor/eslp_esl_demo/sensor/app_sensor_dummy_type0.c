/********************************************************************************************************
 * @file    app_sensor_dummy_type0.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "app_sensor_dummy_type0.h"

static bool app_sensor_dummy_type0_read(u8 *length, u8 *data)
{
    _attribute_data_retention_ static u32 counter = 0;
    u8 buf[] = {U32_TO_BYTES(counter)};

    memcpy(data, buf, min(*length, sizeof(buf)));
    *length = min(*length, sizeof(buf));

    counter++;

    return true;
}

_attribute_data_retention_ static app_sensor_iface_t app_sensor_dummy_type0_iface = {
    .config = {
        .size = APP_SENSOR_SIZE_0,
        { .size0 = 0xffff },
    },
    .read = app_sensor_dummy_type0_read,
};

const app_sensor_iface_t *app_sensor_dummy_type0_get_iface(void)
{
    return &app_sensor_dummy_type0_iface;
}
