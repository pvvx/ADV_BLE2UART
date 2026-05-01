/********************************************************************************************************
 * @file    hci_dfu.h
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
#ifndef HCI_DFU_H_
#define HCI_DFU_H_

#include "common/types.h"
#include "common/compiler.h"
#include "hci_dfu_def.h"
#include "user_config.h"


#ifndef HCI_DFU_EN
#define HCI_DFU_EN            0
#endif


/**
 *******************************************************************************
 * User
 *******************************************************************************
 */

/*! Firmware version. */
#define DFU_FW_VERSION         0x0003

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

/**
 * @brief : DFU initialization
 * @param : none.
 * @param : none.
 */
void DFU_Init(void);

/**
 * @brief : DFU Task Handler
 * @param : none.
 * @param : none.
 */
void DFU_TaskStart(void);

/**
 *******************************************************************************
 * Internal
 *******************************************************************************
 */

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
typedef struct _attribute_aligned_(4){ 
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
