/********************************************************************************************************
 * @file    svc_ota.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "stack/ble/ble.h"

#define OTA_START_HDL SERVICE_TELINK_OTA_HDL

static const u8 serviceOtaUuid[16] = {TELINK_OTA_UUID_SERVICE};

static const u8 otaOutUuid[16] = {TELINK_SPP_DATA_OTA};
//// OTA attribute values

static const u8  otaData    = 0x00;
static const u16 otaDataLen = sizeof(otaData);

static const u8  otaOutDesc[]  = {'O', 'T', 'A'};
static const u16 otaOutDescLen = sizeof(otaOutDesc);

/*
 * @brief the structure for default OTA(Telink) service List.
 */
static const atts_attribute_t otaList[] =
    {
        ATTS_PRIMARY_SERVICE_128(serviceOtaUuid),

        ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadWriteWithoutNotify),
        {ATT_PERMISSIONS_RDWR, ATT_128_UUID_LEN, (u8 *)(size_t)&otaOutUuid[0], (u16 *)(size_t)&otaDataLen, sizeof(otaData), (u8 *)(size_t)&otaData, ATTS_SET_WRITE_CBACK},
        ATTS_COMMON_CCC_DEFINE,

        {
                                          ATT_PERMISSIONS_READ,
                                          ATT_16_UUID_LEN,
                                          (u8 *)(size_t)descriptorCharacteristicUserDescriptionUuid,
                                          (u16 *)(size_t)&otaOutDescLen,
                                          sizeof(otaOutDesc),
                                          (u8 *)(size_t)otaOutDesc,
                                          0,
                                          },
};

/*
 * @brief the structure for default OTA(Telink) service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcOtaGroup =
    {
        NULL,
        otaList,
        NULL,
        NULL,
        OTA_START_HDL,
        0};

static int telinkOtaWrite(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    u8                    buff[517 + 6]; //Max MTU size
    rf_packet_att_data_t *p = (rf_packet_att_data_t *)&buff[0];
    p->att                  = opcode;
    p->handle               = attrHandle;
    p->l2cap                = valueLen + 3;
    p->rf_len               = valueLen + 7;
    memcpy(p->dat, writeValue, valueLen);

    otaWrite(connHandle, p);
    return ATT_SUCCESS;
}

/**
 * @brief      for user add default OTA(Telink) service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addOtaGroup(void)
{
    blc_ota_initOtaServer_module();
    svcOtaGroup.endHandle  = svcOtaGroup.startHandle + ARRAY_SIZE(otaList) - 1;
    svcOtaGroup.writeCback = telinkOtaWrite;
    blc_gatts_addAttributeServiceGroup(&svcOtaGroup);
}

/**
 * @brief      for user remove default OTA(Telink) service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeOtaGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(OTA_START_HDL);
}
