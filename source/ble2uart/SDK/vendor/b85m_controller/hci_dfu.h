/*
 *******************************************************************************
 *  FileName  : hci_dfu.h
 *  Created on: 2021-05-07
 *  Author    : GaoQiu
 *  Copyright :
 *******************************************************************************
 */

#ifndef HCI_DFU_H_
#define HCI_DFU_H_

#include "common/types.h"

/*! DFU timeout define. */
#define DFU_TIMEOUT               30/*!< Unit: s */

/*! DFU re-send number. */
#define DFU_RESEND_CNT            3

#define DFU_CRC_INIT_VALUE        0xFFFFFFFF

#define DFU_CHECKSUM_LEN          4

/*! DFU Mode define. */
enum{
	DFU_MODE_NONE           = 0x00,
	DFU_MODE_FW_UPD         = 0x01,
	DFU_MODE_FW_UPD_LATEST  = 0x02,
	DFU_MODE_FW_UPD_OLDER   = 0x03,
};

/*! DFU End mode define. */
enum{
	DFU_END_MODE_FW_UPD   = 0x00,
	DFU_END_MODE_TERM_UPD = 0x01,
};

/*! DFU Status define. */
enum{
	DFU_STA_NONE         = 0x00,
	DFU_STA_START        = 0x01,
	DFU_STA_END          = 0x02,
	DFU_STA_TERM         = 0x03,
};

/*! DFU structure define. */
typedef struct{
	u32  newFwAddrStart;
	u32  maxFwSize;
	u32  nextFwAddrStart;

	u32  newFwSize;
	u32  fwOffset;
    u32  curCrc;          /*<! use for overall firmware check. */

	u32  timer;           /*<! timeout timer. */
	u8   status;
	u8   resendCnt;       /*<! re-send counter. */
}DfuCb_t;

u32 DFU_Crc32Calc(u32 crcInit, u8 *pdata, u32 len);

void DFU_Reset(void);
void DFU_EraseNewFwArea(void);

#endif /* UART_DFU_H_ */
