/*
 *******************************************************************************
 *  FileName  : hci_dfu_api.h
 *  Created on: 2021-05-07
 *  Author    : GaoQiu
 *  Copyright :
 *******************************************************************************
 */

#ifndef HCI_DFU_API_H_
#define HCI_DFU_API_H_

#include "hci_dfu_def.h"

/*! Firmware version. */
#define DFU_FW_VERSION         0x0014

/*! New firmware start address. */
#define DFU_NEW_FW_ADDR_BASE   (0x20000)

/*! Max size of new firmware. */
#define DFU_NEW_FW_MAX_SIZE    (0x20000 - 0x1000)//128K - 4K (4K is invalid)

/*! Max length of payload. */
#define DFU_MAX_PAYLOAD        64

/*! HCI VS error code. */
enum{
	HCI_SUCCESS           = 0x00,
	HCI_ERR_FW_VER        = 0xA0,
	HCI_ERR_FW_SIZE       = 0xA1,
	HCI_ERR_DFU_DISABLED  = 0xA2,
	HCI_ERR_DFU_ENABLEED  = 0xA3,
	HCI_ERR_FW_CHECKSUM   = 0xA4,
	HCI_ERR_FW_CRC        = 0xA5,
	HCI_ERR_FW_INCOMPLETE = 0xA6,
	HCI_ERR_DATA_LENGTH   = 0xA7,
    HCI_ERR_INVALID_PARAM = 0xA8,
};

void DFU_Init(void);
void DFU_TaskStart(void);

void HCI_VS_CmdHandler(u8 *pHciPkt, u32 len);

bool DFU_isEnable(void);

#endif /* HCI_DFU_API_H_ */
