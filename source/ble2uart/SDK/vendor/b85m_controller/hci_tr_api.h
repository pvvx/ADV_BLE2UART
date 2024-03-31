/********************************************************************************************************
 * @file	hci_tr_api.h
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
#ifndef HCI_TR_API_H_
#define HCI_TR_API_H_

#include "common/types.h"
#include "stack/ble/hci/hci.h"
#include "app_buffer.h"

/*! HCI transport layer enable/disable. */
#define HCI_TR_EN            1

/*! HCI transport layer protocol selection. */
#define HCI_TR_H4            0
#define HCI_TR_H5            1
#define HCI_TR_USB           2
#define HCI_TR_MODE          HCI_TR_H4

/*! HCI ACL data packet max size define. */
#define HCI_ACL_BUF_SIZE     (1 + 4 + HCI_RX_FIFO_SIZE)
#define HCI_TR_TX_BUF_SIZE   (1 + 2 + HCI_TX_FIFO_SIZE)

/*! HCI UART transport pin define */
#define HCI_TR_RX_PIN        GPIO_PB0
#define HCI_TR_TX_PIN        GPIO_PA2
#define HCI_TR_BAUDRATE      (115200)


typedef void (*HciH5PacketHandler_t)(u8 *pPacket, u32 len);

/*! HCI transport layer API. */

/**
 * @brief : HCI transport initialization.
 * @param : none.
 * @param : none.
 */
void HCI_Tr_Init(void);

/**
 * @brief : HCI transport main loop.
 * @param : none.
 * @param : none.
 */
void HCI_Tr_Poll(void);

/**
 * @brief : HCI IRQ handler.
 * @param : none.
 * @param : none.
 */
void HCI_Tr_IRQHandler(void);


/* HCI Rx/Tx handler call-back. This is for compatibility with other codes. */

/**
 * @brief : HCI rx handler call-back.
 * @param : none.
 * @param : none.
 */
int HCI_Tr_RxHandlerCback(void);

/**
 * @brief : HCI tx handler call-back.
 * @param : none.
 * @param : none.
 */
int HCI_Tr_TxHandlerCback(void);


#endif /* HCI_TR_API_H_ */
