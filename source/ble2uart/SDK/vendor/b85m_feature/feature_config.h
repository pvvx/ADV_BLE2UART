/********************************************************************************************************
 * @file	feature_config.h
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
#ifndef FEATURE_CONFIG_H_
#define FEATURE_CONFIG_H_





/////////////////// TEST FEATURE SELECTION /////////////////////////////////



//ble link layer test
#define	TEST_LL_MD										1   //link layer more data

#define TEST_LL_DLE										2   //link layer Data Length Extension

#define TEST_2M_CODED_PHY_CONNECTION					3

#define TEST_WHITELIST									4

#define TEST_SMP										5

#define TEST_GATT_API									6

#define TEST_EXT_ADV									7   //Extended ADV demo

#define TEST_EXT_SCAN									8   //Extended Scan demo

#define TEST_PER_ADV									9   //Periodic ADV demo

#define TEST_PER_ADV_SYNC								30	//Periodic ADV Sync demo

#define TEST_OTA										20

#define TEST_SOFT_TIMER                                 22

#define TEST_MISC_FUNC									190

#define TEST_FEATURE_BACKUP								200


#define FEATURE_TEST_MODE								TEST_LL_MD//TEST_OTA//TEST_FEATURE_BACKUP






#endif /* FEATURE_CONFIG_H_ */
