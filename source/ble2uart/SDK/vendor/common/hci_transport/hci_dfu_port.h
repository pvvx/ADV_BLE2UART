/********************************************************************************************************
 * @file    hci_dfu_port.h
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
#ifndef HCI_DFU_PORT_H_
#define HCI_DFU_PORT_H_

#include "common/types.h"

void FLASH_WritePage(u32 addr, u8 *pData, u32 len);
void FLASH_ReadPage(u32 addr, u8 *pData, u32 len);
void FLASH_EraseSector(u32 addr);
void MCU_Reset(void);
bool UART_IsBusy(void);

void Hci_Reset(void);
u16  Hci_Revision(void);
void Hci_SetRevision(u16 revision);
void Hci_SendCmdCmplEvt(u16 opcode, u8 *pParam, u32 len);
void Hci_SendCmdCmplStatusEvt(u16 opcode, u8 status);
void Hci_SendCmdStatusEvt(u16 opcode, u8 status);


#endif /* HCI_DFU_PORT_H_ */
