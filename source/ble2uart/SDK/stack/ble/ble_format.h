/********************************************************************************************************
 * @file    ble_format.h
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
#ifndef BLE_FORMAT_H
#define BLE_FORMAT_H


#include "stack/ble/ble_common.h"

/**
 * @brief	Describes ACL packet header information
 */
typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 md     :1;
    u8 rfu1   :3;
    u8 rf_len;
}rf_acl_data_head_t;
/**
 * @brief	Structure representing a Link Layer Physical Channel PDU.
 */
typedef struct{
	union{
		rf_acl_data_head_t aclPduHdr;
		struct{
			u8 type;
			u8 rf_len;
		}pduHdr;
	}llPduHdr;        /* LL PDU Header: 2 */
	u8 	llPayload[1]; /* Max LL Payload length: 251 */
}llPhysChnPdu_t;

/**
 * @brief	Structure representing a Link Layer Data Packet
 */
typedef struct{
	u32 dma_len;
	llPhysChnPdu_t llPhysChnPdu;
}rf_packet_ll_data_t;
/**
 * @brief	Structure representing an ATT packet in the RF packet.
 */
typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16  handle;
	u8	dat[20];
}rf_packet_att_t;
/**
 * @brief	Structure representing an ATT data in the RF packet.
 */
typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2cap;
	u16	chanid;

	u8	att;
	u16 handle;

	u8	dat[20];

}rf_packet_att_data_t;

#endif	/* BLE_FORMAT_H */
