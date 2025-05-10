/********************************************************************************************************
 * @file    hci_tr.c
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
#include "hci_tr.h"
#include "hci_tr_def.h"
#include "hci_tr_h4.h"
#include "hci_tr_h5.h"
#include "hci_slip.h"
#include "hci_h5.h"
#include "stack/ble/controller/ble_controller.h"



#if HCI_TR_EN

volatile u8 isUartTxDone = 1;

void HCI_Handler(void);

/**
 * @brief : HCI transport initialization.
 * @param : none.
 * @param : none.
 */
void HCI_TransportInit(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	HCI_Tr_H4Init(&bltHci_rxfifo);

	isUartTxDone = 1;

#elif HCI_TR_MODE == HCI_TR_H5
	HCI_Tr_H5Init();
	HCI_Slip_Init();
	HCI_H5_Init(&bltHci_rxfifo, &bltHci_txfifo);

#elif HCI_TR_MODE == HCI_TR_USB
	//TODO:

#endif
}

/**
 * @brief : HCI transport main loop.
 * @param : none.
 * @param : none.
 */
void HCI_TransportPoll(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	HCI_Tr_H4RxHandler();

	/* HCI Handler */
	HCI_Handler();

#elif HCI_TR_MODE == HCI_TR_H5
	HCI_Tr_H5RxHandler();
	HCI_H5_Poll();

	/* HCI Handler */
	HCI_Handler();

#elif HCI_TR_MODE == HCI_TR_USB
	//TODO:

#endif
}

/**
 * @brief : HCI IRQ handler.
 * @param : none.
 * @param : none.
 */
_attribute_ram_code_
void HCI_TransportIRQHandler(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	HCI_Tr_H4IRQHandler();

#elif HCI_TR_MODE == HCI_TR_H5
	HCI_Tr_H5IRQHandler();

#elif HCI_TR_MODE == HCI_TR_USB
	//TODO:

#endif
}

/**
 * @brief : HCI rx handler.
 * @param : none.
 * @param : none.
 */
int HCI_RxHandler(void)
{
#if HCI_TR_MODE == HCI_TR_H4 || HCI_TR_MODE == HCI_TR_H5
	if(bltHci_rxfifo.wptr == bltHci_rxfifo.rptr){
		return 0;//have no data
	}

	u8 *p =  bltHci_rxfifo.p + (bltHci_rxfifo.rptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	if(p)
	{
		blc_hci_handler(&p[0], 0);//the second parameter is not used.
		bltHci_rxfifo.rptr ++;
		return 1;
	}
#elif HCI_TR_MODE == HCI_TR_USB
	//TODO:

#endif
	return 0;
}

/**
 * @brief : HCI tx handler
 * @param : none.
 * @param : none.
 */
int HCI_TxHandler(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	static u8 uartTxBuf[4 + HCI_TX_FIFO_SIZE] = {0}; //[!!important]
	if(bltHci_txfifo.wptr == bltHci_txfifo.rptr){
		return 0;//have no data
	}
	if(!isUartTxDone){
		return 0;
	}


    u8 *pBuf = uartTxBuf;
    u8 *p = bltHci_txfifo.p + (bltHci_txfifo.rptr & bltHci_txfifo.mask)*bltHci_txfifo.size;
	if(p)
	{
		u32 len = 0;
		BSTREAM_TO_UINT16(len, p);
		UINT32_TO_BSTREAM(pBuf, len);

		ASSERT(len <= HCI_TX_FIFO_SIZE, HCI_TR_ERR_TR_TX_BUF);

		memcpy(pBuf, p, len);
		uart_send_dma(UART_CONVERT((unsigned char* )uartTxBuf));
		isUartTxDone = 0;
		bltHci_txfifo.rptr++;
	}

#elif HCI_TR_MODE == HCI_TR_H5
	//TX handle has been taken over by H5 protocol.

#elif HCI_TR_MODE == HCI_TR_USB
	//TODO:

#endif
	return 0;
}

void HCI_Handler(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	HCI_RxHandler();
	HCI_TxHandler();

#elif HCI_TR_MODE == HCI_TR_H5
	HCI_RxHandler();
	//HCI_TxHandler(); //This is not needed because the H5 takes over the handling of the HCI TX FIFO.

#elif HCI_TR_MODE == HCI_TR_USB
	//TODO:
#endif
}

#endif /* End of HCI_TR_EN */
