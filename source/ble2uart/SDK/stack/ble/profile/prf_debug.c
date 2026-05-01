/********************************************************************************************************
 * @file    prf_debug.c
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
#include "prf_debug.h"

#define _XXXX1  _YYYY,

#define Z_IS_ENABLED3(ignore_this, val, ...) val

#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args 1, 0)

#define Z_IS_ENABLED1(config_macro) Z_IS_ENABLED2(_XXXX##config_macro)

#define IS_ENABLED(config_macro) Z_IS_ENABLED1(config_macro)

/////////////////////////Profile Debug Log Macro Control////////////////////////////
const unsigned char DBG_PRF_MASK_COMMON_LOG = IS_ENABLED(PRF_DBG_COMMON_EN);


/////////////////////////Bluetooth Basic Profile Debug Log//////////////////////////
const unsigned char DBG_PRF_MASK_DIS_LOG = IS_ENABLED(PRF_DBG_DIS_EN);
const unsigned char DBG_PRF_MASK_BAS_LOG = IS_ENABLED(PRF_DBG_BAS_EN);
const unsigned char DBG_PRF_MASK_SCPS_LOG = IS_ENABLED(PRF_DBG_SCPS_EN);

/////////////////////////Bluetooth LE Audio Debug Log////////////////////////////////
const unsigned char DBG_PRF_MASK_AUD_LOG  = IS_ENABLED(PRF_DBG_AUDIO_EN);
const unsigned char DBG_PRF_MASK_ASCS_LOG = IS_ENABLED(PRF_DBG_ASCS_EN);
const unsigned char DBG_PRF_MASK_BASS_LOG = IS_ENABLED(PRF_DBG_BASS_EN);
const unsigned char DBG_PRF_MASK_PACS_LOG = IS_ENABLED(PRF_DBG_PACS_EN);
const unsigned char DBG_PRF_MASK_BAP_LOG = IS_ENABLED(PRF_DBG_BAP_EN);
const unsigned char DBG_PRF_MASK_BCST_LOG = IS_ENABLED(PRF_DBG_BCST_EN);
const unsigned char DBG_PRF_MASK_CSIS_LOG = IS_ENABLED(PRF_DBG_CSIS_EN);
const unsigned char DBG_PRF_MASK_OTS_LOG = IS_ENABLED(PRF_DBG_OTS_EN);
const unsigned char DBG_PRF_MASK_MCS_LOG = IS_ENABLED(PRF_DBG_MCS_EN);
const unsigned char DBG_PRF_MASK_TBS_LOG = IS_ENABLED(PRF_DBG_TBS_EN);
const unsigned char DBG_PRF_MASK_VCS_LOG = IS_ENABLED(PRF_DBG_VCS_EN);
const unsigned char DBG_PRF_MASK_VOCS_LOG = IS_ENABLED(PRF_DBG_VOCS_EN);
const unsigned char DBG_PRF_MASK_MICS_LOG = IS_ENABLED(PRF_DBG_MICS_EN);
const unsigned char DBG_PRF_MASK_AICS_LOG = IS_ENABLED(PRF_DBG_AICS_EN);
const unsigned char DBG_PRF_MASK_CAP_LOG = IS_ENABLED(PRF_DBG_CAP_EN);
const unsigned char DBG_PRF_MASK_PBP_LOG = IS_ENABLED(PRF_DBG_PBP_EN);
const unsigned char DBG_PRF_MASK_TMAS_LOG = IS_ENABLED(PRF_DBG_TMAS_EN);
const unsigned char DBG_PRF_MASK_HAS_LOG = IS_ENABLED(PRF_DBG_HAS_EN);
const unsigned char DBG_PRF_MASK_UCP_LOG = IS_ENABLED(PRF_DBG_UCP_EN);
const unsigned char DBG_PRF_MASK_STORE_LOG = IS_ENABLED(PRF_DBG_STORE_EN);


/////////////////////////Channel Sounding(HADM) Debug Log////////////////////////////
const unsigned char DBG_PRF_MASK_RAS_LOG = IS_ENABLED(PRF_DBG_RAS_EN);

/////////////////////////Electronic Shelf Label(ESL) Debug Log///////////////////////
/////////////////////////Ultra Low Latency(ULL) HID Debug Log///////////////////////
const unsigned char DBG_PRF_MASK_HID_LOG = IS_ENABLED(PRF_DBG_HID_EN);
const unsigned char DBG_PRF_MASK_ULL_HID_LOG = IS_ENABLED(PRF_DBG_ULLHID_EN);

/////////////////////////Test Profile Debug Log////////////////////////////////////

const unsigned char DBG_PRF_MASK_TEST_PRF_LOG = IS_ENABLED(PRF_DBG_TEST_PRF_EN);


