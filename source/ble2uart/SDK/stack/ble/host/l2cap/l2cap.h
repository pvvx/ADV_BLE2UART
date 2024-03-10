/********************************************************************************************************
 * @file	l2cap.h
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
#ifndef L2CAP_H_
#define L2CAP_H_


#define	L2CAP_PSM_EATT		0x0027
#define	L2CAP_PSM_ATT		0x001f



typedef enum{
	CONN_PARAM_UPDATE_ACCEPT = 0x0000,
	CONN_PARAM_UPDATE_REJECT = 0x0001,
}conn_para_up_rsp;



/**
 * @brief		initialize l2cap buffer to reassembly link lay PDU to SDU in master
 * @param[in]	*pMTU_m_rx_buff - the pointer of rx buffer in master
 * @param[in]	mtu_m_rx_size   - the size of of rx buffer in master
 * @param[in]	*pMTU_m_tx_buff - the pointer of tx buffer in master
 * @param[in]	mtu_m_tx_size   - the size of of tx buffer in master
 * @return		none.
 */
void 	blc_l2cap_initAclConnMasterMtuBuffer(u8 *pMTU_m_rx_buff, u16 mtu_m_rx_size, u8 *pMTU_m_tx_buff,u16 mtu_m_tx_size);


/**
 * @brief		initialize l2cap buffer to reassembly link lay PDU to SDU in master
 * @param[in]	*pMTU_s_rx_buff - the pointer of rx buffer in slave
 * @param[in]	mtu_s_rx_size   - the size of of rx buffer in slave
 * @param[in]	*pMTU_s_tx_buff - the pointer of tx buffer in slave
 * @param[in]	mtu_s_tx_size   - the size of of tx buffer in slave
 * @return	none.
 */
void 	blc_l2cap_initAclConnSlaveMtuBuffer(u8 *pMTU_s_rx_buff, u16 mtu_s_rx_size, u8 *pMTU_s_tx_buff, u16 mtu_s_tx_size);


/**
 * @brief		This function is used to set connect request parameter for updating connect parameter in slave
 * @param[in]	connHandle - connection handle
 * @param[in]	min_interval - connect interval minimum
 * @param[in]	max_interval - connect interval maximum
 * @param[in]	latency - connect latency
 * @param[in]	timeout - connect timeout
 * @return		0: success
 * 				1: fail
 */
u8   	bls_l2cap_requestConnParamUpdate (u16 connHandle, u16 min_interval, u16 max_interval, u16 latency, u16 timeout);


/**
 * @brief		This function is used to send connect parameter update response in master
 * @param[in]	connHandle - connection handle
 * @param[in]	req_id - Request packet identifier
 * @param[in]	result - connect parameter update result
 * 				0x0001: CONN_PARAM_UPDATE_REJECT
 * 				0x0000: CONN_PARAM_UPDATE_ACCEPT, need to call the API blm_l2cap_processConnParamUpdatePending() later
 * @return		none.
 */
void  	blc_l2cap_SendConnParamUpdateResponse(u16 connHandle, u8 req_id, conn_para_up_rsp result);


/**
 * @brief		This function is used to host set connect parameter process pending in master
 * @param[in]	connHandle - connection handle
 * @param[in]	min_interval - connect interval minimum
 * @param[in]	max_interval - connect interval maximum
 * @param[in]	latency - connect latency
 * @param[in]	timeout - connect timeout
 * @return		none.
 */
void	blm_l2cap_processConnParamUpdatePending(u16 connHandle, u16 min_interval, u16 max_interval, u16 latency, u16 timeout);


/**
 * @brief		This function is used to set the minimal time for send connect parameter update request after connect created
 * @param[in]	connHandle - connection handle
 * @param[in]	time_ms - the unit is millisecond
 * @return		0: success
 * 				1: fail
 */
u8   	bls_l2cap_setMinimalUpdateReqSendingTime_after_connCreate(u16 connHandle, int time_ms);


/**
 * @brief	This function is used to handler L2CAP data
 * @param	connHandle - connection handle
 * @param	*p - the pointer of l2cap data
 * @return	0
 */
int 	blc_l2cap_pktHandler (u16 connHandle, u8 *raw_pkt);

#endif
