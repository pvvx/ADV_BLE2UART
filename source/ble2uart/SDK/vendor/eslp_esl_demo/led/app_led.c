/********************************************************************************************************
 * @file    app_led.c
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
#include "app_led.h"

_attribute_data_retention_ static struct {
    bool in_use : 1;
    bool command_active : 1;
    bool en : 1;
    const app_led_iface_t *iface;
    app_led_command_t current_command;
    u32 on_off_time;
    u32 repeat_time;
    u8 bit_offset;
} leds[APP_LED_MAX_LEDS];
u8 app_led_get_num_leds(void)
{
    u8 ret = 0;

    foreach_arr(i, leds) {
        ret += leds[i].in_use ? 1 : 0;
    }

    return ret;
}

bool app_led_register(const app_led_iface_t *iface, u8 *led_id)
{
    if (!iface || !iface->apply || !iface->toggle) {
        return false;
    }

    foreach_arr(i, leds) {
        if (leds[i].in_use) {
            continue;
        }

        leds[i].in_use = true;
        leds[i].iface = iface;
        if (led_id) {
            *led_id = i;
        }

        return true;
    }

    return false;
}

bool app_led_get_info(u8 led_id, app_led_config_t *config)
{
    if (led_id < ARRAY_SIZE(leds) && leds[led_id].in_use && leds[led_id].iface) {
        *config = leds[led_id].iface->config;
        return true;
    }

    return false;
}

static void app_led_apply(u8 led_id)
{
    bool en = false;

    if (leds[led_id].command_active) {
        if (leds[led_id].current_command.repeat_duration) {
            en = leds[led_id].current_command.pattern[leds[led_id].bit_offset / 8] &
                                    (1 << (leds[led_id].bit_offset % 8));
        } else {
            en = !!leds[led_id].current_command.repeat_type;
        }
    }

    if (leds[led_id].en != en) {
        leds[led_id].iface->toggle(en);
        leds[led_id].en = en;
    }
}

bool app_led_command_apply(u8 led_id, const app_led_command_t *command)
{
    if (led_id < ARRAY_SIZE(leds) && leds[led_id].in_use) {
        leds[led_id].current_command = *command;
        leds[led_id].command_active = true;
        leds[led_id].on_off_time = clock_time();
        leds[led_id].repeat_time = clock_time();
        leds[led_id].bit_offset = 39;
        leds[led_id].iface->apply(&command->settings);

        app_led_apply(led_id);

        return true;
    }

    return false;
}

static void app_led_id_loop(u8 led_id)
{
    bool led_changed = false;
    u32 timeout;
    if (!leds[led_id].command_active) {
        return;
    }

    /* If repeat duration is 0, then LED is turned on/off continuously */
    if (!leds[led_id].current_command.repeat_duration) {
        return;
    }

    timeout = leds[led_id].current_command.pattern[leds[led_id].bit_offset / 8] & (1 << (leds[led_id].bit_offset % 8)) ?
            leds[led_id].current_command.bit_on_period : leds[led_id].current_command.bit_off_period;

    timeout *= 2000;

    if (clock_time_exceed(leds[led_id].on_off_time, timeout)) {
        // Move one bit in pattern
        leds[led_id].on_off_time = clock_time();
        leds[led_id].bit_offset = !leds[led_id].bit_offset ? 39 : leds[led_id].bit_offset - 1;
        leds[led_id].bit_offset %= sizeof(leds[led_id].current_command.pattern) * 8;
        led_changed = true;

        if ((leds[led_id].bit_offset == 39) && leds[led_id].current_command.repeat_type == 0) {
            // decrease repeat duration
            leds[led_id].current_command.repeat_duration--;
        }
    }

    if ((leds[led_id].current_command.repeat_type == 1) && clock_time_exceed(leds[led_id].repeat_time, 1000 * 1000)) {
        leds[led_id].repeat_time = clock_time();
        leds[led_id].current_command.repeat_duration--;
    }

    if (!leds[led_id].current_command.repeat_duration) {
        leds[led_id].command_active = false;
        led_changed = true;
    }

    if (led_changed) {
        app_led_apply(led_id);
    }
}

bool app_led_is_active(u8 led_id)
{
    if (led_id < ARRAY_SIZE(leds) && leds[led_id].in_use) {
        return leds[led_id].command_active;
    }

    return false;
}


void app_led_loop(void)
{
    foreach_arr(i, leds) {
        if (!leds[i].in_use) {
            continue;
        }

        app_led_id_loop(i);
    }
}
