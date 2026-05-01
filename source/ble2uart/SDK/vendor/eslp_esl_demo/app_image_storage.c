/********************************************************************************************************
 * @file    app_image_storage.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    01,2024
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
#include "app_image_storage.h"


typedef struct __attribute__((packed))
{
    u32 length;
} app_image_storage_img_entry_t;

typedef struct __attribute__((packed))
{
    u8 num_images;
    u32 image_max_size;
    app_image_storage_img_entry_t entries[];
} app_image_storage_hdr_t;

_attribute_data_retention_ static bool app_image_storage_initialized;
_attribute_data_retention_ static u8   app_image_storage_hdr_cache[sizeof(app_image_storage_hdr_t) + (APP_IMAGE_STORAGE_MAX_IMAGES * sizeof(app_image_storage_img_entry_t))];
_attribute_data_retention_ static u8 image_data[APP_IMAGE_STORAGE_MAX_IMAGES][APP_IMAGE_STORAGE_MAX_IMAGE_SIZE];

void app_image_storage_init(void)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    if (app_image_storage_initialized) {
        return;
    }

    hdr->image_max_size = APP_IMAGE_STORAGE_MAX_IMAGE_SIZE;
    hdr->num_images     = APP_IMAGE_STORAGE_MAX_IMAGES;

    for (u8 i = 0; i < hdr->num_images; i++) {
        hdr->entries[i].length = 0;
    }

    app_image_storage_initialized = true;
}

u8 app_image_storage_get_max_image_number(void)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    if (!app_image_storage_initialized) {
        return 0;
    }

    return hdr->num_images;
}

u32 app_image_storage_get_max_image_length(void)
{
    app_image_storage_hdr_t *hdr        = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;
    u32  max_length = 0;

    if (!app_image_storage_initialized) {
        return 0;
    }

    max_length = hdr->image_max_size;

    return max_length;
}

bool app_image_storage_get_image_length(u8 image_idx, u32 *length)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    if (!app_image_storage_initialized || hdr->num_images <= image_idx) {
        return false;
    }

    *length = hdr->entries[image_idx].length;

    return true;
}

u32 app_image_storage_get_image_data(u8 image_idx, u32 offset, u32 length, u8 *buffer)
{
    u32 image_length;

    if (!app_image_storage_get_image_length(image_idx, &image_length)) {
        return 0;
    }

    if (offset >= image_length) {
        return 0;
    }

    length = (offset + length) > image_length ? image_length - offset : length;
    memcpy(buffer, &image_data[image_idx][offset], length);

    return length;
}

bool app_image_storage_update_image_info(u8 image_idx, u32 length)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    if (!app_image_storage_initialized || hdr->num_images <= image_idx) {
        return false;
    }

    if (length > app_image_storage_get_max_image_length()) {
        return false;
    }

    hdr->entries[image_idx].length = length;

    return true;
}

u32 app_image_storage_image_write(u8 image_idx, u32 length, u32 offset, u8 *data, bool truncate)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    (void)truncate;

    if (!app_image_storage_initialized || hdr->num_images <= image_idx) {
        return 0;
    }

    if (offset + length > app_image_storage_get_max_image_length()) {
        return 0;
    }

    memcpy(&image_data[image_idx][offset], data, length);

    return length;
}
