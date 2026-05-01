/********************************************************************************************************
 * @file    ext_hci_uart.h
 *
 * @brief   This is the header file for B91
 *
 * @author  BLE Group
 * @date    2020
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
#ifndef EXT_HCI_UART_H_
#define EXT_HCI_UART_H_

#include "common/types.h"

/*** RTS ***/
#define RTS_INVERT    1 /*!< 0: RTS PIN will change from low to high. !! use for RTS auto mode(default auto mode)*/
#define RTS_THRESH    5 /*!< RTS trigger threshold. range: 1-16. */
/*** CTS ***/
#define CTS_POLARITY  1 /*!< 0: low level stop TX; 1: high level stop TX.*/

void HCI_UartSetPin(u16 txPin, u16 rxPin);
void HCI_UartInit(u32 baudrate, u8 *txBuf, u32 len);
void HCI_UartSetFlowCtrl(u16 ctsPin, u16 rtsPin);

#endif /* EXT_HCI_UART_H_ */
