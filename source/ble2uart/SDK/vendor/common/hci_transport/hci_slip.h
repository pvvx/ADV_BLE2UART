/********************************************************************************************************
 * @file    hci_slip.h
 *
 * @brief   This is the source file for BLE SDK
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
#ifndef HCI_SLIP_H_
#define HCI_SLIP_H_

#include "common/types.h"
#include "hci_tr.h"

/*!  Slip decoder/encoder buffer config macro. */
#define HCI_SLIP_DECODE_BUF_SIZE    MEM_SIZE_ALIGN4(HCI_H5_HEAD_LEN + HCI_TR_RX_BUF_SIZE + HCI_H5_CRC_LEN)
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


