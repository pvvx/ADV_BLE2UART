/********************************************************************************************************
 * @file	app_config.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions
 *              in binary form must reproduce the above copyright notice, this list of
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *
 *              3. Neither the name of TELINK, nor the names of its contributors may be
 *              used to endorse or promote products derived from this software without
 *              specific prior written permission.
 *
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or
 *              relating to such deletion(s), modification(s) or alteration(s).
 *
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************************************/
#pragma once


#include "feature_config.h"






#if(FEATURE_TEST_MODE == TEST_LL_MD)
	#include "feature_ll_more_data/app_config.h"
#elif(FEATURE_TEST_MODE == TEST_CSA2)
	#include "feature_csa2/app_config.h"
#elif(FEATURE_TEST_MODE == TEST_2M_CODED_PHY_CONNECTION)
	#include "feature_2M_coded_phy/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_LL_DLE)
	#include "feature_dle/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_WHITELIST)
	#include "feature_whitelist/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_GATT_API)
	#include "feature_gatt_api/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_L2CAP_CONN_PARAM_UPDATE)
	#include "feature_l2cap_conn_param_update/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_EXT_ADV)
	#include "feature_ext_adv/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PER_ADV)
	#include "feature_per_adv/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PER_ADV_SYNC)
	#include "feature_per_adv_sync/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_EXT_SCAN)
	#include "feature_ext_scan/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_MISC_FUNC)
	#include "feature_misc/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_OTA)
	#include "feature_ota/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_SOFT_TIMER)
	#include "feature_soft_timer/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_FEATURE_BACKUP)
	#include "feature_backup/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_SMP)
   #include "feature_smp/app_config.h"
#endif


