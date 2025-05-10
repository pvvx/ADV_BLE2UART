/********************************************************************************************************
 * @file    common_dbg.c
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
#include "tl_common.h"
#include "common_dbg.h"




#if (UART_LOW_POWER_DEBUG_EN)
	#if (MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
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
