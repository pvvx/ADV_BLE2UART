/********************************************************************************************************
 * @file    app_led_mono_red.c
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
#include "app_led_mono_red.h"

static bool app_led_mono_red_apply(const app_led_settings_t *settings)
{
    (void) settings;

    // Ignore settings - color is fixed in monochrome LEDs, and we can't set brightness
    return true;
}

static bool app_led_mono_red_toggle(bool en)
{
#if UI_LED_ENABLE && defined(GPIO_LED_GREEN)
    gpio_write(GPIO_LED_RED, en ? 1 : 0);
#else
    (void) en;
#endif
    return true;
}

_attribute_data_retention_ static app_led_iface_t app_led_mono_red_iface = {
    .config = {
        .colorRed = 3,
        .colorGreen = 0,
        .colorBlue = 0,
        .type = APP_LED_TYPE_MONO,
    },
    .apply = app_led_mono_red_apply,
    .toggle = app_led_mono_red_toggle,
};

const app_led_iface_t *app_led_mono_red_get_iface(void)
{
    return &app_led_mono_red_iface;
}
