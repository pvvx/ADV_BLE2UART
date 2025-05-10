/********************************************************************************************************
 * @file    sdk_version.h
 *
 * @brief   This is the header file for B85m
 *
 * @author  Driver Group
 * @date    2022
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

/*
 * It is only used by SDK, and the customer is not allowed to modify it
 */

// Version : CERTIFICATION_MARK.SOFT_STRUCTURE.Major.Minor Patch
#define	CERTIFICATION_MARK			4
#define	SOFT_STRUCTURE				0
#define	MAJOR_VERSION				2
#define	MINOR_VERSION				0
#define	PATCH_NUM					0

#define CUSTOM_MAJOR_VERSION		0
#define CUSTOM_MINOR_VERSION		0

/*
 * Release Tool need to change this macro to match the release version,
 * the replace rules is: "$$$B85m_driver_sdk_"#sdk_version_num"$$$", The "#sdk_version_num"
 * will replace with this macro value.
 */
#define B85m_SDK_VERSION_NUM						CERTIFICATION_MARK.SOFT_STRUCTURE.MAJOR_VERSION.MINOR_VERSION

#define SDK_VERSION_NUM								B85m_SDK_VERSION_NUM

#define	SDK_VERSION1(sdk_version_num)				"$$$b85m_ble_multi_connection_sdk_V"#sdk_version_num"$$$"
#define	SDK_VERSION(sdk_version_num)				SDK_VERSION1(sdk_version_num)

#define	PATCH_VERSION1(patch_num)					"$$$patch_"#patch_num"$$$"
#define	PATCH_VERSION(patch_num)					PATCH_VERSION1(patch_num)

#define CUSTOM_VERSION_NUM							CUSTOM_MAJOR_VERSION.CUSTOM_MINOR_VERSION

#define	CUSTOM_VERSION1(cunstom_version_num)		"$$$custom_"#cunstom_version_num"$$$"
#define	CUSTOM_VERSION(cunstom_version_num)			CUSTOM_VERSION1(cunstom_version_num)



