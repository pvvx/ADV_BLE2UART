/********************************************************************************************************
 * @file    app_sensor.h
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

#ifndef APP_SENSOR
#define APP_SENSOR

#define APP_SENSOR_MAX_SENSORS  4

typedef enum {
    APP_SENSOR_SIZE_0 = 0x00,
    APP_SENSOR_SIZE_1 = 0x01,
} app_sensor_size_t;

typedef struct {
    app_sensor_size_t size;
    union {
        u16 size0;
        u32 size1;
    };
} app_sensor_config_t;

typedef bool (*app_sensor_init_cb_t)(void);
typedef bool (*app_sensor_read_cb_t)(u8 *length, u8 *data);
typedef void (*app_sensor_loop_cb_t)(void);

typedef struct {
    app_sensor_config_t config;
    app_sensor_init_cb_t init;
    app_sensor_read_cb_t read;
    app_sensor_loop_cb_t loop;
} app_sensor_iface_t;

/**
 * @brief      Get the number of sensors currently available.
 * @param[in]  none - No input parameters.
 * @return     u8 - The number of available sensors.
 */
u8 app_sensor_get_num_sensors(void);

/**
 * @brief      Register a sensor with the specified interface.
 * @param[in]  iface - Pointer to the sensor interface to be registered.
 * @param[out] sensor_id - Pointer to the variable where the sensor ID will be stored upon successful registration.
 * @return     bool - true: registration successful, false: registration failed.
 */
bool app_sensor_register(const app_sensor_iface_t *iface, u8 *sensor_id);

/**
 * @brief      Main loop for the sensor management system. Should be called periodically to process sensor tasks.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_sensor_loop(void);

/**
 * @brief      Read data from the specified sensor.
 * @param[in]  sensor_id - The ID of the sensor to read from.
 * @param[out] length - Pointer to the variable where the length of the data will be stored.
 * @param[out] data - Pointer to the buffer where the sensor data will be stored.
 * @return     bool - true: data read successfully, false: failed to read data.
 */
bool app_sensor_read(u8 sensor_id, u8 *length, u8 *data);

/**
 * @brief      Get information about the specified sensor.
 * @param[in]  sensor_id - The ID of the sensor to query.
 * @param[out] config - Pointer to the sensor configuration structure where the sensor configuration will be stored.
 * @return     bool - true: information retrieved successfully, false: failed to retrieve information.
 */
bool app_sensor_get_info(u8 sensor_id, app_sensor_config_t *config);

#endif /* APP_SENSOR */
