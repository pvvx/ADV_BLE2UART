/********************************************************************************************************
 * @file    app_display_dummy.c
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
#include "app_display_dummy.h"

#define DISPLAY_DUMMY_WIDTH         128
#define DISPLAY_DUMMY_HEIGHT        128
#define DISPLAY_DUMMY_IMAGE_SIZE    2048
#define DISPLAY_DUMMY_TYPE          BLC_ESLS_DISPLAY_TYPE_BLACK_WHITE

static bool app_display_dummy_is_busy(void)
{
    return false;
}

static bool app_display_dummy_image(void)
{
    tlkapi_printf(APP_LOG_EN, "app_display_dummy_image size");

    return true;
}

static const app_display_iface_t app_display_dummy_cb = {
    .width = DISPLAY_DUMMY_WIDTH,
    .heigth = DISPLAY_DUMMY_HEIGHT,
    .type = DISPLAY_DUMMY_TYPE,
    .image_size = DISPLAY_DUMMY_IMAGE_SIZE,
    .is_busy = app_display_dummy_is_busy,
    .image = app_display_dummy_image,
};

const app_display_iface_t *app_display_dummy_get_iface(void)
{
    return &app_display_dummy_cb;
}
