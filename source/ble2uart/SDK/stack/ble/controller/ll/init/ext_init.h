/********************************************************************************************************
 * @file	ext_init.h
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
#ifndef EXT_INIT_H_
#define EXT_INIT_H_



/**
 * @brief      for user to initialize extended initiating module
 * 			   notice that only one module can be selected between legacy initiating module and extended initiating module
 * @param	   none
 * @return     none
 */
void 		blc_ll_initExtendedInitiating_module(void);



/**
 * @brief      This function is used to create an ACL connection to a connectable advertiser.
 *
 * @param[in]  filter_policy - used to determine whether the WhiteList is used. If the White List is not used, the Peer_Address_Type and the
							   Peer_Address parameters specify the address type and address of the advertising device to connect to.
 * @param[in]  ownAdr_type - indicates the type of address being used in the connection request packets.
 * @param[in]  peerAdrType - indicates the type of address used in the connectable advertisement sent by the peer.
 * @param[in]  *peerAddr - indicates the Peer's Public Device Address, Random (static) Device Address, Non-Resolvable Private Address, or
							Resolvable Private Address depending on the Peer_Address_Type parameter.
 * @param[in]  init_phys - indicates the PHY(s) on which the advertising packets should be received on the primary advertising physical channel and
							the PHYs for which connection parameters have been specified.
 *
 * 			   Attention:
 * 			   scanInter_0/scanWindow_0/conn_min_0/conn_max_0/timeout_0 are only for 	1M PHY.  If    1M PHY is not supported, these parameters are ignored.
 * 			   scanInter_1/scanWindow_1/conn_min_1/conn_max_1/timeout_1 are only for 	2M PHY.  If    2M PHY is not supported, these parameters are ignored.
 * 			   scanInter_2/scanWindow_2/conn_min_2/conn_max_2/timeout_2 are only for Coded PHY.  If Coded PHY is not supported, these parameters are ignored.
 *
 * @param[in]  scanInter_0 - for 1M PHY: recommendations from the Host on how frequently (LE_Scan_Interval) the Controller should scan.
 * @param[in]  scanWindow_0 - for 1M PHY: recommendations from the Host on how long (LE_Scan_Window) the Controller should scan.
 * @param[in]  conn_min_0 - for 1M PHY: the minimum allowed connection interval.
 * @param[in]  conn_max_0 - for 1M PHY: the maximum allowed connection interval.
 * @param[in]  timeout_0 - for 1M PHY: Supervision timeout for the LE Link.
 * @param[in]  scanInter_1 - for 2M PHY: recommendations from the Host on how frequently (LE_Scan_Interval) the Controller should scan.
 * @param[in]  scanWindow_1 - for 2M PHY: recommendations from the Host on how long (LE_Scan_Window) the Controller should scan.
 * @param[in]  conn_min_1 - for 2M PHY: the minimum allowed connection interval.
 * @param[in]  conn_max_1 - for 2M PHY: the maximum allowed connection interval.
 * @param[in]  timeout_1 - for 2M PHY: Supervision timeout for the LE Link.
 * @param[in]  scanInter_2 - for Coded PHY: recommendations from the Host on how frequently (LE_Scan_Interval) the Controller should scan.
 * @param[in]  scanWindow_2 - for Coded PHY: recommendations from the Host on how long (LE_Scan_Window) the Controller should scan.
 * @param[in]  conn_min_2 - for Coded PHY: the minimum allowed connection interval.
 * @param[in]  conn_max_2 - for Coded PHY: the maximum allowed connection interval.
 * @param[in]  timeout_2 - for Coded PHY: Supervision timeout for the LE Link.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_extended_createConnection ( init_fp_t  filter_policy,  own_addr_type_t  ownAdrType, 			u8	peerAdrType,            u8 *peerAddr, init_phy_t init_phys,
											   scan_inter_t scanInter_0,  	 scan_wind_t scanWindow_0, 	conn_inter_t conn_min_0, conn_inter_t conn_max_0, conn_tm_t timeout_0,
											   scan_inter_t scanInter_1,     scan_wind_t scanWindow_1, 	conn_inter_t conn_min_1, conn_inter_t conn_max_1, conn_tm_t timeout_1,
											   scan_inter_t scanInter_2,     scan_wind_t scanWindow_2, 	conn_inter_t conn_min_2, conn_inter_t conn_max_2, conn_tm_t timeout_2 );






#endif /* EXT_INIT_H_ */
