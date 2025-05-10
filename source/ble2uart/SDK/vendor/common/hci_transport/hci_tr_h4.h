/********************************************************************************************************
 * @file    hci_tr_h4.h
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
#ifndef HCI_TR_H4_H_
#define HCI_TR_H4_H_

#include "hci_tr.h"

/*!  H4 transport buffer config macro. */        // 4 dma length
#define HCI_H4_TR_RX_BUF_SIZE   (MEM_SIZE_ALIGN16(4 + HCI_TR_RX_BUF_SIZE)*2)//80 /*! The value of this size must be set according to the size of the HCI RX buffer, and at least twice the size of the HCI RX buffer.*/
#define HCI_H4_TR_RX_BUF_NUM    4

#define HCI_H4_FLUSH_TO         1000/*! in ms*/

/**
 * @brief : H4 transport initialization.
 * @param : pFifo       Pointer point to HCI rx fifo.
 * @return: none
 */
void HCI_Tr_H4Init(hci_fifo_t *pFifo);

/**
 * @brief : H4 transport rx handler.
 * @param : none.
 * @return: none
 */
void HCI_Tr_H4RxHandler(void);

/**
 * @brief : H4 IRQ handler.
 * @param : none.
 * @return: none
 */
void HCI_Tr_H4IRQHandler(void);


#endif /* HCI_TR_H4_H_ */
