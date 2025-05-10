/********************************************************************************************************
 * @file    user_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#if(__PROJECT_ACL_PER_DEMO__)
	#include "vendor/acl_peripheral_demo/app_config.h"
#elif(__PROJECT_C1P1_DEMO__)
	#include "vendor/acl_c1p1_demo/app_config.h"
#elif(__PROJECT_CONTROLLER__)
	#include "vendor/ble_controller/app_config.h"
#elif(__PROJECT_ACL_CONN_DEMO__)
	#include "vendor/acl_connection_demo/app_config.h"
#elif(__PROJECT_FEATURE_TEST__)
	#include "vendor/feature_test/app_config.h"
#elif(__PROJECT_ACL_CEN_DEMO__)
	#include "vendor/acl_central_demo/app_config.h"
#elif(__PROJECT_REMOTE__)
	#include "vendor/ble_remote/app_config.h"
#else
	#include "vendor/common/default_config.h"
#endif

