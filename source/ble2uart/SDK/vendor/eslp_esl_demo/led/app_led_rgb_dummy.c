/********************************************************************************************************
 * @file    app_led_rgb_dummy.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    03,2025
 *
 * @par     Copyright (c) 2025, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "app_led_mono_green.h"

static bool app_led_rgb_dummy_apply(const app_led_settings_t *settings);
static bool app_led_rgb_dummy_toggle(bool en);

_attribute_data_retention_ static app_led_iface_t app_led_rgb_dummy_iface = {
    .config = {
        .colorRed = 3,
        .colorGreen = 3,
        .colorBlue = 3,
        .type = APP_LED_TYPE_SRGB,
    },
    .apply = app_led_rgb_dummy_apply,
    .toggle = app_led_rgb_dummy_toggle,
};

static bool app_led_rgb_dummy_apply(const app_led_settings_t *settings)
{
    // In RGB LED color settings must be applied
    app_led_rgb_dummy_iface.config.colorBlue = settings->colorBlue;
    app_led_rgb_dummy_iface.config.colorGreen = settings->colorGreen;
    app_led_rgb_dummy_iface.config.colorRed = settings->colorRed;

    return true;
}

static bool app_led_rgb_dummy_toggle(bool en)
{
    (void) en;

    return true;
}

const app_led_iface_t *app_led_rgb_dummy_get_iface(void)
{
    return &app_led_rgb_dummy_iface;
}
