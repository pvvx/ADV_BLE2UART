/********************************************************************************************************
 * @file    esls.h
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

#include "esls_server_buf.h"

/******************************* ESLS Common Start **********************************************************************/

#define BLT_ESLS_LOG(fmt, ...)              BLC_PROFILE_DEBUG(DBG_PRF_MASK_ESLS_LOG, "[ESLS]"fmt, ##__VA_ARGS__)

typedef struct {
    u8 eslId;
    u8 groupId : 7;
} blc_esls_eslAddress_t;

typedef struct {
    u8 sessionKey[16];
    u8 IV[8];
} blc_esls_keyMaterial_t;

typedef enum {
    BLC_ESLS_STATE_UNASSOCIATED,
    BLC_ESLS_STATE_CONFIGURING,
    BLC_ESLS_STATE_SYNCHRONIZED,
    BLC_ESLS_STATE_UPDATING,
    BLC_ESLS_STATE_UNSYNCHRONIZED,
} blc_esls_state_t;

#define BLC_ESLS_ESL_ID_BROADCAST   0xFF

#define BLC_ESLS_LED_TYPE_Pos   6
#define BLC_ESLS_LED_TYPE_Msk   0xC0

#define BLC_ESLS_LED_BLUE_Pos   4
#define BLC_ESLS_LED_BLUE_Msk   0x30

#define BLC_ESLS_LED_GREEN_Pos  2
#define BLC_ESLS_LED_GREEN_Msk  0x0C

#define BLC_ESLS_LED_RED_Pos    0
#define BLC_ESLS_LED_RED_Msk    0x03

#define BLC_ESLS_LED_FIELD_SET(FIELD, VAL)    ((VAL << FIELD##_Pos) & FIELD##_Msk)
#define BLC_ESLS_LED_FIELD_GET(FIELD, VAL)    ((VAL & FIELD##_Msk) >> FIELD##_Pos)

// ESLS 3.9.1.1 Command opcodes: "The shortest possible TLV is 2 octets in size, and the longest is 17 octets."
#define BLC_ESLS_CMD_RSP_MIN_LENGTH 2
#define BLC_ESLS_CMD_RSP_MAX_LENGTH 17

typedef enum {
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_PING = 0x00,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UNASSOCIATE_FROM_AP = 0X01,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_SERVICE_RESET = 0x02,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_FACTORY_RESET = 0x03,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UPDATE_COMPLETE = 0x04,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_0 = 0x0F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_1 = 0x1F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_2 = 0x2F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_3 = 0x3F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_4 = 0x4F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_5 = 0x5F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_6 = 0x6F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_7 = 0x7F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_8 = 0x8F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_9 = 0x9F,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_A = 0xAF,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_B = 0xBF,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_C = 0xCF,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_D = 0xDF,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_E = 0xEF,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_F = 0xFF,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_READ_SENSOR_DATA = 0x10,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_REFRESH_DISPLAY = 0x11,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_IMAGE = 0x20,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_TIMED_IMAGE = 0x60,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_CONTROL = 0xB0,
    BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_TIMED_CONTROL = 0xF0,
} blc_eslss_ControlPointCommandOpcode_t;

typedef struct {
    u8 opcode;
    u8 eslId;
} blc_eslss_controlPointCommandHdr_t,
    blc_eslss_controlPointCommandPing_t,
    blc_eslss_controlPointCommandUnassociteFromAp_t,
    blc_eslss_controlPointCommandServiceReset_t,
    blc_eslss_controlPointCommandFactoryReset_t,
    blc_eslss_controlPointCommandUpdateComplete_t;

typedef struct {
    blc_eslss_controlPointCommandHdr_t hdr;
    u8 sensorId;
} blc_eslss_controlPointCommandReadSensorData_t;

typedef struct {
    blc_eslss_controlPointCommandHdr_t hdr;
    u8 displayId;
} blc_eslss_controlPointCommandRefreshDisplay_t;

typedef struct {
    blc_eslss_controlPointCommandHdr_t hdr;
    u8 displayId;
    u8 imageId;
} blc_eslss_controlPointCommandDisplayImage_t;

typedef struct {
    blc_eslss_controlPointCommandHdr_t hdr;
    u8 displayId;
    u8 imageId;
    u32 absoluteTime;
} blc_eslss_controlPointCommandDisplayTimedImage_t;

typedef struct {
    blc_eslss_controlPointCommandHdr_t hdr;
    u8 ledId;
    u8 colorRed : 2;
    u8 colorGreen : 2;
    u8 colorBlue : 2;
    u8 brightness : 2;
    u8 flashingPattern [7];
    u16 repeatType : 1;
    u16 repeatDuration : 15;
} blc_eslss_controlPointCommandLedControl_t;

typedef struct {
    blc_eslss_controlPointCommandHdr_t hdr;
    u8 ledId;
    u8 colorRed : 2;
    u8 colorGreen : 2;
    u8 colorBlue : 2;
    u8 brightness : 2;
    u8 flashingPattern [7];
    u16 repeatType : 1;
    u16 repeatDuration : 15;
    u32 absoluteTime;
} blc_eslss_controlPointCommandLedTimedControl_t;

typedef struct {
    blc_eslss_controlPointCommandHdr_t hdr;
    u8 parameters[];
} blc_eslss_controlPointCommandVendorSpecific_t;

typedef enum {
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_ERROR = 0x00,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_LED_STATE = 0x01,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_BASIC_STATE = 0x10,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_DISPLAY_STATE = 0x11,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_0 = 0x0E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_1 = 0x1E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_2 = 0x2E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_3 = 0x3E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_4 = 0x4E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_5 = 0x5E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_6 = 0x6E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_7 = 0x7E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_8 = 0x8E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_9 = 0x9E,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_A = 0xAE,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_B = 0xBE,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_C = 0xCE,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_D = 0xDE,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_E = 0xEE,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_F = 0xFE,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_0 = 0x0F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_1 = 0x1F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_2 = 0x2F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_3 = 0x3F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_4 = 0x4F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_5 = 0x5F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_6 = 0x6F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_7 = 0x7F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_8 = 0x8F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_9 = 0x9F,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_A = 0xAF,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_B = 0xBF,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_C = 0xCF,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_D = 0xDF,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_E = 0xEF,
    BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_F = 0xFF,
} blc_eslss_ControlPointResponseOpcode_t;

#define BLC_ESLSS_SENSOR_VALUE_OPCODE(SENSOR_VALUE_LENGTH)  \
        (BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_0 | (SENSOR_VALUE_LENGTH << 4))

//
#define BLC_ESLSS_VENDOR_SPECIFIC_RESPONSE_OPCODE(PAYLOAD_LENGTH)   \
        (BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_0 | ((PAYLOAD_LENGTH - 1) << 4))

typedef enum {
    BLC_ESLSS_ERROR_CODE_UNSPECIFIED = 0x01,
    BLC_ESLSS_ERROR_CODE_INVALID_OPCODE = 0x02,
    BLC_ESLSS_ERROR_CODE_INVALID_STATE = 0x03,
    BLC_ESLSS_ERROR_CODE_INVALID_IMAGE_INDEX = 0x04,
    BLC_ESLSS_ERROR_CODE_IMAGE_NOT_AVAILABLE = 0x05,
    BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS = 0x06,
    BLC_ESLSS_ERROR_CODE_CAPACITY_LIMIT = 0x07,
    BLC_ESLSS_ERROR_CODE_INSUFFICIENT_BATTERY = 0x08,
    BLC_ESLSS_ERROR_CODE_INSUFFICIENT_RESOURCES = 0x09,
    BLC_ESLSS_ERROR_CODE_RETRY = 0x0A,
    BLC_ESLSS_ERROR_CODE_QUEUE_FULL = 0x0B,
    BLC_ESLSS_ERROR_CODE_IMPLAUSIBLE_ABSOLUTE_TIME = 0x0C,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_0 = 0xF0,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_1 = 0xF1,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_2 = 0xF2,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_3 = 0xF3,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_4 = 0xF4,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_5 = 0xF5,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_6 = 0xF6,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_7 = 0xF7,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_8 = 0xF8,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_9 = 0xF9,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_A = 0xFA,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_B = 0xFB,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_C = 0xFC,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_D = 0xFD,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_E = 0xFE,
    BLC_ESLSS_ERROR_CODE_VENDOR_SPECIFIC_F = 0xFF,
} blc_eslss_errorCode_t;

typedef struct {
    u8 opcode;
    u8 parameters[];
} blc_eslss_controlPointResponseHdr_t;

typedef struct {
    blc_eslss_controlPointResponseHdr_t hdr;
    u8 error;
} blc_eslss_controlPointResponseError_t;

typedef struct {
    blc_eslss_controlPointResponseHdr_t hdr;
    u8 ledId;
} blc_eslss_controlPointResponseLedState_t;

typedef struct {
    blc_eslss_controlPointResponseHdr_t hdr;
    u16 serviceNeeded : 1;
    u16 synchronized : 1;
    u16 activeLed : 1;
    u16 pendingLedUpdate : 1;
    u16 pendingDisplayUpdate : 1;
} blc_eslss_controlPointResponseBasicState_t;

typedef struct {
    blc_eslss_controlPointResponseHdr_t hdr;
    u8 displayId;
    u8 imageId;
} blc_eslss_controlPointResponseDisplayState_t;

typedef struct {
    blc_eslss_controlPointResponseHdr_t hdr;
    u8 sensorId;
    u8 sensorData[];
} blc_eslss_controlPointResponseSensorValue_t;

typedef struct {
    blc_eslss_controlPointResponseHdr_t hdr;
    u8 parameters[];
} blc_eslss_controlPointResponseVendorSpecific_t;


/******************************* ESLS Common End **********************************************************************/

/******************************* ESLS Server Start **********************************************************************/

extern const blc_eslss_regParam_t defaultEslpsParam;

//ESLS Server Event ID
typedef enum{
    ESL_EVT_ESLSS_START = ESL_EVT_TYPE_ESLSS,
    ESL_EVT_ESLSS_ESL_ADDRESS,
    ESL_EVT_ESLSS_AP_SYNC_KEY_MATERIAL,
    ESL_EVT_ESLSS_ESL_RESPONSE_KEY_MATERIAL,
    ESL_EVT_ESLSS_ESL_CURRENT_ABSOLUTE_TIME,
    ESL_EVT_ESLSS_ESL_CONTROL_POINT_COMMAND,
    //NONE: TODO:
} esl_eslss_evt_enum;

typedef struct { //Event ID: ESL_EVT_ESLSS_ESL_ADDRESS
    blc_esls_eslAddress_t eslAddress;
} blc_eslss_eslAddressEvt_t;

typedef struct { //Event ID: ESL_EVT_ESLSS_AP_SYNC_KEY_MATERIAL
    blc_esls_keyMaterial_t apSyncKeyMaterial;
} blc_eslss_apSyncKeyMaterialEvt_t;

typedef struct { //Event ID: ESL_EVT_ESLSS_ESL_RESPONSE_KEY_MATERIAL
    blc_esls_keyMaterial_t eslResponseKeyMaterial;
} blc_eslss_eslResponseKeyMaterialEvt_t;

typedef struct { //Event ID: ESL_EVT_ESLSS_ESL_CURRENT_ABSOLUTE_TIME
    u32 eslCurrentAbsoluteTime;
} blc_eslss_eslCurrentAbsoluteTimeEvt_t;

typedef struct { //Event ID: ESL_EVT_ESLSS_ESL_CONTROL_POINT_COMMAND
    blc_eslss_controlPointCommandHdr_t cmd[1];
} bls_eslss_elsControlPointCommandEvt_t;


/**
 * @brief       This function serves to register ESLS Server function
 * @param[in]   param - refer to 'blc_eslss_regParam_t'
 * @return      none.
 */
void blc_esl_registerESLSControlServer(const blc_eslss_regParam_t *param);

/**
 * @brief      Set the callback function for ESL control operations.
 * @param[in]  cb - Callback function to be registered for ESL control operations.
 * @return     none
 */
void blc_eslss_setElectronicShelfLabelCback(eslsCallback_t cb);

/**
 * @brief      Set the ESL ID to accept or reject ESL commands.
 * @param[in]  connHandle - Connection handle.
 * @param[in]  elsId - ESL ID to be set.
 * @return     none
 */
void blc_eslss_setEslId(u16 connHandle, u8 elsId);

/**
 * @brief      Clear the ESL ID to stop accepting ESL commands.
 * @param[in]  connHandle - Connection handle.
 * @return     none
 */
void blc_eslss_clearEslId(u16 connHandle);

/**
 * @brief      Update the ESL display information.
 * @param[in]  connHandle - Connection handle.
 * @param[in]  displayDataNum - Number of display data items.
 * @param[in]  data - Array containing the display data.
 * @return     ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslss_updateEslDisplayInformation(u16 connHandle, u8 displayDataNum, const blc_esls_displayData_t *data);

/**
 * @brief      Update the ESL image information.
 * @param[in]  connHandle - Connection handle.
 * @param[in]  maxImageIndex - Maximum image index.
 * @return     ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslss_updateEslImageInformation(u16 connHandle, u8 maxImageIndex);

/**
 * @brief      Update the ESL sensor information.
 * @param[in]  connHandle - Connection handle.
 * @param[in]  sensorInformationsNum - Number of sensor information items.
 * @param[in]  info - Array containing the sensor information.
 * @return     ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslss_updateEslSensorInformation(u16 connHandle, u8 sensorInformationsNum, const blc_esls_sensorInformation_t *info);

/**
 * @brief      Update the ESL LED information.
 * @param[in]  connHandle - Connection handle.
 * @param[in]  ledInformationsNum - Number of LED information items.
 * @param[in]  info - Array containing the LED information.
 * @return     ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslss_updateEslLedInformation(u16 connHandle, u8 ledInformationsNum, const blc_esls_ledInformation_t *info);

/**
 * @brief      Update the ESL control point response.
 * @param[in]  connHandle - Connection handle.
 * @param[in]  rsp - Response header to be sent.
 * @return     ble_sts_t - Status of the operation.
 */
ble_sts_t blc_eslss_updateEslControlPointResponse(u16 connHandle, blc_eslss_controlPointResponseHdr_t *rsp);

/**
 * @brief      Get the size of the ESL command.
 * @param[in]  cmd - Pointer to the control point command header.
 * @return     u16 - The size of the command.
 */
u16 blc_esl_getCommandSize(blc_eslss_controlPointCommandHdr_t *cmd);

/**
 * @brief      Get the size of the ESL response.
 * @param[in]  rsp - Pointer to the control point response header.
 * @return     u16 - The size of the response.
 */
u16 blc_esl_getResponseSize(blc_eslss_controlPointResponseHdr_t *rsp);

/**
 * @brief      Parse the ESL payload TLVs.
 * @param[in]  val - Pointer to the ESL payload.
 * @param[in]  len - Length of the remaining ESL payload.
 * @param[out] cmd - Pointer to the parsed command.
 * @param[out] cmdLen - Length of the parsed command.
 * @param[out] rsp - Pointer to the parsed response.
 * @param[out] rspLen - Length of the parsed response.
 * @return     u16 - The number of parsed bytes.
 */
u16 blc_eslss_eslCommandPayloadParse(u8 *val, u16 len,
                    blc_eslss_controlPointCommandHdr_t *cmd, u16 *cmdLen,
                    blc_eslss_controlPointResponseHdr_t *rsp, u16 *rspLen);

/**
 * @brief      Parse the ESL payload and set the response slot.
 * @param[in]  val - Pointer to the ESL payload.
 * @param[in]  len - Length of the ESL payload.
 * @param[out] rspSlot - Pointer to the response slot.
 * @return     bool - True if the response slot is set, false otherwise.
 */
bool blc_eslss_eslCommandPayloadGetRspSlot(u8 *val, u16 len, u8 *rspSlot);

/**
 * @brief           This function is used to write ESL payload with responses
 * @param[in]       buffer - ESL payload buffer
 * @param[in]       length - ESL payload length
 * @param[in]       rsp - Response to be written
 * @return          u16 - Number of bytes written. If there is no room, it will return 0.
 */
u16 blc_eslss_eslResponsePayloadWrite(u8 *buffer, u16 length, blc_eslss_controlPointResponseHdr_t *rsp);

/******************************* ESLS Server End **********************************************************************/
