/********************************************************************************************************
 * @file	acl_conn.h
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
#ifndef ACL_CONN_H_
#define ACL_CONN_H_


/**
 * @brief      This function is used to get the current number of ACL connections.
 * @param[in]  none.
 * @return     The number of currently connected ACLs.
 */
int 		blc_ll_getCurrentConnectionNumber(void);   //master + slave connection number


/**
 * @brief      This function is used to obtain the maximum number of connections that can be supported.
 * @param[in]  none.
 * @return     Maximum number of connections that can be supported.
 */
int			blc_ll_getSupportedMaxConnNumber(void);


/**
 * @brief      This function is used to obtain the number of ACL connections of the Master role.
 * @param[in]  none.
 * @return     The number of currently connected master ACLs.
 */
int 		blc_ll_getCurrentMasterRoleNumber(void);   //master role number


/**
 * @brief      This function is used to obtain the number of ACL connections of the Slave role.
 * @param[in]  none.
 * @return     The number of currently connected slave ACLs.
 */
int 		blc_ll_getCurrentSlaveRoleNumber(void);    //slave  role number


/**
 * @brief      This function is used to configure the number of master and slave connections that the protocol stack can support.
 * @param[in]  max_master_num - Number of master ACL connections supported.
 * @param[in]  max_slave_num - Number of slave ACL connections supported.
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_setMaxConnectionNumber(int max_master_num, int max_slave_num);


/**
 * @brief      This function is used to obtain the currently available TX FIFO numbers according to the ACL handle.
 * @param[in]  connHandle - ACL connection handle.
 * @return     available TX FIFO numbers
 */
u8  		blc_ll_getTxFifoNumber (u16 connHandle);


/**
 * @brief      This function is used to disconnect the device with the specified ACL connection handle and specify the reason for the disconnection.
 * @param[in]  connHandle - ACL connection handle.
 * @param[in]  reason - Reason for disconnection..
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_disconnect (u16 connHandle, u8 reason);


/**
 * @brief      This function is used to obtain the connection establishment time point corresponding to the current ACL connection handle.
 * @param[in]  connHandle - ACL connection handle.
 * @param[in]  reason - Reason for disconnection..
 * @return     The connection establishment time point corresponding to the current ACL connection handle: Based on 16M system clock ticks.
 */
u32 		blc_ll_getConnectionStartTick(u16 connHandle);


/**
 * @brief      This function is used to read remote version with the specified ACL connection handle.
 * @param[in]  connHandle - ACL connection handle.
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_readRemoteVersion(u16 connHandle);


/**
 * @brief      for user to initialize ACL connection module, this is must if user want use ACL master role or ACL slave role.
 * @param	   none
 * @return     none
 */
void		blc_ll_initAclConnection_module(void);



/**
 * @brief      for user to initialize LinkLayer ACL connection RX FIFO.
 * 			   all connection will share the FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_initAclConnRxFifo(u8 *pRxbuf, int fifo_size, int fifo_number);



/**
 * @brief      for user to initialize master LinkLayer ACL connection TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address.
 * @param[in]  fifo_size - TX FIFO size
 * @param[in]  fifo_number - TX FIFO number, can only be 4, 8, 16 or 32
 * @param[in]  conn_number - Number of supported master ACL connections
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_initAclConnMasterTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number, int conn_number);


/**
 * @brief      for user to initialize slave LinkLayer ACL connection TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address.
 * @param[in]  fifo_size - TX FIFO size
 * @param[in]  fifo_number - TX FIFO number, can only be 4, 8, 16 or 32
 * @param[in]  conn_number - Number of supported slave ACL connections
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_initAclConnSlaveTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number, int conn_number);


/**
 * @brief      check ACL whether is in establish state.
 * @param[in]  connHandle - ACL connection handle.
 * @return     status, 0:  connection not established(disconnection or connection complete but not established)
 * 					   1:  connection established state
 */
bool  		blc_ll_isAclConnEstablished(u16 connHandle);


/**
 * @brief      set ACL data length for Master and Slave.
 * @param[in]  maxRxOct - ACL max RX Oct.
 * @param[in]  maxTxOct_master - ACL master max TX Oct
 * @param[in]  maxTxOct_slave - ACL slave max TX Oct
 * @return     status, 0x00 :  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_setAclConnMaxOctetsNumber(u8 maxRxOct, u8 maxTxOct_master, u8 maxTxOct_slave);


/**
 * @brief		This function is used to set ll_length_req pending timing after connection created
 * @param[in]	time_ms - pending timing, unit: ms
 * @return      none
 */
void		blc_ll_setDataLengthReqSendingTime_after_connCreate(int time_ms);


/**
 * @brief       this function is used to set PHY type for connection
 * @param[in]	connHandle - Connection_Handle Range:0x0000 to 0x0EFF
 * @param[in]	all_phys - preference PHY for TX & RX
 * @param[in]	tx_phys - preference PHY for TX
 * @param[in]	rx_phys - preference PHY for RX
 * @param[in]	phy_options - LE coding indication prefer
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t  	blc_ll_setPhy (	u16 connHandle,					le_phy_prefer_mask_t all_phys,
							le_phy_prefer_type_t tx_phys, 	le_phy_prefer_type_t rx_phys,
							le_ci_prefer_t phy_options);


/**
 * @brief       This function is used to set LE Coded PHY preference, S2 or S8, or no specific preference.
 * @param[in]	prefer_CI - Reference structure: hci_le_readPhyCmd_retParam_t.
 * @return      status, 0x00:  succeed
 * 					    other: failed
 */
ble_sts_t	blc_ll_setDefaultConnCodingIndication(le_ci_prefer_t prefer_CI);


/**
 * @brief       this function is used to allows the Host to specify its preferred values for the transmitter PHY and
 *              receiver PHY to be used for all subsequent connections over the LE transport.
 * @param[in]	all_phys - Reference structure: le_phy_prefer_mask_t:
 *                         bit0: The Host has no preference among the transmitter PHYs supported by the Controller
 *                         bit1: The Host has no preference among the receiver PHYs supported by the Controller
 *                         All other bits: Reserved for future use
 * @param[in]	tx_phys - Reference structure: le_phy_prefer_mask_t:
 *                         bit0: The Host prefers to use the LE 1M transmitter PHY (possibly among others)
 *                         bit1: The Host prefers to use the LE 2M transmitter PHY (possibly among others)
 *                         bit2: The Host prefers to use the LE Coded transmitter PHY (possibly among others)
 *                         All other bits: Reserved for future use
 * @param[in]	rx_phys - Reference structure: le_phy_prefer_mask_t
 * @return      status, 0x00:  succeed
 * 					    other: failed
 */
ble_sts_t 	blc_ll_setDefaultPhy(le_phy_prefer_mask_t all_phys, le_phy_prefer_type_t tx_phys, le_phy_prefer_type_t rx_phys);


/**
 * @brief       this function is used to read the current transmitter PHY and receiver PHY on the connection identified
 *              by the Connection_Handle.
 * @param[in]	connHandle - Connection_Handle Range:0x0000 to 0x0EFF
 * @param[out]	para - Reference structure: hci_le_readPhyCmd_retParam_t:
 * 					   Status -  0x00 HCI_LE_Read_PHY command succeeded; 0x01 to 0xFF: HCI_LE_Read_PHY command failed
 * 					   Connection_Handle - Connection_Handle Range:0x0000 to 0x0EFF
 *					   TX_PHY - 1:LE 1M; 2: LE 2M; 3: LE Coded.
 *					   RX_PHY - 1:LE 1M; 2: LE 2M; 3: LE Coded.
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_readPhy( u16 connHandle, hci_le_readPhyCmd_retParam_t *para);


/**
 * @brief      This function is used to obtain the latest average RSSI of ACL connections.
 * @param[in]  connHandle - ACL connection handle.
 * @return     The value of latest average RSSI.
 */
u8			blc_ll_getAclLatestAvgRSSI(u16 connHandle);


ble_sts_t 	blc_hci_le_readBufferSize_cmd(u8 *pData);
ble_sts_t 	blc_hci_le_getLocalSupportedFeatures(u8 *features);
ble_sts_t 	blc_hci_readSuggestedDefaultTxDataLength (u8 *tx, u8 *txtime);
ble_sts_t 	blc_hci_writeSuggestedDefaultTxDataLength (u16 tx, u16 txtime);
ble_sts_t	blc_hci_readMaximumDataLength(hci_le_readMaxDataLengthCmd_retParam_t  *para);

#endif /* ACL_CONN_H_ */
