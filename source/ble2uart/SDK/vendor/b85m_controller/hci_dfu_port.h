/*
 *******************************************************************************
 *  FileName  : hci_dfu_port.h
 *  Created on: 2021-05-07
 *  Author    : GaoQiu
 *  Copyright :
 *******************************************************************************
 */

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
void Hci_SetRevision(u16 revison);
void Hci_SendCmdCmplEvt(u16 opcode, u8 *pParam, u32 len);
void Hci_SendCmdCmplStatusEvt(u16 opcode, u8 status);
void Hci_SendCmdStatusEvt(u16 opcode, u8 status);


#endif /* HCI_DFU_PORT_H_ */
