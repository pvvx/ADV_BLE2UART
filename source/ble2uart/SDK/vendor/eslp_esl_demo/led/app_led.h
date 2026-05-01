/********************************************************************************************************
 * @file    app_led.h
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

#ifndef APP_LED
#define APP_LED

#define APP_LED_MAX_LEDS    4

typedef enum {
    APP_LED_TYPE_SRGB = 0x00,
    APP_LED_TYPE_MONO = 0x01,
} app_led_type_t;

typedef struct {
    u8 colorRed : 2;
    u8 colorGreen : 2;
    u8 colorBlue : 2;
    u8 type : 2;
} app_led_config_t;

typedef struct {
    u8 colorRed : 2;
    u8 colorGreen : 2;
    u8 colorBlue : 2;
    u8 brightness : 2;
} app_led_settings_t;

typedef struct {
    app_led_settings_t settings;
    u8 pattern[5];
    u8 bit_off_period;
    u8 bit_on_period;
    u16 repeat_type : 1;
    u16 repeat_duration : 15;
} app_led_command_t;

typedef bool (*app_led_settings_apply_cb_t)(const app_led_settings_t *settings);
typedef bool (*app_led_toggle_cb_t)(bool on);

typedef struct {
    app_led_config_t config;
    app_led_settings_apply_cb_t apply;
    app_led_toggle_cb_t toggle;
} app_led_iface_t;

/**
 * @brief      Get the number of LEDs currently available.
 * @param[in]  none - No input parameters.
 * @return     u8 - The number of LEDs.
 */
u8 app_led_get_num_leds(void);

/**
 * @brief      Register an LED with the specified configuration.
 * @param[in]  config - Pointer to the configuration of the LED interface.
 * @param[out] led_id - Pointer to the variable where the LED ID will be stored upon successful registration.
 * @return     bool - true: registration successful, false: registration failed.
 */
bool app_led_register(const app_led_iface_t *config, u8 *led_id);

/**
 * @brief      Main loop for the LED management system. Should be called periodically to process LED tasks.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_led_loop(void);

/**
 * @brief      Apply a command to control the specified LED.
 * @param[in]  led_id - The ID of the LED to apply the command to.
 * @param[in]  command - Pointer to the LED command structure to be applied.
 * @return     bool - true: command applied successfully, false: failed to apply the command.
 */
bool app_led_command_apply(u8 led_id, const app_led_command_t *command);

/**
 * @brief      Get information about the specified LED.
 * @param[in]  led_id - The ID of the LED to query.
 * @param[out] config - Pointer to the LED configuration structure where the LED configuration will be stored.
 * @return     bool - true: information retrieved successfully, false: failed to retrieve information.
 */
bool app_led_get_info(u8 led_id, app_led_config_t *config);

/**
 * @brief      Check if the specified LED is currently active.
 * @param[in]  led_id - The ID of the LED to check.
 * @return     bool - true: LED is active, false: LED is not active.
 */
bool app_led_is_active(u8 led_id);

#endif /* APP_LED */
