/********************************************************************************************************
 * @file	ext_scan.h
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
#ifndef LL_SCAN_EXT_H_
#define LL_SCAN_EXT_H_

#include "stack/ble/hci/hci_cmd.h"






/**
 * @brief      for user to initialize extended scanning module
 * 			   notice that only one module can be selected between legacy scanning module and extended scanning module
 * @param	   none
 * @return     none
 */
void 		blc_ll_initExtendedScanning_module(void);







/**
 * @brief      This function is used to set the extended scan parameters to be used on the advertising physical channels
 *			   attention: when scan_phys is SCAN_PHY_1M, 	scanType_1 & scanInter_1 & scanWindow_1 are invalid parameters,
 *			   			  when scan_phys is SCAN_PHY_CODED, scanType_0 & scanInter_0 & scanWindow_0 are invalid parameters,
 * @param[in]  ownAddrType - Own_Address_Type
 * @param[in]  scan_fp - Scanning_Filter_Policy
 * @param[in]  scan_phys - Scanning_PHYs, "SCAN_PHY_1M" or "SCAN_PHY_CODED"
 *
 * 			   Attention:
 * 			   scanType_0/scanInter_0/scanWindow_0 are only for 1M    PHY.  If    1M PHY is not supported, these parameters are ignored.
 * 			   scanType_1/scanInter_1/scanWindow_1 are only for Coded PHY.  If Coded PHY is not supported, these parameters are ignored.
 *
 * @param[in]  scanType_0 - Scan_Type for 1M PHY, Passive Scanning or Active Scanning.
 * @param[in]  scanInter_0 - Scan_Interval for 1M PHY, Time interval from when the Controller started its last scan until it
							   begins the subsequent scan on the primary advertising physical channel.
 * @param[in]  scanWindow_0 - Duration of the scan on the primary advertising physical channel for 1M PHY
 *
 * @param[in]  scanType_1 - Scan_Type for Coded PHY, Passive Scanning or Active Scanning.
 * @param[in]  scanInter_1 - Scan_Interval for Coded PHY, Time interval from when the Controller started its last scan until it
							   begins the subsequent scan on the primary advertising physical channel.
 * @param[in]  scanWindow_1 - Duration of the scan on the primary advertising physical channel  for Coded PHY
 *
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_setExtScanParam ( own_addr_type_t  ownAddrType,	scan_fp_type_t scan_fp,		 scan_phy_t	scan_phys,
									 	 scan_type_t   scanType_0,	scan_inter_t   scanInter_0,  scan_wind_t scanWindow_0,
										 scan_type_t   scanType_1,	scan_inter_t   scanInter_1,  scan_wind_t scanWindow_1);







/**
 * @brief	   This function is used to enable or disable scanning.
 * @param[in]  extScan_en - 0x00: Scanning disabled; 0x01: Scanning enabled
 * @param[in]  filter_duplicate - Filter_Duplicates
 * @param[in]  duration - Scan duration
 * @param[in]  period - Time interval from when the Controller started its last Scan_Duration until it begins the
 * 			   			subsequent Scan_Duration.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_setExtScanEnable (scan_en_t  extScan_en,	dupe_fltr_en_t filter_duplicate, scan_durn_t duration,	scan_period_t period);





#endif /* LL_SCAN_EXT_H_ */
