/********************************************************************************************************
 * @file	ext_adv.h
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
#ifndef LL_EXT_ADV_H_
#define LL_EXT_ADV_H_


#include "stack/ble/hci/hci_cmd.h"


/* maximum number of advertising sets this SDK can support, periodic advertising is included. */
#define			ADV_SETS_NUMBER_MAX								4


/* Note: user can't modify this value,and this value must 4 byte aligned
 *  if use BLUETOOTH_VER_5_2 this value is 680 other than 572
 */
#define 		ADV_SET_PARAM_LENGTH							680//572 //



#if 0
/* if use legacy ADV, primary ADV packet maximum length is 44 */
#define 		MAX_LENGTH_PRIMARY_ADV_PKT_LEGACY				44

/* if use extended ADV, primary ADV packet maximum length is 28 */
#define 		MAX_LENGTH_PRIMARY_ADV_PKT_EXTENDED				28

/* if not sure which kind of ADV will be used, use the bigger value is recommended.
 * For example, when used as BLE Controller, BLE Host can use either legacy or extended ADV */
#define 		MAX_LENGTH_PRIMARY_ADV_PKT						44



#define 		MAX_LENGTH_SECOND_ADV_PKT						264   //sizeof(rf_pkt_ext_adv_t) = 261
#endif














/**
 * @brief      for user to initialize extended advertising module
 * 			   notice that: 1. only one module can be selected between legacy advertising module and extended advertising module
 *							2. this API must be used before any other extended ADV initialization APIs.
 * @param	   none
 * @return     none
 */
void 		blc_ll_initExtendedAdvertising_module(void);

/**
 * @brief      for user to allocate single or multiple advertising sets buffer
 * 			   notice that: this API must used after "blc_ll_initExtendedAdvertising_module",
 * 			                and before any other extended ADV initialization APIs.
 * @param[in]  pBuff_advSets - global buffer allocated by application layer.
 * @param[in]  num_advSets - number of application adv_sets
 * @return     Status - 0x00: command succeeded;
 * 						0x12: num_advSets exceed maximum number of supported adv_sets.
 */
ble_sts_t 	blc_ll_initExtendedAdvSetBuffer(u8 *pBuff_advSets, int num_advSets);

/**
 * @brief      initialize Advertising Data buffer for all adv_set
 * @param[in]  pExtAdvData -
 * @param[in]  max_len_advData -
 * @return     none
 */
void 		blc_ll_initExtAdvDataBuffer(u8 *pExtAdvData, int max_len_advData);

/**
 * @brief      initialize Advertising Data buffer for specific adv_set.
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  pExtAdvData -
 * @param[in]  max_len_advData -
 * @return     Status - 0x00: succeed.
 * 						0x12: adv_handle out of range.
 */
ble_sts_t 	blc_ll_initExtAdvDataBuffer_by_advHandle( u8 adv_handle, u8 *pExtAdvData, int max_len_advData);

/**
 * @brief      initialize Scan Response Data Buffer for all adv_set
 * @param[in]  pScanRspData -
 * @param[in]  max_len_scanRspData -
 * @return     none
 */
void 		blc_ll_initExtScanRspDataBuffer(u8 *pScanRspData, int max_len_scanRspData);

/**
 * @brief      initialize Scan Response Data Buffer buffer for specific adv_set.
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  pScanRspData -
 * @param[in]  max_len_scanRspData -
 * @return     Status - 0x00: succeed.
 * 						0x12: adv_handle out of range.
 */
ble_sts_t 	blc_ll_initExtScanRspDataBuffer_by_advHandle(u8 adv_handle, u8 *pScanRspData, int max_len_scanRspData);







/**
 * @brief      This function is used to set the advertising parameters
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  adv_evt_prop -
 * @param[in]  pri_advInter_min & pri_advInter_max
 * @param[in]
 * @param[in]
 * @param[in]
 * @param[in]
 * @return     Status - 0x00: command succeeded;
 * 						0x12:  1. adv_handle out of range;
 * 						       2. pri_advChnMap out of range
 * 						0x0C:  advertising is enabled for the specified advertising set
 */
ble_sts_t 	blc_ll_setExtAdvParam(  u8 adv_handle, 		 			advEvtProp_type_t adv_evt_prop,	u32 pri_advInter_min, 		u32 pri_advInter_max,
									adv_chn_map_t pri_advChnMap,	own_addr_type_t ownAddrType, 	u8 peerAddrType, 			u8  *peerAddr,
									adv_fp_type_t advFilterPolicy,  tx_power_t adv_tx_pow,			le_phy_type_t pri_adv_phy, 	u8 sec_adv_max_skip,
									le_phy_type_t sec_adv_phy, 	 	u8 adv_sid, 					u8 scan_req_noti_en);



/**
 * @brief      This function is used to set the data used in advertising PDU that have a data field
 * 			   notice that: setting legacy ADV data also use this API, data length can not exceed 31
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  *advData -
 * @param[in]  advData_len -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_ll_setExtAdvData (u8 adv_handle, int advData_len, u8 *advData);






/**
 * @brief      This function is used to provide scan response data used in scanning response PDUs.
 * 			   notice that: setting legacy scan response data also use this API, data length can not exceed 31
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  scanRspData_len -
 * @param[in]  *scanRspData -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_setExtScanRspData(u8 adv_handle, int scanRspData_len, u8 *scanRspData);





/**
 * @brief      This function is used to request the Controller to enable or disable one or more advertising sets using the
			   advertising sets identified by the adv_handle
 * @param[in]  enable -
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  duration -	the duration for which that advertising set is enabled
 * 							Range: 0x0001 to 0xFFFF, Time = N * 10 ms, Time Range: 10 ms to 655,350 ms
 * @param[in]  max_extAdvEvt - Maximum number of extended advertising events the Controller shall
 *                             attempt to send prior to terminating the extended advertising
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_setExtAdvEnable(adv_en_t enable, u8 adv_handle, u16 duration, u8 max_extAdvEvt);



/**
 * @brief      This function is used by the Host to set the random device address specified by the Random_Address
			   parameter
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  *rand_addr - Random Device Address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_ll_setAdvRandomAddr(u8 adv_handle, u8* rand_addr);



/**
 * @brief      This function is is used to remove an advertising set from the Controller.
 * @param[in]  adv_handle - Used to identify an advertising set
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_ll_removeAdvSet(u8 adv_handle);



/**
 * @brief      This function is used to remove all existing advertising sets from the Controller.
 * @param[in]  none.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_ll_clearAdvSets(void);




//only for Debug
ble_sts_t	blc_ll_setAuxAdvChnIdxByCustomers(u8 aux_chn);




#endif /* LL_EXT_ADV_H_ */
