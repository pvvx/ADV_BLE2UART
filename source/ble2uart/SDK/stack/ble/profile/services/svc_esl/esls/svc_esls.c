/********************************************************************************************************
 * @file    svc_esls.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    07,2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "stack/ble/ble.h"

#define ESLS_START_HDL                  SERVICE_ESLS_HDL

#define ESL_CONTROL_POINT_SIZE          64

const u16 eslsEslDisplayInformationMaxSize = ESLS_DISPLAYS_SUPPORTED * 5;
const u16 eslsEslSensorInformationMaxSize = ESLS_SENSORS_SUPPORTED * 5;
const u16 eslsEslLedInformationMaxSize = ESLS_LEDS_SUPPORTED;

#if ESLS_DISPLAYS_SUPPORTED
_attribute_ble_data_retention_ u8 eslsEslDisplayInformationValue[ESLS_DISPLAYS_SUPPORTED * 5];
_attribute_ble_data_retention_ u16 eslsEslDisplayInformationValueLen;

_attribute_ble_data_retention_ u8 eslsEslImageInformationValue = 0;
static const u16 eslsEslImageInformationValueLen = sizeof(u8);
#endif

#if ESLS_SENSORS_SUPPORTED
_attribute_ble_data_retention_ u8 eslsEslSensorInformationValue[ESLS_SENSORS_SUPPORTED * 5];
_attribute_ble_data_retention_ u16 eslsEslSensorInformationValueLen;
#endif

#if ESLS_LEDS_SUPPORTED
_attribute_ble_data_retention_ u8 eslsEslLedInformationValue[ESLS_LEDS_SUPPORTED];
_attribute_ble_data_retention_ u16 eslsEslLedInformationValueLen;
#endif

_attribute_ble_data_retention_ u8 eslsEslControlPointValue[ESL_CONTROL_POINT_SIZE];
_attribute_ble_data_retention_ u16 eslsEslControlPointValueLen;
const u16 eslsEslControlPointMaxSize = sizeof(eslsEslControlPointValue);
static const u8 eslsEslControlPointCCC[2] = {0,0};
static const u16 eslsEslControlPointCCCLen = sizeof(eslsEslControlPointCCC);

static const atts_attribute_t eslsList[] =
{
    ATTS_PRIMARY_SERVICE(serviceElectronicShelfLabelUuid),

    //ESL Address
    ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWrite, characteristicEslAddressUuid),

    //AP Sync Key Material
    ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWrite, characteristicApSyncKeyMaterialUuid),

    //ESL Response Key Material
    ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWrite, characteristicEslResponseKeyMaterialUuid),

    //ESL Current Absolute Time
    ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWrite, characteristicEslCurrentAbsoluteTimeUuid),

#if ESLS_DISPLAYS_SUPPORTED
    //ESL Display Information
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicEslDisplayInformationUuid, eslsEslDisplayInformationValue),

    //ESL Image Information
    ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicEslImageInformationUuid, eslsEslImageInformationValue),
#endif

#if ESLS_SENSORS_SUPPORTED
    //ESL Sensor Information
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicEslSensorInformationUuid, eslsEslSensorInformationValue),
#endif

#if ESLS_LEDS_SUPPORTED
    //ESL LED Information
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicEslLedInformationUuid, eslsEslLedInformationValue),
#endif

    ATTS_CHAR_UUID_ENCR_WRITE_POINT_CB(charPropWriteWriteWithoutNotify, characteristicEslControlPointUuid, eslsEslControlPointValue),
    ATTS_CCC_DEFINE(eslsEslControlPointCCC),
};

/* ESLS group structure */
_attribute_ble_data_retention_ static atts_group_t svcEslsGroup =
{
    NULL,
    eslsList,
    NULL,
    NULL,
    ESLS_START_HDL,
    0,
};

void blc_svc_addEslsGroup(void)
{
    svcEslsGroup.endHandle = svcEslsGroup.startHandle+ARRAY_SIZE(eslsList)-1;
    blc_gatts_addAttributeServiceGroup(&svcEslsGroup);
}

void blc_svc_removeEslsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(ESLS_START_HDL);
}

void blc_svc_eslsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcEslsGroup.readCback = readCback;
    svcEslsGroup.writeCback = writeCback;
}


