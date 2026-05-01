/********************************************************************************************************
 * @file    esls_server_buf.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#pragma once

typedef void (*eslsCallback_t)(u16 connHandle, int evtID, u8 *data, u16 dataLen);

typedef enum {
    BLC_ESLS_DISPLAY_TYPE_BLACK_WHITE = 0x01,
    BLC_ESLS_DISPLAY_TYPE_THREE_GRAY_SCALE,
    BLC_ESLS_DISPLAY_TYPE_FOUR_GRAY_SCALE,
    BLC_ESLS_DISPLAY_TYPE_EIGHT_GRAY_SCALE,
    BLC_ESLS_DISPLAY_TYPE_SIXTEEN_GRAY_SCALE,
    BLC_ESLS_DISPLAY_TYPE_RED_BLACK_WHITE,
    BLC_ESLS_DISPLAY_TYPE_YELLOW_BLACK_WHITE,
    BLC_ESLS_DISPLAY_TYPE_RED_YELLOW_BLACK_WHITE,
    BLC_ESLS_DISPLAY_TYPE_SEVEN_COLOR,
    BLC_ESLS_DISPLAY_TYPE_SIXTEEN_COLOR,
    BLC_ESLS_DISPLAY_TYPE_FULL_RGB,
} blc_esls_displayType_t;

typedef struct {
    u16 width;
    u16 height;
    u8 displayType;
} blc_esls_displayData_t;

typedef enum {
    BLC_ESLS_SENSOR_INFORMATION_SIZE_0 = 0x00,
    BLC_ESLS_SENSOR_INFORMATION_SIZE_1,
} blc_esls_sensorInformationSize_t;

typedef struct {
    blc_esls_sensorInformationSize_t size;
    union {
        u16 sensorType0;
        u32 sensorType1;
    };
} blc_esls_sensorInformation_t;

typedef enum {
    BLC_ESLS_LED_INFORMATION_SRGB = 0x00,
    BLC_ESLS_LED_INFORMATION_MONOCHROME,
} blc_esls_ledInformationType_t;

typedef struct {
    u8 type : 2;
    u8 blue : 2;
    u8 green : 2;
    u8 red : 2;
} blc_esls_ledInformation_t;

typedef struct{
    u16 eslAddressHdl;
    u16 apSyncKetMaterialHdl;
    u16 eslResponseKeyMaterialHdl;
    u16 eslCurrentAbsoluteTimeHdl;
    u16 eslDisplayInformationHdl;
    u16 eslImageInformationHdl;
    u16 eslSensorInformationHdl;
    u16 eslLedInformationHdl;
    u16 eslControlPointHdl;
    u8 eslId;
    u8 reserved;
    eslsCallback_t cb;
} blc_esls_server_t;

typedef struct blc_eslp_server_ctrl{
    blc_prf_proc_t process;
    blc_esls_server_t eslpServer;
} blc_eslp_server_ctrl_t;

typedef struct {
    u8 displayDataNum;
    const blc_esls_displayData_t *displayData;
    u8 maxImageIndex;
    u8 sensorInformationsNum;
    const blc_esls_sensorInformation_t *sensorInfo;
    u8 ledInformationsNum;
    const blc_esls_ledInformation_t *ledInfo;
} blc_eslss_regParam_t;
