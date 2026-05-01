/********************************************************************************************************
 * @file    ots_server_data.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    04,2024
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

#pragma once

#ifndef OTS_SERVER_MAX_OBJECT_NAME_LENGTH
#define OTS_SERVER_MAX_OBJECT_NAME_LENGTH   (0x20)
#endif

#ifndef OTS_SERVER_MAX_OBJECTS_NUM
#define OTS_SERVER_MAX_OBJECTS_NUM ( 0x30 )
#endif

#ifndef OTS_SERVER_OBJECT_ID_START
#define OTS_SERVER_OBJECT_ID_START  ( 0x100 )
#endif

struct ots_server_data_object;

/**
 * @brief           This function initializes the OTS server data.
 * @return          void
 */
void ots_server_data_init(void);

/**
 * @brief           This function retrieves the object ID of a given OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @return          u64 - The object ID.
 */
u64 ots_server_data_get_object_id(struct ots_server_data_object *object);

/**
 * @brief           This function retrieves the OTS server data object by object ID.
 * @param[in]       id - The object ID.
 * @return          struct ots_server_data_object* - Pointer to the OTS server data object.
 */
struct ots_server_data_object *ots_server_data_get_object_by_object_id(u64 id);

/**
 * @brief           This function retrieves the first OTS server data object.
 * @return          struct ots_server_data_object* - Pointer to the first OTS server data object.
 */
struct ots_server_data_object *ots_server_data_get_first_object(void);

/**
 * @brief           This function retrieves the last OTS server data object.
 * @return          struct ots_server_data_object* - Pointer to the last OTS server data object.
 */
struct ots_server_data_object *ots_server_data_get_last_object(void);

/**
 * @brief           This function retrieves the next OTS server data object.
 * @param[in]       object - Pointer to the current OTS server data object.
 * @return          struct ots_server_data_object* - Pointer to the next OTS server data object.
 */
struct ots_server_data_object *ots_server_data_get_next_object(struct ots_server_data_object *object);

/**
 * @brief           This function retrieves the previous OTS server data object.
 * @param[in]       object - Pointer to the current OTS server data object.
 * @return          struct ots_server_data_object* - Pointer to the previous OTS server data object.
 */
struct ots_server_data_object *ots_server_data_get_prev_object(struct ots_server_data_object *object);

/**
 * @brief           This function creates a new OTS server data object with specified parameters.
 * @param[in]       allocatedSize - The allocated size of the object.
 * @param[in]       currentSize - The current size of the object.
 * @param[in]       type - Pointer to the type UUID of the object.
 * @param[in]       properties - The properties of the object.
 * @return          struct ots_server_data_object* - Pointer to the newly created OTS server data object.
 */
struct ots_server_data_object *ots_server_data_new_object(u32 allocatedSize, u32 currentSize, uuid_t *type, u32 properties);

/**
 * @brief           This function deletes a given OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object to be deleted.
 * @return          bool - True if the object was successfully deleted, false otherwise.
 */
bool ots_server_data_delete_object(struct ots_server_data_object *object);

/**
 * @brief           This function sets the name of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[in]       name - Pointer to the name string.
 * @param[in]       nameLen - The length of the name string.
 * @return          bool - True if the name was successfully set, false otherwise.
 */
bool ots_server_data_set_name(struct ots_server_data_object *object, u8 *name, u16 nameLen);

/**
 * @brief           This function retrieves the name of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[out]      name - Pointer to a pointer where the name will be stored.
 * @param[out]      nameLen - Pointer to store the length of the name.
 * @return          bool - True if the name was successfully retrieved, false otherwise.
 */
bool ots_server_data_get_name(struct ots_server_data_object *object, u8 **name, u16 *nameLen);

/**
 * @brief           This function sets the size of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[in]       allocatedSize - The allocated size of the object.
 * @param[in]       currentSize - The current size of the object.
 * @return          bool - True if the size was successfully set, false otherwise.
 */
bool ots_server_data_set_size(struct ots_server_data_object *object, u32 allocatedSize, u32 currentSize);

/**
 * @brief           This function retrieves the size of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[out]      allocatedSize - Pointer to store the allocated size of the object.
 * @param[out]      currentSize - Pointer to store the current size of the object.
 * @return          bool - True if the size was successfully retrieved, false otherwise.
 */
bool ots_server_data_get_size(struct ots_server_data_object *object, u32 *allocatedSize, u32 *currentSize);

/**
 * @brief           This function retrieves the properties of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[out]      properties - Pointer to store the properties of the object.
 * @return          bool - True if the properties were successfully retrieved, false otherwise.
 */
bool ots_server_data_get_properties(struct ots_server_data_object *object, u32 *properties);

/**
 * @brief           This function retrieves the type of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[out]      type - Pointer to the UUID of the object type.
 * @return          bool - True if the type was successfully retrieved, false otherwise.
 */
bool ots_server_data_get_type(struct ots_server_data_object *object, uuid_t *type);

/**
 * @brief           This function sets the length of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[in]       allocatedLen - The allocated length of the object.
 * @param[in]       currentLen - The current length of the object.
 * @return          bool - True if the length was successfully set, false otherwise.
 */
bool ots_server_data_set_length(struct ots_server_data_object *object, u32 allocatedLen, u32 currentLen);

/**
 * @brief           This function retrieves the length of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[out]      allocatedLen - Pointer to store the allocated length of the object.
 * @param[out]      currentLen - Pointer to store the current length of the object.
 * @return          bool - True if the length was successfully retrieved, false otherwise.
 */
bool ots_server_data_get_length(struct ots_server_data_object *object, u32 *allocatedLen, u32 *currentLen);

/**
 * @brief           This function sets the locked state of the OTS server data object.
 * @param[in]       object - Pointer to the OTS server data object.
 * @param[in]       connHandle - The connection handle.
 * @param[in]       locked - The locked state to set (true for locked, false for unlocked).
 * @return          bool - True if the locked state was successfully set, false otherwise.
 */
bool ots_server_data_set_locked(struct ots_server_data_object *object, u16 connHandle, bool locked);
