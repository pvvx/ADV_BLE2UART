/********************************************************************************************************
 * @file	common_dbg.c
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
#include "tl_common.h"
#include "common_dbg.h"




#if (UART_LOW_POWER_DEBUG_EN)
	#if (MCU_CORE_TYPE == MCU_CORE_9518)
		#define UART0_DMA_CHANNEL_TX            DMA4
		#define UART0_BAUDRATE                  1000000  //115200

		int lp_uart_init = 0;  //attention: can not be retention data !!!
		void low_power_uart_debug_init(void)
		{
			uart0_init(UART0_BAUDRATE);
			uart_set_tx_dma_config(UART0, UART0_DMA_CHANNEL_TX);
			uart_clr_tx_done(UART0);
			dma_clr_irq_mask(UART0_DMA_CHANNEL_TX,TC_MASK|ABT_MASK|ERR_MASK);

			lp_uart_init = 1;
		}
	#elif (MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
		#define UART_BAUDRATE					1000000
		#define UART_TRANS_BUFF_LEN				32

		__attribute__((aligned(4))) unsigned char uart_trans_buff[UART_TRANS_BUFF_LEN] = {0x0c,0x00,0x00,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc};

		int lp_uart_init = 0;  //attention: can not be retention data !!!
		void low_power_uart_debug_init(void)
		{
			reg_dma_uart_tx_size = UART_TRANS_BUFF_LEN >> 4;//set DMA TX buffer size.
			uart_gpio_set(UART_TX_PB1, UART_RX_PB7);// uart tx/rx pin set
			uart_reset();  //uart module power-on again.
			uart_init_baudrate(UART_BAUDRATE, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE);
			uart_dma_enable(0, 1); 	//uart data in hardware buffer moved by dma, so we need enable them first
			dma_chn_irq_enable(FLD_DMA_CHN_UART_TX, 1);   	//uart Tx dma irq enable

			lp_uart_init = 1;
		}
	#endif
#endif
