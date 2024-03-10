/********************************************************************************************************
 * @file	ble_format.h
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
#ifndef BLE_FORMAT_H
#define BLE_FORMAT_H


#include "stack/ble/ble_common.h"


typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 md     :1;
    u8 rfu1   :3;
    u8 rf_len;
}rf_acl_data_head_t;



typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 cie    :1;
    u8 rfu0   :1;
    u8 npi    :1;
    u8 rfu1   :1;
    u8 rf_len;
}rf_cis_data_hdr_t;



typedef struct {
    u8 llid   :2;
    u8 cssn   :3;
    u8 cstf   :1;
    u8 rfu0   :2;
    u8 rf_len;
}rf_bis_data_hdr_t;



typedef struct{
	u8	type;
	u8  rf_len;
	u8	opcode;
	u8	cigId;
	u8	cisId;
	u8  phyM2S;
	u8	phyS2M;

	u32 maxSduM2S :12;
	u32 rfu0 	  :3;
	u32 framed    :1;
	u32 maxSduS2M :12;
	u32 rfu1      :4;

	u8 sduIntvlM2S[3]; //SDU_Interval_M_To_S(20 bits) + RFU(4 bits)
	u8 sduIntvlS2M[3]; //SDU_Interval_S_To_M(20 bits) + RFU(4 bits)

	u16 maxPduM2S;
	u16 maxPduS2M;
	u8	nse;
	u8	subIntvl[3];    //unit: uS

	u8 	bnM2S:4;
	u8 	bnS2M:4;
	u8 	ftM2S;
	u8 	ftS2M;
	u16	isoIntvl;		//unit: 1.25 mS

	u8	cisOffsetMin[3];
	u8	cisOffsetMax[3];
	u16	connEventCnt; //similar to instant

}rf_packet_ll_cis_req_t;

typedef struct{
	u8	type;				//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)
	u8 	opcode;
	u8  cisOffsetMin[3];
	u8  cisOffsetMax[3];
	u16	connEventCnt;
}rf_packet_ll_cis_rsp_t;

typedef struct{
	u8	type;				//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)
	u8 	opcode;
	u32 cisAccessAddr;      //Access Address of the CIS
	u8  cisOffset[3];
	u8  cigSyncDly[3];
	u8  cisSyncDly[3];
	u16 connEventCnt;
}rf_packet_ll_cis_ind_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u8 	opcode;
	u8  cig_id;
	u8	cis_id;
	u8	errorCode;
}rf_packet_ll_cis_terminate_t;



typedef struct{
	union{
		rf_bis_data_hdr_t  bisPduHdr;
		rf_cis_data_hdr_t  cisPduHdr;
		rf_acl_data_head_t aclPduHdr;
		struct{
			u8 type;
			u8 rf_len;
		}pduHdr;
	}llPduHdr;        /* LL PDU Header: 2 */
	u8 	llPayload[1]; /* Max LL Payload length: 251 */
}llPhysChnPdu_t;

typedef struct{
	u32 dma_len;
	llPhysChnPdu_t llPhysChnPdu;
}rf_packet_ll_data_t;








typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16  handle;
	u8	dat[20];
}rf_packet_att_t;


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
