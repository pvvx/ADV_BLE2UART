/********************************************************************************************************
 * @file    app_sensor.c
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
#include "app_sensor.h"

_attribute_data_retention_  static struct {
    bool in_use : 1;
    const app_sensor_iface_t *iface;
} sensors[APP_SENSOR_MAX_SENSORS];

u8 app_sensor_get_num_sensors(void)
{
    u8 ret = 0;

    foreach_arr(i, sensors) {
        ret += sensors[i].in_use ? 1 : 0;
    }

    return ret;
}

bool app_sensor_register(const app_sensor_iface_t *iface, u8 *sensor_id)
{
    if (!iface || !iface->read) {
        return false;
    }

    foreach_arr(i, sensors) {
        if (sensors[i].in_use) {
            continue;
        }

        sensors[i].in_use = true;
        sensors[i].iface = iface;
        if (sensor_id) {
            *sensor_id = i;
        }

        if (sensors[i].iface->init) {
            sensors[i].iface->init();
        }

        return true;
    }

    return false;
}

void app_sensor_loop(void)
{
    foreach_arr(i, sensors) {
        if (sensors[i].in_use || !sensors[i].iface || !sensors[i].iface->loop) {
            continue;
        }

        sensors[i].iface->loop();
    }
}

bool app_sensor_read(u8 sensor_id, u8 *length, u8 *data)
{
    if (sensor_id < ARRAY_SIZE(sensors) && sensors[sensor_id].in_use) {
        return sensors[sensor_id].iface->read(length, data);
    }

    return false;
}

bool app_sensor_get_info(u8 sensor_id, app_sensor_config_t *config)
{
    if (sensor_id < ARRAY_SIZE(sensors) && sensors[sensor_id].in_use) {
        *config = sensors[sensor_id].iface->config;
        return true;
    }

    return false;
}
