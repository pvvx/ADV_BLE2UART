/********************************************************************************************************
 * @file    app_display.h
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

#ifndef APP_DISPLAY
#define APP_DISPLAY

#define APP_DISPLAY_MAX_DISPLAYS    4

typedef bool (*app_display_image_cb_t)(void);
typedef bool (*app_display_get_buffer_cb_t)(u8 **buffer_ptr);
typedef bool (*app_display_clean_cb_t)(void);
typedef bool (*app_display_is_busy_cb_t)(void);
typedef void (*app_display_loop_cb_t)(void);

typedef struct {
    u16 width;
    u16 heigth;
    u16 image_size;
    u8 type;
    app_display_image_cb_t image;
    app_display_is_busy_cb_t is_busy;
    app_display_loop_cb_t loop;
    app_display_clean_cb_t clean;
    app_display_get_buffer_cb_t get_buffer;
} app_display_iface_t;

u8 app_display_get_num_displays(void);
bool app_display_register(const app_display_iface_t *config, u8 *display_id);
void app_display_loop(void);
bool app_display_image(u8 display_id);
bool app_display_clean(u8 display_id);
bool app_display_is_busy(u8 display_id);
bool app_display_get_info(u8 display_id, u16 *width, u16 *height, u8 *displayType, u16 *image_size);
bool app_display_get_buffer(u8 display_id, u8 **buffer_ptr);

#endif /* APP_DISPLAY */
