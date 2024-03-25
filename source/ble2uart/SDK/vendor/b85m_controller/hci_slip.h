/********************************************************************************************************
 * @file	hci_slip.h
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
#ifndef HCI_SLIP_H_
#define HCI_SLIP_H_

#include "common/types.h"
#include "hci_tr_api.h"

/*!  Slip decoder/encoder buffer config macro. */
#define HCI_SLIP_DECODE_BUF_SIZE    MEM_SIZE_ALIGN4(HCI_H5_HEAD_LEN + HCI_ACL_BUF_SIZE + HCI_H5_CRC_LEN)
#define HCI_SLIP_ENCODE_BUF_SIZE    MEM_SIZE_ALIGN4(2 + (HCI_H5_HEAD_LEN + HCI_TR_TX_BUF_SIZE + HCI_H5_CRC_LEN) * 2)

/*! Slip invalid unencoded. */
#define SLIP_INVALID_UNENCODED      0x00

/*! Slip invalid escape. */
#define SLIP_INVALID_ESCAPE         0x0000

#define SLIP_DELIMITER              0xC0
#define SLIP_ESCAPE                 0xDB

/**
 * @brief : Slip initialization.
 * @param : none.
 * @param : none.
 */
void HCI_Slip_Init(void);

/**
 * @brief : Register H5 protocol handler.
 * @param : func    Pointer point to handler function.
 * @param : none.
 */
void HCI_Slip_RegisterPktHandler(HciH5PacketHandler_t func);

/**
 * @brief : Slip send.
 * @param : pPacket   Pointer point to buffer.
 * @param : len       The length of data.
 * @param : none.
 */
bool HCI_Slip_Send(u8 *pPacket, u32 len);

/**
 * @brief : Slip decoder.
 * @param : pPacket   Pointer point to buffer.
 * @param : len       The length of data.
 * @param : none.
 */
void HCI_Slip_DecodePacket(u8 *pPacket, u32 len);

/**
 * @brief : Slip encoder.
 * @param : pPacket   Pointer point to buffer.
 * @param : len       The length of data.
 * @param : none.
 */
void HCI_Slip_EncodePacket(u8 *pPacket, u32 len);

/**
 * @brief : Set OOF software flow control enable/disable.
 * @param : enable   enable/disable OOF.
 * @param : none.
 */
void HCI_SLip_SetFlowCtrlEnable(bool enable);

#endif /* HCI_SLIP_H_ */


