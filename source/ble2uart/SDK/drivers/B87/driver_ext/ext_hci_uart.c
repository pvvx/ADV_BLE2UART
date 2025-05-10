/********************************************************************************************************
 * @file    ext_hci_uart.c
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
#include "ext_hci_uart.h"
#include "common/config/user_config.h"
#include "../uart.h"
#include "../irq.h"
#include "../dma.h"

void HCI_UartSetPin(u16 txPin, u16 rxPin)
{
	uart_gpio_set(txPin, rxPin);
}

void HCI_UartInit(u32 baudrate, u8 *txBuf, u32 len)
{
	/* UART configuration. */
	//uart_gpio_set(HCI_TR_TX_PIN, HCI_TR_RX_PIN);
	uart_reset();

	uart_init_baudrate(baudrate, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE);

	uart_recbuff_init(txBuf, len);

	/* UART Rx/Tx use DMA mode. */
	uart_dma_enable(1, 1);

	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX, 1);

	irq_set_mask(FLD_IRQ_UART_EN);
	uart_mask_tx_done_irq_enable();
	uart_mask_error_irq_enable();

#if HCI_TR_FLOW_CTRL_EN
	//uart_set_rts(true, UART_RTS_MODE_AUTO, HCI_TR_RTS_THRESH, HCI_TR_RTS_INVERT, HCI_TR_RTS_PIN);
	//uart_set_cts(true, HCI_TR_CTS_POLARITY, HCI_TR_CTS_PIN);
#endif
}

void HCI_UartSetFlowCtrl(u16 ctsPin, u16 rtsPin)
{
	uart_set_rts(true, UART_RTS_MODE_AUTO, RTS_THRESH, RTS_INVERT, rtsPin);
	uart_set_cts(true, CTS_POLARITY, ctsPin);
}

