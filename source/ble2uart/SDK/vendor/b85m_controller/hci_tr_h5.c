/********************************************************************************************************
 * @file	hci_tr_h5.c
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
#include "hci_tr_h5.h"
#include "hci_tr_api.h"
#include "hci_tr_def.h"
#include "hci_slip.h"
#include "hci_tr.h"
#include "hci_h5.h"

/*! Backup buffer define. */
static u8 h5TrBackupBuf[HCI_H5_TR_RX_BUF_SIZE];

/*! HCI H5 transport Rx buffer define. */
static u8 h5TrRxBuf[HCI_H5_TR_RX_BUF_SIZE * HCI_H5_TR_RX_BUF_NUM];

/*! HCI H5 transport Rx Fifo define. */
static hci_fifo_t h5TrRxFifo = {
	HCI_H5_TR_RX_BUF_SIZE,
	HCI_H5_TR_RX_BUF_NUM,
	HCI_H5_TR_RX_BUF_NUM - 1,
	0,
	0,
	&h5TrRxBuf[0],
};

/*! HCI H5 transport main control block. */
typedef struct{
	hci_fifo_t            *pRxFifo;
	HciH5PacketHandler_t   HCI_Tr_SlipHandler;
	u8                    *pBackupBuf;
	u8                     backupCnt;
	u8                     recvd1;
	u16                    recvd2;
}HciH5TrCb_t;

static HciH5TrCb_t hciH5TrCb;

void HCI_Tr_DefaultSlipHandler(u8 *pPacket, u32 len)
{
}

/**
 * @brief : H5 protocol initializaiton.
 * @param : none.
 * @param : none.
 */
void HCI_Tr_H5Init(void)
{
	hciH5TrCb.pRxFifo    = &h5TrRxFifo;
	hciH5TrCb.pBackupBuf = h5TrBackupBuf;
	hciH5TrCb.backupCnt  = 0;

	hciH5TrCb.HCI_Tr_SlipHandler = HCI_Tr_DefaultSlipHandler;

	/* Timer configuration. */
	HCI_Tr_TimeInit(5);

	/* UART configuration. */
	uart_gpio_set(HCI_TR_TX_PIN, HCI_TR_RX_PIN);
	uart_reset();

#if 0
#if (MCU_CORE_TYPE == MCU_CORE_825x)
	//baud rate: 115200
	#if(CLOCK_SYS_CLOCK_HZ == 16000000)
		uart_init(9, 13, PARITY_NONE, STOP_BIT_ONE);
	#elif(CLOCK_SYS_CLOCK_HZ == 24000000)
		uart_init(12, 15, PARITY_NONE, STOP_BIT_ONE);
	#elif(CLOCK_SYS_CLOCK_HZ == 32000000)
		uart_init(30, 8, PARITY_NONE, STOP_BIT_ONE);//uart_init_baudrate();
	#elif(CLOCK_SYS_CLOCK_HZ == 48000000)
		uart_init(25, 15, PARITY_NONE, STOP_BIT_ONE);
	#endif
#else ///8278 baud rate: 1M(1000000)
	#if(CLOCK_SYS_CLOCK_HZ == 16000000)
		uart_init(0, 15, PARITY_NONE, STOP_BIT_ONE);
	#elif(CLOCK_SYS_CLOCK_HZ == 24000000)
		uart_init(1, 11, PARITY_NONE, STOP_BIT_ONE);
	#elif(CLOCK_SYS_CLOCK_HZ == 32000000)
		uart_init(1, 15, PARITY_NONE, STOP_BIT_ONE);//uart_init_baudrate();
	#elif(CLOCK_SYS_CLOCK_HZ == 48000000)
		uart_init(2, 15, PARITY_NONE, STOP_BIT_ONE);
	#endif
#endif

#else
	uart_init_baudrate(HCI_TR_BAUDRATE, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE);
#endif

	uart_recbuff_init(hciH5TrCb.pRxFifo->p,  hciH5TrCb.pRxFifo->size);

	/* UART Rx/Tx use DMA mode. */
	uart_dma_enable(1, 1);
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);
	uart_irq_enable(0, 0);
}

/**
 * @brief : Register slip handler
 * @param : func    Pointer point to handler function.
 * @param : none.
 */
void HCI_Tr_H5RegisterSlipHandler(HciH5PacketHandler_t func)
{
	ASSERT(func != NULL, HCI_TR_ERR_INVALID_PARAM);

	hciH5TrCb.HCI_Tr_SlipHandler = func;
}

/**
 * @brief : Find slip delimiiter
 * @param : pPacket     Pointer point to slip packet buffer.
 * @param : len         the length of data.
 * @param : none.
 */
static u8 *HCI_Tr_H5FindSlipFlag(u8 *pPacket, u32 len)
{
	u8 *pBuf = pPacket;
	while(len)
	{
		u8 slipFlag = pBuf[0];
		if(slipFlag != 0xC0){
			pBuf++;
			len--;
			continue;
		}
		return pBuf;// pointer point to 0xC0
	}
	return NULL;
}

int HCI_Tr_H5BackupHandler(u8 *pPacket, u32 len)
{
	u8 *pBuf = hciH5TrCb.pBackupBuf;
	u16 backupLen = hciH5TrCb.backupCnt;

	if(backupLen == 0)
	{
		ASSERT(len < HCI_H5_TR_RX_BUF_SIZE, HCI_TR_ERR_TR_BACKUP_BUF);

		memcpy(pBuf, pPacket, len);
		hciH5TrCb.backupCnt += len;
		HCI_Tr_TimeEnable();

		TR_TRACK_INFO("Backup start: ");
		HCI_TRACK_DATA(pBuf, hciH5TrCb.backupCnt);
		return len;
	}

	u16 cnt = 0;
	while(len)
	{
		if(*pPacket == 0xC0)
		{
			ASSERT(hciH5TrCb.backupCnt+1 < HCI_H5_TR_RX_BUF_SIZE, HCI_TR_ERR_TR_BACKUP_BUF);

			memcpy(pBuf + hciH5TrCb.backupCnt, pPacket, 1);
			hciH5TrCb.backupCnt += 1;
			cnt++;

			TR_TRACK_INFO("Rx data2: ");
			HCI_TRACK_DATA(hciH5TrCb.pBackupBuf, hciH5TrCb.backupCnt);

			hciH5TrCb.HCI_Tr_SlipHandler(hciH5TrCb.pBackupBuf, hciH5TrCb.backupCnt);

			hciH5TrCb.backupCnt = 0;
			HCI_Tr_TimeDisable();
			return cnt;
		}
		else
		{
			ASSERT(hciH5TrCb.backupCnt+1 < HCI_H5_TR_RX_BUF_SIZE, HCI_TR_ERR_TR_BACKUP_BUF);

			memcpy(pBuf + hciH5TrCb.backupCnt, pPacket, 1);//TODO: Check len
			hciH5TrCb.backupCnt += 1;
			cnt++;
			HCI_Tr_TimeEnable();
		}

		pPacket++;
		len--;
	}

	return cnt;
}

/**
 * @brief : H5 transport rx handler.
 * @param : none.
 * @param : none.
 */
void HCI_Tr_H5RxHandler(void)
{
	hci_fifo_t *pFifo = hciH5TrCb.pRxFifo;
	if(pFifo->wptr == pFifo->rptr){
		return;
	}

	while(pFifo->wptr != pFifo->rptr)
	{
		u8 *pPacket = pFifo->p + (pFifo->rptr & pFifo->mask) * pFifo->size;
		u32 len = 0;
		BSTREAM_TO_UINT32(len, pPacket);

		TR_TRACK_INFO("TR Rx data: ");
		HCI_TRACK_DATA(pPacket, len);

		while(len)
		{
			if(hciH5TrCb.backupCnt){
				u8 res = HCI_Tr_H5BackupHandler(pPacket, len);
				len -= res;
				pPacket += res;
			}

			u8 *pBuf = NULL;
			u8 *pBuf2 = NULL;
			pBuf = HCI_Tr_H5FindSlipFlag(pPacket, len);
			if(pBuf[1] == 0xC0){
				pBuf++;
			}
			if(pBuf)
			{
				u16 tmpLen = pBuf+1 - pPacket;
				pBuf2 = HCI_Tr_H5FindSlipFlag(pBuf+1, len - tmpLen);
				if(pBuf2 == NULL){
					HCI_Tr_H5BackupHandler(pBuf, len - (pBuf - pPacket));
					len = 0;
					continue;
				}
				else{
					u16 slipLen = pBuf2 + 1 - pBuf;
					if(slipLen < 6){
						pPacket += slipLen;
						len -= slipLen;
						continue;
					}

					TR_TRACK_INFO("Rx data1: ");
					HCI_TRACK_DATA(pBuf, slipLen);

					hciH5TrCb.HCI_Tr_SlipHandler(pBuf, slipLen);

					pPacket += slipLen;
					len -= slipLen;
				}
			}
			else
			{
				len = 0;
				continue;
			}
		}

		if(len == 0){
			pFifo->rptr++;
		}
	}
}

/**
 * @brief : H5 transport IRQ handler.
 * @param : none.
 * @param : none.
 */
_attribute_ram_code_
void HCI_Tr_H5IRQHandler(void)
{
	/* Timer timeout IRQ */
	if(reg_tmr_sta & BIT(0)){
		reg_tmr_sta = BIT(0);
		HCI_Tr_TimeDisable();
		hciH5TrCb.backupCnt = 0;
	}

	/* UART DMA Rx IRQ */
	if(reg_dma_irq_status & BIT(0))
	{
		reg_dma_irq_status = BIT(0);

		hci_fifo_t *pFifo = hciH5TrCb.pRxFifo;
		if(pFifo->wptr - pFifo->rptr >= pFifo->mask+1){
			return; //have no memory.
		}

		u8 *p = pFifo->p + (pFifo->wptr & pFifo->mask) * pFifo->size;
		u32 len = 0;
		BYTES_TO_UINT32(len, p);

		if(len)
		{
			pFifo->wptr++;
			p = pFifo->p + (pFifo->wptr & pFifo->mask) * pFifo->size;
			reg_dma_uart_rx_addr = (u16)(u32)p;
		}
	}

	/* UART DMA Tx IRQ */
	if(reg_dma_irq_status & BIT(1)) {
		reg_dma_irq_status = BIT(1);//clear
	}
}


