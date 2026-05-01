/********************************************************************************************************
 * @file    app_image_storage.h
 *
 * @brief   This is the header file for BLE SDK
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

/**
 * @brief      Initialize the image storage system.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_image_storage_init(void);

/**
 * @brief      Get the maximum number of images that can be stored.
 * @param[in]  none - No input parameters.
 * @return     u8 - The maximum number of images that can be stored.
 */
u8 app_image_storage_get_max_image_number(void);

/**
 * @brief      Get the maximum length (in bytes) that an image can have.
 * @param[in]  none - No input parameters.
 * @return     u32 - The maximum length of an image in bytes.
 */
u32 app_image_storage_get_max_image_length(void);

/**
 * @brief      Get the length of the specified image.
 * @param[in]  image_idx - The index of the image whose length is to be retrieved.
 * @param[out] length - Pointer to a variable where the image length will be stored.
 * @return     bool - true: length retrieved successfully, false: failed to retrieve length.
 */
bool app_image_storage_get_image_length(u8 image_idx, u32 *length);

/**
 * @brief      Retrieve the image data from the storage.
 * @param[in]  image_idx - The index of the image to be retrieved.
 * @param[in]  offset - The offset from where to start retrieving data in the image.
 * @param[in]  length - The length of data to be retrieved from the image.
 * @param[out] buffer - Pointer to the buffer where the retrieved image data will be stored.
 * @return     u32 - The number of bytes successfully retrieved.
 */
u32 app_image_storage_get_image_data(u8 image_idx, u32 offset, u32 length, u8 *buffer);

/**
 * @brief      Update the information of the specified image (e.g., its length).
 * @param[in]  image_idx - The index of the image whose information is to be updated.
 * @param[in]  length - The new length of the image.
 * @return     bool - true: information updated successfully, false: failed to update information.
 */
bool app_image_storage_update_image_info(u8 image_idx, u32 length);

/**
 * @brief      Write data to the specified image storage.
 * @param[in]  image_idx - The index of the image to be written to.
 * @param[in]  length - The length of the data to be written.
 * @param[in]  offset - The offset at which to start writing the data.
 * @param[in]  data - Pointer to the data to be written.
 * @param[in]  truncate - Flag indicating whether to truncate the image to the new length.
 * @return     u32 - The number of bytes successfully written.
 */
u32 app_image_storage_image_write(u8 image_idx, u32 length, u32 offset, u8 *data, bool truncate);
