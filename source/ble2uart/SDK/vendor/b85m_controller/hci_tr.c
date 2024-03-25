/********************************************************************************************************
 * @file	hci_tr.c --- h4
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
#include "hci_tr.h"
#include "hci_tr_def.h"
#include "hci_tr_h4.h"
#include "hci_tr_h5.h"
#include "hci_tr_api.h"
#include "hci_slip.h"
#include "hci_h5.h"
#include "stack/ble/controller/ble_controller.h"

/**
 * @brief : HCI transport initializaiton.
 * @param : none.
 * @param : none.
 */
void HCI_Tr_Init(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	HCI_Tr_H4Init(&bltHci_rxfifo);

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
void HCI_Tr_Poll(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	HCI_Tr_H4RxHandler();

#elif HCI_TR_MODE == HCI_TR_H5
	HCI_Tr_H5RxHandler();
	HCI_H5_Poll();

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
void HCI_Tr_IRQHandler(void)
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
 * @brief : HCI rx handler call-back.
 * @param : none.
 * @param : none.
 */
int HCI_Tr_RxHandlerCback(void)
{
#if HCI_TR_MODE == HCI_TR_H4 || HCI_TR_MODE == HCI_TR_H5
	if(bltHci_rxfifo.wptr == bltHci_rxfifo.rptr){
		return 0;//have no data
	}

	u8 *p =  bltHci_rxfifo.p + (bltHci_rxfifo.rptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	if(p)
	{
	#if(UI_LED_ENABLE)
		//gpio_toggle(GPIO_LED_RED);
	#endif

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
 * @brief : HCI tx handler call-back.
 * @param : none.
 * @param : none.
 */
int HCI_Tr_TxHandlerCback(void)
{
#if HCI_TR_MODE == HCI_TR_H4
	static u8 uartTxBuf[4 + HCI_TX_FIFO_SIZE] = {0}; //[!!important]
	if(bltHci_txfifo.wptr == bltHci_txfifo.rptr){
		return 0;//have no data
	}

	if(uart_tx_is_busy()){
		return 0;
	}

    u8 *pBuf = uartTxBuf;
    //u8 *p = hci_fifo_get(&bltHci_txfifo);
    u8 *p = bltHci_txfifo.p + (bltHci_txfifo.rptr & bltHci_txfifo.mask)*bltHci_txfifo.size;
	if(p)
	{
		u32 len = 0;
		BSTREAM_TO_UINT16(len, p);
		UINT32_TO_BSTREAM(pBuf, len);

		ASSERT(len <= HCI_TX_FIFO_SIZE, HCI_TR_ERR_TR_TX_BUF);

		memcpy(pBuf, p, len);
		if(uart_dma_send(uartTxBuf))
		{
		#if(UI_LED_ENABLE)
			//gpio_toggle(GPIO_LED_GREEN);
		#endif
			//hci_fifo_pop (&bltHci_txfifo);
			bltHci_txfifo.rptr++;
			return 1;
		}
	}

#elif HCI_TR_MODE == HCI_TR_H5
	//TX handle has been taken over by H5 protocol.

#elif HCI_TR_MODE == HCI_TR_USB
	//TODO:

#endif
	return 0;
}


