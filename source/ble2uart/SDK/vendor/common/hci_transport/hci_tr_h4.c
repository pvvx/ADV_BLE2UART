/********************************************************************************************************
 * @file    hci_tr_h4.c
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
#include "hci_tr_h4.h"
#include "hci_tr.h"
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"
#include "hci_tr_def.h"


#if HCI_TR_EN


#define HCI_Tr_H4TimerEnable()    hciH4TrCB.flushTimer = clock_time()|1
#define HCI_Tr_H4TimerDisable()   hciH4TrCB.flushTimer = 0
#define HCI_Tr_H4FlushHandler()   \
do{\
	if(hciH4TrCB.flushTimer && clock_time_exceed(hciH4TrCB.flushTimer, HCI_H4_FLUSH_TO*1000))\
	{\
		hciH4TrCB.flushTimer = 0;\
		hciH4TrCB.backupCnt = 0;\
	}\
}while(0)

 _attribute_aligned_(4) u8 h4TrBackupBuf[HCI_H4_TR_RX_BUF_SIZE];
 _attribute_aligned_(4) u8 h4TrRxBuf[HCI_H4_TR_RX_BUF_SIZE * HCI_H4_TR_RX_BUF_NUM];

static hci_fifo_t _attribute_aligned_(4) h4TrRxFifo = {
	HCI_H4_TR_RX_BUF_SIZE,
	HCI_H4_TR_RX_BUF_NUM,
	HCI_H4_TR_RX_BUF_NUM - 1,
	0,0,
	&h4TrRxBuf[0],
};

/*!  HCI Transmit main control block */
typedef struct _attribute_aligned_(4) {
	hci_fifo_t   *pHciRxFifo;  /*!< Point to HCI rx fifo. */
	hci_fifo_t   *pRxFifo;     /*!< Point to H4 rx fifo.  */
	u8           *pBackUpBuf;
	u32           flushTimer;
	u8            backupCnt;
	u8            align[3];
}HciH4TrCb_t;

static HciH4TrCb_t _attribute_aligned_(4) hciH4TrCB;

/**
 * @brief  : HCI Transport initialization.
 * @param  : pFifo    Pointer point to hci rx FIFO.
 * @return : None.
 */
void HCI_Tr_H4Init(hci_fifo_t *pHciRxFifo)
{
	hciH4TrCB.pHciRxFifo = pHciRxFifo;
	hciH4TrCB.pRxFifo    = &h4TrRxFifo;
	hciH4TrCB.pBackUpBuf = h4TrBackupBuf;
	hciH4TrCB.backupCnt  = 0;
	hciH4TrCB.flushTimer = 0;

#if 1
	HCI_UartSetPin(UART_CONVERT(HCI_TR_TX_PIN, HCI_TR_RX_PIN));
	HCI_UartInit(UART_CONVERT(HCI_TR_BAUDRATE, hciH4TrCB.pRxFifo->p, hciH4TrCB.pRxFifo->size));
#if HCI_TR_FLOW_CTRL_EN
	HCI_UartSetFlowCtrl(UART_CONVERT(HCI_TR_CTS_PIN, HCI_TR_RTS_PIN));
#endif

#else
	/* UART configuration. */
	uart_gpio_set(HCI_TR_TX_PIN, HCI_TR_RX_PIN);
	uart_reset();
	
	uart_init_baudrate(HCI_TR_BAUDRATE, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE);

	uart_recbuff_init(hciH4TrCB.pRxFifo->p, hciH4TrCB.pRxFifo->size);

	/* UART Rx/Tx use DMA mode. */
	uart_dma_enable(1, 1);
#if 0
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);
	uart_irq_enable(0, 0);
#else
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX, 1);

	irq_set_mask(FLD_IRQ_UART_EN);
	uart_mask_tx_done_irq_enable();
	uart_mask_error_irq_enable();
#endif

#if HCI_TR_FLOW_CTRL_EN
	uart_set_rts(true, UART_RTS_MODE_AUTO, HCI_TR_RTS_THRESH, HCI_TR_RTS_INVERT, HCI_TR_RTS_PIN);
	uart_set_cts(true, HCI_TR_CTS_POLARITY, HCI_TR_CTS_PIN);
#endif
#endif
}

/**
 * @brief  : HCI Transmit backup handler
 * @param  : pPacket    Pointer point to data buffer.
 * @param  : len        The length of data.
 * @return : Returns the number of bytes processed.
 */
//_attribute_ram_code_
int HCI_Tr_H4BackUpHandler(u8 *pPacket, u32 len)
{
	u8 *pBuf = hciH4TrCB.pBackUpBuf;
	u8 backupLen = hciH4TrCB.backupCnt;

	if(backupLen == 0)
	{
		memcpy(pBuf, pPacket, len);
		hciH4TrCB.backupCnt = len;

		HCI_Tr_H4TimerEnable();
		return len;
	}

	u8 hciType = 0;
	BSTREAM_TO_UINT8(hciType, pBuf);

	u16 paramLen = 0;
	hci_fifo_t *pHciRxFifo = hciH4TrCB.pHciRxFifo;

	switch(hciType)
	{
	case HCI_TR_TYPE_CMD:
		if(backupLen >= 1 + HCI_CMD_HEAD_LEN)
		{
			pBuf += 2;//skip opcode
			BSTREAM_TO_UINT8(paramLen, pBuf);

			u16 cpyLen = 1 + HCI_CMD_HEAD_LEN + paramLen - hciH4TrCB.backupCnt;
			memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, min(cpyLen, len));
			hciH4TrCB.backupCnt += min(cpyLen, len);

			if(hciH4TrCB.backupCnt == 1 + HCI_CMD_HEAD_LEN + paramLen)
			{
				u8 *p = pHciRxFifo->p + (pHciRxFifo->wptr & (pHciRxFifo->num-1)) * pHciRxFifo->size;
				memcpy(p, hciH4TrCB.pBackUpBuf, hciH4TrCB.backupCnt);
				pHciRxFifo->wptr++;

				hciH4TrCB.backupCnt = 0;//!!! important
				HCI_Tr_H4TimerDisable();
			}
			else
			{
				HCI_Tr_H4TimerEnable();
			}
			return min(cpyLen, len);
		}
		else
		{
			if(backupLen + len >= 1 + HCI_CMD_HEAD_LEN)
			{
				u16 cnt = 0;
				u16 cpyLen = 1 + HCI_CMD_HEAD_LEN - backupLen;
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, cpyLen);
				hciH4TrCB.backupCnt += cpyLen;
				pPacket += cpyLen;
				len -= cpyLen;
				cnt += cpyLen;

				pBuf += 2;//skip opcode
				BSTREAM_TO_UINT8(paramLen, pBuf);
				cpyLen = 1 + HCI_CMD_HEAD_LEN + paramLen - hciH4TrCB.backupCnt;
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, min(cpyLen, len));
				hciH4TrCB.backupCnt += min(cpyLen, len);
				cnt += min(cpyLen, len);

				if(hciH4TrCB.backupCnt == 1 + HCI_CMD_HEAD_LEN + paramLen){
					u8 *p = pHciRxFifo->p + (pHciRxFifo->wptr & (pHciRxFifo->num-1)) * pHciRxFifo->size;
					memcpy(p, hciH4TrCB.pBackUpBuf, hciH4TrCB.backupCnt);
					pHciRxFifo->wptr++;

					hciH4TrCB.backupCnt = 0;//!!! important
					HCI_Tr_H4TimerDisable();
				}
				else{
					HCI_Tr_H4TimerEnable();
				}
				return cnt;
			}
			else
			{
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, len);
				hciH4TrCB.backupCnt += len;
				HCI_Tr_H4TimerEnable();
				return len;
			}
		}
		break;

	case HCI_TR_TYPE_ACL:
		if(backupLen >= 1 + HCI_ACL_HEAD_LEN)
		{
			pBuf += 2;//skip connHandle
			BSTREAM_TO_UINT16(paramLen, pBuf);
			u16 cpyLen = 1 + HCI_ACL_HEAD_LEN + paramLen - backupLen;
			memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, min(cpyLen, len));
			hciH4TrCB.backupCnt += min(cpyLen, len);

			if(hciH4TrCB.backupCnt == 1 + HCI_ACL_HEAD_LEN + paramLen)
			{
				u8 *p = pHciRxFifo->p + (pHciRxFifo->wptr & (pHciRxFifo->num-1)) * pHciRxFifo->size;
				memcpy(p, hciH4TrCB.pBackUpBuf, hciH4TrCB.backupCnt);
				pHciRxFifo->wptr++;

				hciH4TrCB.backupCnt = 0;//!!!important
				HCI_Tr_H4TimerDisable();
			}
			else
			{
				HCI_Tr_H4TimerEnable();
			}
			return min(cpyLen, len);
		}
		else
		{
			if(backupLen + len >= 1 + HCI_ACL_HEAD_LEN)
			{
				u16 cnt = 0;
				u16 cpyLen = 1 + HCI_ACL_HEAD_LEN - backupLen;
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, cpyLen);
				hciH4TrCB.backupCnt += cpyLen;
				pPacket += cpyLen;
				len -= cpyLen;
				cnt += cpyLen;

				pBuf += 2;//skip connHandle
				BSTREAM_TO_UINT16(paramLen, pBuf);
				cpyLen = 1 + HCI_ACL_HEAD_LEN + paramLen - hciH4TrCB.backupCnt;
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, min(cpyLen, len));
				hciH4TrCB.backupCnt += min(cpyLen, len);
				cnt += min(cpyLen, len);

				if(hciH4TrCB.backupCnt == 1 + HCI_ACL_HEAD_LEN + paramLen){
					u8 *p = pHciRxFifo->p + (pHciRxFifo->wptr & (pHciRxFifo->num-1)) * pHciRxFifo->size;
					memcpy(p, hciH4TrCB.pBackUpBuf, hciH4TrCB.backupCnt);
					pHciRxFifo->wptr++;

					hciH4TrCB.backupCnt = 0;//!!!important
					HCI_Tr_H4TimerDisable();
				}
				else{
					HCI_Tr_H4TimerEnable();
				}
				return cnt;
			}
			else
			{
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, len);
				hciH4TrCB.backupCnt += len;
				HCI_Tr_H4TimerEnable();
				return len;
			}
		}
		break;

	case HCI_TR_TYPE_EVENT:
	case HCI_TR_TYPE_SCO:
	default:
		return len;
	}

	/* we will never be here */
	return 0;
}

/**
 * @brief  : HCI Transmit RX handler
 * @param  : none.
 * @return : none.
 */
void HCI_Tr_H4RxHandler(void)
{
	/* backup buffer flush handler */
	HCI_Tr_H4FlushHandler();

	hci_fifo_t *pRxFifo = hciH4TrCB.pRxFifo;
	if(pRxFifo->wptr == pRxFifo->rptr){
		return;//have no data
	}

	u8 *pPacket = pRxFifo->p + (pRxFifo->rptr & pRxFifo->mask) * pRxFifo->size;
	u32 len = 0;
	BSTREAM_TO_UINT32(len, pPacket);//DMA mode length

	hci_fifo_t *pHciRxFifo = hciH4TrCB.pHciRxFifo;

	while(len)
	{
		if(hciH4TrCB.backupCnt){
			u8 res = HCI_Tr_H4BackUpHandler(pPacket, len);
			pPacket += res;
			len -= res;
		}

		u8 *pBuf = pPacket;
		u8 hciType = 0;
		BSTREAM_TO_UINT8(hciType, pBuf);
		u16 paramLen = 0;
		u16 pktLen = 0;

		switch(hciType)
		{
		case HCI_TR_TYPE_CMD:
			if(len < 1 + HCI_CMD_HEAD_LEN)
			{
				HCI_Tr_H4BackUpHandler(pPacket, len);

				len = 0;
				continue;
			}

			pBuf += 2; //skip opcode
		    BSTREAM_TO_UINT8(paramLen, pBuf);
		    pktLen = 1 + HCI_CMD_HEAD_LEN + paramLen;
			break;

		case HCI_TR_TYPE_ACL:
			if(len < 1 + HCI_ACL_HEAD_LEN)
			{
				HCI_Tr_H4BackUpHandler(pPacket, len);

				len = 0;
				continue;
			}
			pBuf += 2; //skip connHandle
		    BSTREAM_TO_UINT16(paramLen, pBuf);
		    pktLen = 1 + HCI_ACL_HEAD_LEN + paramLen;
			break;

		case HCI_TR_TYPE_EVENT:
			if(len < 1 + HCI_EVT_HEAD_LEN)
			{
				len = 0;
				continue;
			}
			pBuf += 1; //skip evtCode
		    BSTREAM_TO_UINT8(paramLen, pBuf);
		    pktLen = 1 + HCI_EVT_HEAD_LEN + paramLen;
		    len -= pktLen;
		    pktLen = 0;
		    continue;

		case HCI_TR_TYPE_SCO:
			if(len < 1 + HCI_SCO_HEAD_LEN)
			{
				len = 0;
				continue;
			}
			pBuf += 2; //skip connHandler
		    BSTREAM_TO_UINT8(paramLen, pBuf);
		    pktLen = 1 + HCI_SCO_HEAD_LEN + paramLen;
		    len -= pktLen;
		    pktLen = 0;
		    continue;

		default:
			len = 0;
			continue;
		}

	    if(pktLen && len >= pktLen)
	    {
	    	//if(pFifo->wptr - pFifo->rptr >= pFifo->num){
	    	//	return; //have no memory.
	    	//}

	    	 /* Copy packet to HCI Rx buffer */
			 u8 *p = pHciRxFifo->p + (pHciRxFifo->wptr & (pHciRxFifo->num-1)) * pHciRxFifo->size;
			 memcpy(p, pPacket, pktLen);
			 pHciRxFifo->wptr++;

			 len -= pktLen;
			 pPacket += pktLen;
	    }
	    else if(pktLen && len < pktLen)
	    {
			HCI_Tr_H4BackUpHandler(pPacket, len);

			len = 0;
			continue;
	    }
	}/* while(len) */


	if(len == 0){
		pRxFifo->rptr++;
	}
}

/**
 * @brief : H4 IRQ handler.
 * @param : none.
 * @return: none
 */
_attribute_ram_code_
void HCI_Tr_H4IRQHandler(void)
{
	/* UART Error IRQ */
#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if(reg_uart_status0(UART_NUM) & FLD_UART_RX_ERR_FLAG){
		reg_uart_status0(UART_NUM) = FLD_UART_CLEAR_RX_FLAG; //write 1 to clear
	}
#else
	if(reg_uart_status0 & FLD_UART_RX_ERR_FLAG){
		reg_uart_status0 = FLD_UART_CLEAR_RX_FLAG; //write 1 to clear
	}
#endif

	/* UART DMA Rx IRQ */
#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if(dma_chn_irq_status_get(FLD_DMA_CHN_UART_RX) & FLD_DMA_CHN_UART_RX)
#else
	if(dma_chn_irq_status_get() & FLD_DMA_CHN_UART_RX)
#endif
	{
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);

		hci_fifo_t *pRxFifo = hciH4TrCB.pRxFifo;
		if(pRxFifo->wptr - pRxFifo->rptr >= pRxFifo->num){
			return; //have no memory.
		}

		u8 *p = pRxFifo->p + (pRxFifo->wptr & pRxFifo->mask) * pRxFifo->size;
		u32 len = 0;
		BYTES_TO_UINT32(len, p);

		if(len)
		{
			pRxFifo->wptr++;
			p = pRxFifo->p + (pRxFifo->wptr & pRxFifo->mask) * pRxFifo->size;
			reg_dma_uart_rx_addr = (u16)(u32)p;
		}
	}

	/* UART DMA Tx IRQ */
#if 0
	if(reg_dma_irq_status & BIT(1)) {
		reg_dma_irq_status = BIT(1);//clear

		isUartTxDone = 1;
	}
#else
	/* UART TxDone IRQ */
	#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
		if(reg_uart_status1(UART_NUM) & FLD_UART_TX_DONE){
			uart_clr_tx_done(UART_NUM);

			isUartTxDone = 1;
		}
	#else
		if(reg_uart_status1 & FLD_UART_TX_DONE){
			uart_clr_tx_done();

			isUartTxDone = 1;
		}
	#endif
#endif

	/* When stop bit error or parity error. */
#if(MCU_CORE_TYPE == MCU_CORE_TC321X)
	if(reg_uart_status0(UART_NUM) & FLD_UART_RX_ERR_FLAG)
	{
		reg_uart_status0(UART_NUM) = FLD_UART_CLEAR_RX_FLAG; //write 1 to clear
	}
#else
	if(reg_uart_status0 & FLD_UART_RX_ERR_FLAG)
	{
		reg_uart_status0 = FLD_UART_CLEAR_RX_FLAG; //write 1 to clear
	}
#endif
}

#endif /* HCI_TR_EN */
