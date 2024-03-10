/********************************************************************************************************
 * @file	ble_config.h
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

//////////////////////////////////////////////////////////////////////////////
/**
 *  @brief  Definition for Device info
 */
#include "drivers.h"
#include "tl_common.h"

#define 	BQB_TEST_EN      										0
//BQB Test
#if BQB_TEST_EN
	#define BQB_TEST_PHY_EN         1
    #define BQB_TEST_CONN_UPD_EN    1
    #define BQB_TEST_CHN_MAP_EN     1
#else
    #define BQB_TEST_PHY_EN         0
    #define BQB_TEST_CONN_UPD_EN    0
    #define BQB_TEST_CHN_MAP_EN     0
#endif

//the following just for xiaomi project.
#define  	HCI_DFU_EN  			1
///

///////////////////  Feature ////////////////////////////
#ifndef		LL_MULTI_SLAVE_MAC_ENABLE
#define		LL_MULTI_SLAVE_MAC_ENABLE				    			0
#endif

//////////////////////////////////////////////////////////////////////
//note both the following two macro XXX_PRIVATE_XXX and XXX_NORMAL_XXX
//CAN NOT be set 1 at the same time.One is private,another is standard.
#ifndef		LL_FEATURE_PRIVATE_BIS_SYNC_RECEIVER
#define 	LL_FEATURE_PRIVATE_BIS_SYNC_RECEIVER					0
#endif

#ifndef		LL_FEATURE_NORMAL_BIS_SYNC_RECEIVER
#define		LL_FEATURE_NORMAL_BIS_SYNC_RECEIVER						1
#endif
//////////////////////////////////////////////////////////////////////


#if(MCU_CORE_TYPE == MCU_CORE_825x)
	#define	FIX_HW_CRC24_EN											1
	#define HW_ECDH_EN                                     			0
	#define ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN				1
#elif(MCU_CORE_TYPE == MCU_CORE_827x)
	#define	FIX_HW_CRC24_EN											0
	#define HW_ECDH_EN                                      		1
	#define ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN				1
#elif(MCU_CORE_TYPE == MCU_CORE_9518)
	#define	FIX_HW_CRC24_EN											0
	#define HW_ECDH_EN                                      		1
	#define ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN				0
#else
	#error "unsupported mcu type !"
#endif



#ifndef LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN
#define LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN					0
#endif

//Link layer feature enable flag default setting
#ifndef LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION
#define LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION					1
#endif

#ifndef LL_FEATURE_SUPPORT_LL_PRIVACY
#define LL_FEATURE_SUPPORT_LL_PRIVACY								0 //TODO: legAdv and slave role conn support now
#endif

#ifndef LL_FEATURE_SUPPORT_LE_2M_PHY
#define LL_FEATURE_SUPPORT_LE_2M_PHY								1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_CODED_PHY
#define LL_FEATURE_SUPPORT_LE_CODED_PHY								1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_AOA_AOD
#define LL_FEATURE_SUPPORT_LE_AOA_AOD								0
#endif

#ifndef LL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING
#define LL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING					1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING
#define LL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING						1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE
#define LL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE						1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING
#define LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING					1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC
#define LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC				1
#endif

#ifndef LL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2
#define LL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2				1
#endif

//core_5.2 feature begin
#ifndef LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER
#define LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER		1
#endif

#ifndef LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
#define LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE		1
#endif

#ifndef LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER
#define LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER					1
#endif

#ifndef LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER
#define LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER					1
#endif

#ifndef LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS
#define LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS						1
#endif

#ifndef	LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE
#define	LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE					1
#endif
//core_5.2 feature end



#ifndef BQB_LOWER_TESTER_ENABLE
#define BQB_LOWER_TESTER_ENABLE										0
#endif

#ifndef HCI_NEW_FIFO_FEATURE_ENABLE
#define HCI_NEW_FIFO_FEATURE_ENABLE									1
#endif

#ifndef HCI_SEND_NUM_OF_CMP_AFT_ACK
#define HCI_SEND_NUM_OF_CMP_AFT_ACK									0
#endif

#ifndef L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE
#define L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE							0  //just for debug
#endif

#ifndef L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
#define L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN     				0
#endif

#ifndef UPPER_TESTER_DBG_EN
#define UPPER_TESTER_DBG_EN											0
#endif

#ifndef UPPER_TESTER_HCI_LOG_EN
#define UPPER_TESTER_HCI_LOG_EN										0
#endif

//gen p256, dhkey supported by controller
#ifndef	CONTROLLER_GEN_P256KEY_ENABLE
#define	CONTROLLER_GEN_P256KEY_ENABLE								0
#endif

#ifndef SMP_SEC_LEVEL_CHEACK_EN
#define	SMP_SEC_LEVEL_CHEACK_EN										0
#endif

