/********************************************************************************************************
 * @file    ots.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    10,2023
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
#pragma once


#include "ots_server_buf.h"
#include "ots_server_data.h"

#ifndef BLT_OTS_LOG
#define BLT_OTS_LOG(fmt, ...)           BLC_PROFILE_DEBUG(DBG_PRF_MASK_OTS_LOG, "[OTS]"fmt, ##__VA_ARGS__)
#endif

#define OTS_L2CAP_SPSM      ( 0x25 )

typedef struct __attribute__((packed))  {
    u8 objectId[6];
} blc_ots_object_id_t;

typedef enum {
    BLC_OTS_OACP_FEATURE_OACP_CREATE_OPCODE_SUPPORTED = 0x00000001,
    BLC_OTS_OACP_FEATURE_OACP_DELETE_OPCODE_SUPPORTED = 0x00000002,
    BLC_OTS_OACP_FEATURE_OACP_CALCULATE_CHECKSUM_OPCODE_SUPPORTED = 0x00000004,
    BLC_OTS_OACP_FEATURE_OACP_EXECUTE_OPCODE_SUPPORTED = 0x00000008,
    BLC_OTS_OACP_FEATURE_OACP_READ_OPCODE_SUPPORTED = 0x00000010,
    BLC_OTS_OACP_FEATURE_OACP_WRITE_OPCODE_SUPPORTED = 0x00000020,
    BLC_OTS_OACP_FEATURE_APPENDING_ADD_DATA_TO_OBJECTS_SUPPORTED = 0x00000040,
    BLC_OTS_OACP_FEATURE_TRUNCATION_OF_OBJECTS_SUPPORTED = 0x00000080,
    BLC_OTS_OACP_FEATURE_PATCHING_OF_OBJECTS_SUPPORTED = 0x00000100,
    BLC_OTS_OACP_FEATURE_OACP_ABORT_OPCODE_SUPPORTED = 0x00000200,
} blc_ots_oacp_feature_t;

typedef enum {
    BLC_OTS_OLCP_FEATURE_OLCP_GO_TO_OPCODE_SUPPORTED = 0x00000001,
    BLC_OTS_OLCP_FEATURE_OLCP_ORDER_OPCODE_SUPPORTED = 0x00000002,
    BLC_OTS_OLCP_FEATURE_OLCP_REQUEST_NUMBER_OF_OBJECTS_OPCODE_SUPPORTED = 0x00000004,
    BLC_OTS_OLCP_FEATURE_OLCP_CLEAR_MARKING_OPCODE_SUPPORTED = 0x00000008,
} blc_ots_olcp_feature_t;

typedef enum {
    BLC_OTS_OBJECT_PROPERTIES_DELETE = 0x00000001,
    BLC_OTS_OBJECT_PROPERTIES_EXECUTE = 0x00000002,
    BLC_OTS_OBJECT_PROPERTIES_READ = 0x00000004,
    BLC_OTS_OBJECT_PROPERTIES_WRITE = 0x00000008,
    BLC_OTS_OBJECT_PROPERTIES_APPEND = 0x00000010,
    BLC_OTS_OBJECT_PROPERTIES_TRUNCATE = 0x00000020,
    BLC_OTS_OBJECT_PROPERTIES_PATCH = 0x00000040,
    BLC_OTS_OBJECT_PROPERTIES_MARK = 0x00000080,
} blc_ots_object_properties_t;

typedef struct __attribute__((packed)) {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
} blc_ots_utc_t;

typedef enum {
    BLC_OTS_FILTER_NO_FILTER = 0x00,
    BLC_OTS_FILTER_NAME_STARTS_WITH = 0x01,
    BLC_OTS_FILTER_NAME_ENDS_WITH = 0x02,
    BLC_OTS_FILTER_NAME_CONTAINS = 0x03,
    BLC_OTS_FILTER_NAME_IS_EXACTLY = 0x04,
    BLC_OTS_FILTER_OBJECT_TYPE = 0x05,
    BLC_OTS_FILTER_CREATED_BETWEEN = 0x06,
    BLC_OTS_FILTER_MODIFIED_BETWEEN = 0x07,
    BLC_OTS_FILTER_CURRENT_SIZE_BETWEEN = 0x08,
    BLC_OTS_FILTER_ALLOCATED_SIZE_BETWEEN = 0x09,
    BLC_OTS_FILTER_MARKED_OBJECTS = 0x0A,
} blc_ots_filter_t;

typedef struct __attribute__((packed)) {
    blc_ots_filter_t filter;
} blc_ots_object_filter_hdr_t;

typedef struct __attribute__((packed)) {
    blc_ots_object_filter_hdr_t hdr;
} blc_ots_object_filter_no_filter_t,    \
    blc_ots_object_filter_marked_objects_t;

typedef struct __attribute__((packed)) {
    blc_ots_object_filter_hdr_t hdr;
    u16 length;
    u8 data[0];
} blc_ots_object_filter_name_starts_with_t,     \
    blc_ots_object_filter_name_ends_with_t,     \
    blc_ots_object_filter_name_is_exactly_t,    \
    blc_ots_object_filter_name_contains_t;

typedef struct __attribute__((packed)) {
    blc_ots_object_filter_hdr_t hdr;
    uuid_t type;
} blc_ots_object_filter_object_type_t;

typedef struct __attribute__((packed)) {
    blc_ots_object_filter_hdr_t hdr;
    blc_ots_utc_t timestamp1;
    blc_ots_utc_t timestamp2;
} blc_ots_object_filter_created_between_t,  \
    blc_ots_object_filter_modified_between_t;

typedef struct __attribute__((packed)) {
    blc_ots_object_filter_hdr_t hdr;
    u32 size1;
    u32 size2;
} blc_ots_object_filter_current_size_between_t, \
    blc_ots_object_filter_allocated_size_between_t;

typedef struct __attribute__((packed)) {
    u32 oacpFeatures;
    u32 olcpFeatures;
} blc_ots_feature_t;

typedef struct __attribute__((packed)) {
    u32 currentSize;
    u32 allocatedSize;
} blc_ots_object_size_t;

typedef enum {
    BLC_OTS_OACP_OPCODE_CREATE = 0x01,
    BLC_OTS_OACP_OPCODE_DELETE = 0x02,
    BLC_OTS_OACP_OPCODE_CALCULATE_CHECKSUM = 0x03,
    BLC_OTS_OACP_OPCODE_EXECUTE = 0x04,
    BLC_OTS_OACP_OPCODE_READ = 0x05,
    BLC_OTS_OACP_OPCODE_WRITE = 0x06,
    BLC_OTS_OACP_OPCODE_ABORT = 0x07,
    BLC_OTS_OACP_OPCODE_RESPONSE = 0x60,
} blc_ots_oacp_opcode_t;

typedef enum {
    BLC_OTS_OACP_RESULT_CODE_SUCCESS = 0x01,
    BLC_OTS_OACP_RESULT_CODE_OP_CODE_NOT_SUPPORTED = 0x02,
    BLC_OTS_OACP_RESULT_CODE_INVALID_PARAMETER = 0x03,
    BLC_OTS_OACP_RESULT_CODE_INSUFFICIENT_RESOURCES = 0x04,
    BLC_OTS_OACP_RESULT_CODE_INVALID_OBJECT = 0x05,
    BLC_OTS_OACP_RESULT_CODE_CHANNEL_UNAVAILABLE = 0x06,
    BLC_OTS_OACP_RESULT_CODE_UNSUPPORTED_TYPE = 0x07,
    BLC_OTS_OACP_RESULT_CODE_PROCEDURE_NOT_PEMITTED = 0x08,
    BLC_OTS_OACP_RESULT_CODE_OBJECT_LOCKED = 0x09,
    BLC_OTS_OACP_RESULT_CODE_OPERATION_FAILED = 0x0A,
} blc_ots_oacp_result_code_t;

typedef enum {
    BLC_OTS_OACP_WRITE_MODE_TRUNCATE = 0x02,
} blc_ots_oacp_write_mode_t;

typedef struct __attribute__((packed)) {
    blc_ots_oacp_opcode_t opcode;
} blc_ots_oacp_cmd_hdr_t;

typedef struct __attribute__((packed)) {
    blc_ots_oacp_cmd_hdr_t hdr;
    u32 size;
    uuid_t type;
} blc_ots_oacp_create_t;

typedef struct __attribute__((packed)) {
    blc_ots_oacp_cmd_hdr_t hdr;
} blc_ots_oacp_delete_t,    \
    blc_ots_oacp_abort_t;

typedef struct __attribute__((packed)) {
    blc_ots_oacp_cmd_hdr_t hdr;
    u32 offset;
    u32 len;
} blc_ots_oacp_calculate_checksum_t,    \
    blc_ots_oacp_read_t;

typedef struct __attribute__((packed)) {
    blc_ots_oacp_cmd_hdr_t hdr;
    u16 len;
    u8 data[];
} blc_ots_oacp_execute_t;

typedef struct __attribute__((packed)) {
    blc_ots_oacp_cmd_hdr_t hdr;
    u32 offset;
    u32 len;
    u8 mode;
} blc_ots_oacp_write_t;

typedef struct __attribute__((packed)) {
    u8 opCode;
    u8 requestedOpCode;
    u8 resultCode;
    u8 params[];
} blc_otsc_oacp_response_hdr_t;

typedef enum {
    BLC_OTS_OLCP_OPCODE_FIRST = 0x01,
    BLC_OTS_OLCP_OPCODE_LAST = 0x02,
    BLC_OTS_OLCP_OPCODE_PREVIOUS = 0x03,
    BLC_OTS_OLCP_OPCODE_NEXT = 0x04,
    BLC_OTS_OLCP_OPCODE_GOTO = 0x05,
    BLC_OTS_OLCP_OPCODE_ORDER = 0x06,
    BLC_OTS_OLCP_OPCODE_REQUEST_NUMBER_OF_OBJECTS = 0x07,
    BLC_OTS_OLCP_OPCODE_CLEAR_MARKING = 0x08,
    BLC_OTS_OLCP_OPCODE_RESPONSE = 0x70,
} blc_ots_olcp_opcode_t;

typedef enum {
    BLC_OTS_OLCP_RESULT_CODE_SUCCESS = 0x01,
    BLC_OTS_OLCP_RESULT_CODE_OP_CODE_NOT_SUPPORTED = 0x02,
    BLC_OTS_OLCP_RESULT_CODE_INVALID_PARAMETER = 0x03,
    BLC_OTS_OLCP_RESULT_CODE_OPERATION_FAILED = 0x04,
    BLC_OTS_OLCP_RESULT_CODE_OUT_OF_BOUNDS = 0x05,
    BLC_OTS_OLCP_RESULT_CODE_TOO_MANY_OBJECTS = 0x06,
    BLC_OTS_OLCP_RESULT_CODE_NO_OBJECT = 0x07,
    BLC_OTS_OLCP_RESULT_CODE_OBJECT_ID_NOT_FOUND = 0x08,
} blc_ots_olcp_result_code_t;

typedef enum {
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_NAME_ASC = 0x01,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_TYPE_ASC = 0x02,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_CURRENT_SIZE_ASC = 0x03,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_FIRST_CREATED_TIMESTAMP_ASC = 0x04,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_LAST_MODIFIED_TIMESTAMP_ASC = 0x05,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_NAME_DESC = 0x11,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_TYPE_DESC = 0x12,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_CURRENT_SIZE_DESC = 0x13,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_FIRST_CREATED_TIMESTAMP_DESC = 0x14,
    BLC_OTS_OLCP_LIST_SORT_ORDER_BY_OBJECT_LAST_MODIFIED_TIMESTAMP_DESC = 0x15,
} blc_ots_olcp_list_sort_order_t;

typedef enum {
    BLC_OTS_APPLICATION_ERROR_CODE_WRITE_REQUEST_REJECTED = 0x80,
    BLC_OTS_APPLICATION_ERROR_CODE_OBJECT_NOT_SELECTED = 0x81,
    BLC_OTS_APPLICATION_ERROR_CODE_CONCURRENCY_LIMIT_EXCEEDED = 0x82,
    BLC_OTS_APPLICATION_ERROR_CODE_OBJECT_NAME_ALREAD_EXISTS = 0x83,
} blc_ots_application_error_code_t;

typedef struct __attribute__((packed)) {
    u8 opcode;  //blc_ots_olcp_opcode_t
} blc_ots_olcp_cmd_hdr_t;

typedef struct __attribute__((packed)) {
    blc_ots_olcp_cmd_hdr_t hdr;
} blc_ots_olcp_first_t,                         \
    blc_ots_olcp_last_t,                        \
    blc_ots_olcp_previous_t,                    \
    blc_ots_olcp_next_t,                        \
    blc_ots_olcp_request_number_of_objects_t,   \
    blc_ots_olcp_clear_marking_t;

typedef struct __attribute__((packed)) {
    blc_ots_olcp_cmd_hdr_t hdr;
    blc_ots_object_id_t id;
} blc_ots_olcp_goto_t;

typedef struct __attribute__((packed)) {
    blc_ots_olcp_cmd_hdr_t hdr;
    blc_ots_olcp_list_sort_order_t order;
} blc_ots_olcp_order_t;

typedef struct __attribute__((packed)) {
    u8 opCode;
    u8 requestedOpCode;
    u8 resultCode;
    u8 params[];
} blc_otsc_olcp_response_hdr_t;

typedef enum {
    BLC_OTS_OBJECT_RECORD_TYPE_UUID_128 = 0x01,
    BLC_OTS_OBJECT_RECORD_CURRENT_SIZE_PRESENT = 0x02,
    BLC_OTS_OBJECT_RECORD_ALLOCATED_SIZE_PRESENT = 0x04,
    BLC_OTS_OBJECT_RECORD_FIRST_CREATED_PRESENT = 0x08,
    BLC_OTS_OBJECT_RECORD_LAST_MODIFIED_PRESENT = 0x10,
    BLC_OTS_OBJECT_RECORD_PROPERTIES_PRESENT = 0x20,
    BLC_OTS_OBJECT_RECORD_EXTENDED_FLAGS_PRESENT = 0x80,
} blc_ots_object_record_t;

static inline bool blc_ots_object_id_equal(const blc_ots_object_id_t *a, const blc_ots_object_id_t *b)
{
    return !memcmp(a->objectId, b->objectId, sizeof(a->objectId));
}

#ifndef OTSS_L2CAP_MTU
#define OTSS_L2CAP_MTU  ( 0x80 )
#endif


//OTS Server Event ID
typedef enum {
    ESL_EVT_OTSS_START = ESL_EVT_TYPE_OTSS,
} esl_otss_evt_enum;

typedef u16 (*blc_otss_object_read_cb_t)(blc_ots_object_id_t *id, u16 offset, u16 length, u8 **outData);
typedef u16 (*blc_otss_object_write_cb_t)(blc_ots_object_id_t *id, u8 mode, u16 offset, u16 length, u8 *data);
typedef bool (*blc_otss_object_append_additional_data_cb_t)(blc_ots_object_id_t *id, u16 newAllocatedLength);

typedef struct {
    blc_otss_object_read_cb_t read_cb;
    blc_otss_object_write_cb_t write_cb;
    blc_otss_object_append_additional_data_cb_t append_cb;
} blc_otss_object_callbacks_t;

/**
 * @brief           Registers the OTS control server.
 * @param[in]       param - Pointer to the registration parameters for the OTS control server.
 * @return          None
 */
void blc_esl_registerOTSControlServer(const blc_otss_regParam_t *param);

/**
 * @brief           Registers callbacks for OTS object operations.
 * @param[in]       callbacks - Pointer to the structure containing the callback functions for OTS object operations.
 * @return          None
 */
void blc_otss_registerCallbacks(const blc_otss_object_callbacks_t *callbacks);

/**
 * @brief           Adds an object to the OTS service.
 * @param[in]       size - Pointer to the structure containing the object size.
 * @param[in]       type - Pointer to the UUID representing the object type.
 * @param[in]       properties - The properties of the object, represented as a bitmask.
 * @param[out]      id - Pointer to store the object ID.
 * @return          ble_sts_t - Status of the operation.
 */
ble_sts_t blc_otss_objectAdd(blc_ots_object_size_t *size, uuid_t *type, u32 properties, blc_ots_object_id_t *id);

/**
 * @brief           Sets the name of the OTS object.
 * @param[in]       id - Pointer to the object ID whose name is to be set.
 * @param[in]       name - Pointer to the buffer containing the name of the object.
 * @param[in]       length - The length of the name.
 * @return          ble_sts_t - Status of the operation.
 */
ble_sts_t blc_otss_objectSetName(blc_ots_object_id_t *id, u8 *name, u16 length);

/**
 * @brief           Callback for handling host events.
 * @param[in]       h - The event handle.
 * @param[in]       para - Pointer to the event parameters.
 * @param[in]       n - The number of event parameters.
 * @return          int - Status of the callback.
 */
int blc_otss_hostEventCallback(u32 h, u8 *para, int n);

