/********************************************************************************************************
 * @file    app_display.c
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
#include "app_display.h"

_attribute_data_retention_ static struct {
    bool active;
    const app_display_iface_t *config;
} displays[APP_DISPLAY_MAX_DISPLAYS];

u8 app_display_get_num_displays(void)
{
    u8 ret = 0;

    foreach_arr(i, displays) {
        ret += displays[i].active ? 1 : 0;
    }

    return ret;
}

bool app_display_register(const app_display_iface_t *config, u8 *display_id)
{
    foreach_arr(i, displays) {
        if (displays[i].active) {
            continue;
        }

        displays[i].active = true;
        displays[i].config = config;
        if (display_id) {
            *display_id = i;
        }

        return true;
    }

    return false;
}

void app_display_loop(void)
{
    foreach_arr(i, displays) {
        if (!displays[i].active) {
            continue;
        }

        if (displays[i].config && displays[i].config->loop) {
            displays[i].config->loop();
        }
    }
}

bool app_display_image(u8 display_id)
{
    if (display_id < ARRAY_SIZE(displays) && displays[display_id].active &&
            displays[display_id].config && displays[display_id].config->image) {
        return displays[display_id].config->image();
    }
    return false;
}

bool app_display_get_buffer(u8 display_id, u8 **buffer_ptr)
{
    if (display_id < ARRAY_SIZE(displays) && displays[display_id].active &&
            displays[display_id].config && displays[display_id].config->get_buffer) {
        return displays[display_id].config->get_buffer(buffer_ptr);
    }

    return false;
}

bool app_display_is_busy(u8 display_id)
{
    if (display_id < ARRAY_SIZE(displays) && displays[display_id].active &&
            displays[display_id].config && displays[display_id].config->is_busy) {
        return displays[display_id].config->is_busy();
    }

    return false;
}

bool app_display_clean(u8 display_id)
{
    if (display_id < ARRAY_SIZE(displays) && displays[display_id].active &&
            displays[display_id].config && displays[display_id].config->clean) {
        return displays[display_id].config->clean();
    }

    return false;
}

bool app_display_get_info(u8 display_id, u16 *width, u16 *height, u8 *displayType, u16 *imageSize)
{
    if (display_id < ARRAY_SIZE(displays) && displays[display_id].active) {
        if (width) {
            *width = displays[display_id].config->width;
        }

        if (height) {
            *height = displays[display_id].config->heigth;
        }

        if (displayType) {
            *displayType = displays[display_id].config->type;
        }

        if (imageSize) {
            *imageSize = displays[display_id].config->image_size;
        }

        return true;
    }

    return false;
}
