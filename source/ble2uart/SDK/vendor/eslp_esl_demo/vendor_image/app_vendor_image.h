/********************************************************************************************************
 * @file    app_vendor_image.h
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
#include "gui.h"

#if APP_VENDOR_IMAGE

    #define APP_VENDOR_IMAGE_SIZE (4736)

/**
 * @brief      Handle vendor-specific control point commands for the vendor image.
 * @param[in]  vendorCmd - Pointer to the vendor-specific command structure.
 * @param[out] rsp - Pointer to the response header structure where the result will be stored.
 * @return     bool - true: command handled successfully, false: failed to handle the command.
 */
bool app_vendor_image_handle_vendor_cmd(blc_eslss_controlPointCommandVendorSpecific_t *vendorCmd,
                                        blc_eslss_controlPointResponseHdr_t           *rsp);

/**
 * @brief      Get the image data for the vendor image.
 * @param[out] image - Pointer to an array where the image data will be stored.
 * @return     none.
 */
void app_vendor_image_get_image(u8 image[APP_VENDOR_IMAGE_SIZE]);

/**
 * @brief      Get the size of the vendor image.
 * @param[in]  none - No input parameters.
 * @return     u16 - The size of the vendor image.
 */
u16 app_vendor_image_get_image_size(void);

#endif
