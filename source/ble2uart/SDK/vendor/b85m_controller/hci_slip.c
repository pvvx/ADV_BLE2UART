/********************************************************************************************************
 * @file	hci_slip.c
 *
 * @brief	This is the source file for BLE SDK
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
#include "hci_slip.h"
#include "hci_tr_h5.h"
#include "hci_tr_def.h"
#include "hci_h5.h"
#include "hci_tr_api.h"

/*! Slip decode buffer define. */
u8 slipDecodeBuf[HCI_SLIP_DECODE_BUF_SIZE];

/*! Slip encode buffer define. */
u8 slipEncodeBuf[4 + HCI_SLIP_ENCODE_BUF_SIZE];/*!< dmalen=4 */

typedef struct{
	u16 escapeSeq;
	u16 unencoded;
}SlipEscapeMap_t;

/*! Slip escape table. */
static const SlipEscapeMap_t slipEscapeTbl[] = {
	{0xDCDB, 0xC0},
	{0xDDDB, 0xDB},
	{0xDEDB, 0x11}, /*!< Only valid when OOF Software Flow Control is enabled*/
	{0xDFDB, 0x13}, /*!< Only valid when OOF Software Flow Control is enabled*/
};

/*! Slip main control block. */
typedef struct{
	HciH5PacketHandler_t       HCI_H5_PktHandler;
	u8                        *pDecodeBuf;
	u8                        *pEncodeBuf;
	u16                        decodeLen;
	u16                        encodeLen;
	u8                         oofEnable;         /*!< OOF Software Flow Control. */
	u8                         recvd[3];
}HciH5SlipCb_t;

static HciH5SlipCb_t hciSlipCb;

/**
 * @brief : Register H5 protocol handler.
 * @param : func    Pointer point to handler function.
 * @param : none.
 */
void HCI_Slip_RegisterPktHandler(HciH5PacketHandler_t func)
{
	if(func == NULL){
		return;
	}
	hciSlipCb.HCI_H5_PktHandler = func;
}

static u8 HCI_Slip_GetUnencoded(u16 escapeSeq)
{
	for(int i=0; i<COUNTOF(slipEscapeTbl); i++)
	{
		if(escapeSeq == slipEscapeTbl[i].escapeSeq){
			return slipEscapeTbl[i].unencoded;
		}
	}
	return SLIP_INVALID_UNENCODED;
}

static u16 HCI_SLip_GetEscapeCode(u8 unencoded)
{
	for(int i=0; i<COUNTOF(slipEscapeTbl); i++)
	{
		if(unencoded == slipEscapeTbl[i].unencoded){
			return slipEscapeTbl[i].escapeSeq;
		}
	}
	return SLIP_INVALID_ESCAPE;
}

/**
 * @brief : Slip decoder.
 * @param : pPacket   Pointer point to buffer.
 * @param : len       The length of data.
 * @param : none.
 */
void HCI_Slip_DecodePacket(u8 *pPacket, u32 len)
{
	SLIP_TRACK_INFO("Slip Decode Start...\n");

	if(pPacket == NULL || len == 0 || len < 6){
		return;//discard
	}

	u8 *pBuf = pPacket+1;//skip slip start flag.
	len -= 2;            //reduce slip start/end flag.
	u16 slipEscape = 0x0000;
	u8 unencoded = 0;
	hciSlipCb.decodeLen = 0;
	u8 *pDecodeBuf = hciSlipCb.pDecodeBuf;

	while(len)
	{
		if(*pBuf == 0xDB)
		{
			BYTES_TO_UINT16(slipEscape, pBuf);
			unencoded = HCI_Slip_GetUnencoded(slipEscape);
			if(unencoded == SLIP_INVALID_UNENCODED){
				return;//discard
			}

			pDecodeBuf[hciSlipCb.decodeLen++] = unencoded;
			pBuf += 2;
			len  -= 2;

			if(hciSlipCb.decodeLen > HCI_SLIP_DECODE_BUF_SIZE){
				SLIP_TRACK_ERR("Decode Length: %d, Decode buffer size: %d\n", hciSlipCb.decodeLen, HCI_SLIP_DECODE_BUF_SIZE);
				ASSERT(false, HCI_TR_ERR_SLIP_DECODE_BUF);
			}
		}
		else
		{
			pDecodeBuf[hciSlipCb.decodeLen++] = *pBuf;
			pBuf += 1;
			len  -= 1;

			if(hciSlipCb.decodeLen > HCI_SLIP_DECODE_BUF_SIZE){
				SLIP_TRACK_ERR("Decode Length: %d, Decode buffer size: %d\n", hciSlipCb.decodeLen, HCI_SLIP_DECODE_BUF_SIZE);
				ASSERT(false, HCI_TR_ERR_SLIP_DECODE_BUF);
			}
		}
	}

	SLIP_TRACK_INFO("Slip decode data:");
	HCI_TRACK_DATA(hciSlipCb.pDecodeBuf, hciSlipCb.decodeLen);
	hciSlipCb.HCI_H5_PktHandler(hciSlipCb.pDecodeBuf, hciSlipCb.decodeLen);
}

/**
 * @brief : Slip encoder.
 * @param : pPacket   Pointer point to buffer.
 * @param : len       The length of data.
 * @param : none.
 */
void HCI_Slip_EncodePacket(u8 *pPacket, u32 len)
{
	hciSlipCb.encodeLen = 0;
	u8* pEncodeBuf = hciSlipCb.pEncodeBuf + 4;
	UINT8_TO_BSTREAM(pEncodeBuf, SLIP_DELIMITER);
	hciSlipCb.encodeLen++;

	u8 *pBuf = pPacket;
	u16  escapeSeq = 0;

	for(int i=0; i<len; i++)
	{
		switch(pBuf[i])
		{
		case 0xC0:
		case 0xDB:
		case 0x11:
		case 0x13:
			if(!hciSlipCb.oofEnable && (pBuf[i] == 0x11 || pBuf[i] == 0x13)){
				UINT8_TO_BSTREAM(pEncodeBuf, pBuf[i]);
				hciSlipCb.encodeLen += 1;
			}
			else{
				escapeSeq = HCI_SLip_GetEscapeCode(pBuf[i]);
				UINT16_TO_BSTREAM(pEncodeBuf, escapeSeq);
				hciSlipCb.encodeLen += 2;
			}

			if(hciSlipCb.encodeLen > HCI_SLIP_ENCODE_BUF_SIZE){
				SLIP_TRACK_ERR("Encode length: %d, Encode buf size: %d\n", hciSlipCb.encodeLen, HCI_SLIP_ENCODE_BUF_SIZE);
				ASSERT(FALSE, HCI_TR_ERR_SLIP_ENCODE_BUF);
			}
			break;

		default:
			UINT8_TO_BSTREAM(pEncodeBuf, pBuf[i]);
			hciSlipCb.encodeLen += 1;

			if(hciSlipCb.encodeLen > HCI_SLIP_ENCODE_BUF_SIZE){
				SLIP_TRACK_ERR("Encode length: %d, Encode buf size: %d\n", hciSlipCb.encodeLen, HCI_SLIP_ENCODE_BUF_SIZE);
				ASSERT(FALSE, HCI_TR_ERR_SLIP_ENCODE_BUF);
			}
			break;
		}
	}

	UINT8_TO_BSTREAM(pEncodeBuf, SLIP_DELIMITER);
	hciSlipCb.encodeLen += 1;

	if(hciSlipCb.encodeLen > HCI_SLIP_ENCODE_BUF_SIZE){
		SLIP_TRACK_ERR("Encode length: %d, Encode buf size: %d\n", hciSlipCb.encodeLen, HCI_SLIP_ENCODE_BUF_SIZE);
		ASSERT(FALSE, HCI_TR_ERR_SLIP_ENCODE_BUF);
	}
}

/**
 * @brief : Slip send.
 * @param : pPacket   Pointer point to buffer.
 * @param : len       The length of data.
 * @param : none.
 */
bool HCI_Slip_Send(u8 *pPacket, u32 len)
{
	if(uart_tx_is_busy()){
		return false;
	}
	HCI_Slip_EncodePacket(pPacket, len);

	u8 *pBuf = hciSlipCb.pEncodeBuf;
	UINT32_TO_BSTREAM(pBuf, hciSlipCb.encodeLen);

	u8 res = uart_dma_send(hciSlipCb.pEncodeBuf);
	return res==0 ? false:true;
}

static void HCI_Slip_DefaultPktHandler(u8 *pPacket, u32 len)
{
}

/**
 * @brief : Slip initialization.
 * @param : none.
 * @param : none.
 */
void HCI_Slip_Init(void)
{
	hciSlipCb.pDecodeBuf = slipDecodeBuf;
	hciSlipCb.decodeLen = 0;

	hciSlipCb.pEncodeBuf = slipEncodeBuf;
	hciSlipCb.encodeLen = 0;

	hciSlipCb.HCI_H5_PktHandler = HCI_Slip_DefaultPktHandler;

	hciSlipCb.oofEnable = 0;

	/* Register Slip Handler to Transport. */
	HCI_Tr_H5RegisterSlipHandler(HCI_Slip_DecodePacket);
}

/**
 * @brief : Set OOF software flow control enable/disable.
 * @param : enable   enable/disable OOF.
 * @param : none.
 */
void HCI_SLip_SetFlowCtrlEnable(bool enable)
{
	hciSlipCb.oofEnable = enable;
}


