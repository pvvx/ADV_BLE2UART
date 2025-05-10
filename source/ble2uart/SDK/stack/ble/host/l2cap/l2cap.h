/********************************************************************************************************
 * @file    l2cap.h
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
#ifndef L2CAP_H_
#define L2CAP_H_


#define	L2CAP_PSM_EATT		0x0027
#define	L2CAP_PSM_ATT		0x001f

/**
 * @brief	Represent responses to a connection parameter update request
 */
typedef enum{
	CONN_PARAM_UPDATE_ACCEPT = 0x0000,
	CONN_PARAM_UPDATE_REJECT = 0x0001,
}conn_para_up_rsp;

typedef int (*l2cap_handler_t) (u16 conn, u8 * p);

/**
 * @brief	This function is used to register the function to process L2CAP SIG CHANNEL packet
 * @param	*p - the pointer of l2cap data
 * @return	none.
 */
void	blc_l2cap_reg_att_sig_handler(l2cap_handler_t p);//signaling pkt proc


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
 * @brief		This function is used to send command reject response in master
 * @param[in]	connHandle - connection handle
 * @param[in]	req_id - Request packet identifier
 * @param[in]	reason - command reject reason
 * 				0x0000: SIG_CMD_NOT_UNDERSTAND, data_len must set 0x0000
 * 				0x0001: SIG_MTU_EXCEEDED,  data_len must set 0x0002
 * 				0x0002: SIG_INVALID_CID_REQUEST,  data_len must set 0x0004,
 * @param[in]	data - Extended description of reason.
 *				SIG_CMD_NOT_UNDERSTAND: None
 *				SIG_MTU_EXCEEDED:  the maximum signaling MTU the sender of this packet can accept.
 *				SIG_INVALID_CID_REQUEST: local and remote channel endpoints of the disputed channel.
 * @param[in]	data_len - Extended description data length.
 * @return		none.
 */
void blc_l2cap_SendCommandRejectResponse(u16 connHandle, u8 req_id, u16 reason, u8* data, u8 data_len);


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
