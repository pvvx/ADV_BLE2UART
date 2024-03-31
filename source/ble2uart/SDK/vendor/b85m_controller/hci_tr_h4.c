/********************************************************************************************************
 * @file	hci_tr_h4.c
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
#include "hci_tr_h4.h"
#include "hci_tr.h"
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"
#include "hci_tr_api.h"
#include "hci_tr_def.h"

/******************************************************************************/
/*********    UART Normal Mode                                    *************/
/******************************************************************************/
#if !HCI_H4_DMA_MODE_EN
/*! HCI Transmit State */
enum{
	HCI_TR_STATE_INIT   = 0x00,
	HCI_TR_STATE_IDLE   = 0x01,
	HCI_TR_STATE_RCVING = 0x02,
	HCI_TR_STATE_RCVED  = 0x03,
	HCI_TR_STATE_ERR    = 0x04,
};

/*!  HCI Transmit main control block */
typedef struct{
	u8            *pCurRxBuf;
	u16            rxCnt;
	u16            frameHeadLen;
	u16            paramLen;
	u8             trState;
	u8             trType;
	hci_fifo_t     *rxFifo;
}HciTrCB_t;

static HciTrCB_t hciTrCb;

void HCI_Tr_H4Init(hci_fifo_t *pRxFifo)
{
	hciTrCb.rxFifo = pRxFifo;

	hciTrCb.pCurRxBuf = NULL;
	hciTrCb.rxCnt = 0;

	hciTrCb.frameHeadLen = 0;
	hciTrCb.paramLen = 0;

	hciTrCb.trState = HCI_TR_STATE_INIT;
	hciTrCb.trType = HCI_TR_TYPE_NONE;

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
#else ///8278 use 1000000 for xiaomi project.
	//baud rate: 1000000
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

	/* UART Rx use Normal mode, UART Tx use DMA mode. */
	uart_dma_enable(0, 1);
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_TX, 1);
	uart_irq_enable(1, 0);
	uart_ndma_irq_triglevel(1,1);
}

u8 rxIndex = 0;
static u8 HCI_Tr_ReceiveByte(void)
{
	u8 data = REG_ADDR8(0x90 + rxIndex);
	rxIndex++;
	rxIndex = rxIndex & 0x03;
	return data;
}

_attribute_ram_code_
void HCI_Tr_H4RxHandler(u8 data)
{
	u8 res = data; //HCI_Tr_ReceiveByte();

	hci_fifo_t *pFifo = hciTrCb.rxFifo;

	switch(hciTrCb.trState)
	{
	case HCI_TR_STATE_INIT:
		//hciTrCb.trState = HCI_TR_STATE_IDLE;
		break;

	case HCI_TR_STATE_IDLE:
	{
		HCI_Tr_TimeEnable();
		hciTrCb.trType = HCI_TR_TYPE_NONE;
		hciTrCb.rxCnt = 0;

		switch(res)
		{
		case HCI_TR_TYPE_CMD:
			hciTrCb.frameHeadLen = HCI_CMD_HEAD_LEN;//3;
			break;
		case HCI_TR_TYPE_ACL:
			hciTrCb.frameHeadLen = HCI_ACL_HEAD_LEN;//4;
			break;
		case HCI_TR_TYPE_EVENT:
			//hciTrCb.frameHeadLen = HCI_EVT_HEAD_LEN;//2;
			//break;
		case HCI_TR_TYPE_SCO:
			//hciTrCb.frameHeadLen = HCI_SCO_HEAD_LEN;//3;
			//break;
		default:
			hciTrCb.frameHeadLen = 0;
			return;
		}

		if(pFifo->wptr - pFifo->rptr >= pFifo->num){
			return; //have no memory.
		}

		hciTrCb.pCurRxBuf = pFifo->p + (pFifo->wptr & (pFifo->num-1)) * pFifo->size;
		hciTrCb.pCurRxBuf[hciTrCb.rxCnt++] = res;

		hciTrCb.trType   = res;
		hciTrCb.paramLen = 0;
		hciTrCb.trState  = HCI_TR_STATE_RCVING;
		break;
	}
	case HCI_TR_STATE_RCVING:
	{
		HCI_Tr_TimeEnable();

		hciTrCb.pCurRxBuf[hciTrCb.rxCnt++] = res;

		if(hciTrCb.frameHeadLen == hciTrCb.rxCnt - 1)
		{
			u8 *p = NULL;
			if(hciTrCb.trType == HCI_TR_TYPE_ACL){
				p = hciTrCb.pCurRxBuf + hciTrCb.rxCnt - 2;
				hciTrCb.paramLen = (p[1]<<8) + p[0];
			}
			else if(hciTrCb.trType == HCI_TR_TYPE_CMD){
				p = hciTrCb.pCurRxBuf + hciTrCb.rxCnt - 1;
				hciTrCb.paramLen = p[0];
			}
			else{
				/* We will never be here */
				hciTrCb.paramLen = 0xff00;
				hciTrCb.trState = HCI_TR_STATE_IDLE;
				HCI_Tr_TimeDisable();
			}

			if(hciTrCb.paramLen == 0){
				//array_printf(hciTrCb.pCurRxBuf, hciTrCb.rxCnt);
				pFifo->wptr++;
				hciTrCb.trState = HCI_TR_STATE_IDLE;
				HCI_Tr_TimeDisable();
				break;
			}
			else if(hciTrCb.paramLen + hciTrCb.rxCnt > hciTrCb.rxFifo->size){
				hciTrCb.trState = HCI_TR_STATE_IDLE;
				HCI_Tr_TimeDisable();
				break;
			}
		}

		if(hciTrCb.paramLen != 0)
		{
			if(hciTrCb.rxCnt == 1 + hciTrCb.frameHeadLen + hciTrCb.paramLen){
				//array_printf(hciTrCb.pCurRxBuf, hciTrCb.rxCnt);
				pFifo->wptr++;
				hciTrCb.trState = HCI_TR_STATE_IDLE;
				HCI_Tr_TimeDisable();
			}
			else if(hciTrCb.rxCnt > 1 + hciTrCb.frameHeadLen + hciTrCb.paramLen){
				hciTrCb.trState = HCI_TR_STATE_IDLE;
				HCI_Tr_TimeDisable();
			}
		}
		break;
	}

	case HCI_TR_STATE_ERR:

	default:
		hciTrCb.trState = HCI_TR_STATE_IDLE;
		HCI_Tr_TimeDisable();
		break;
	}
}

_attribute_ram_code_
void HCI_Tr_H4IRQHandler(void)
{
	/* UART Normal Rx IRQ */
	if(REG_ADDR8(0x9d) & 0x08)
	{
		for(int i=0; i<((REG_ADDR8(0x9c)) & 0x0f); i++)
		{
			u8 res = HCI_Tr_ReceiveByte();
			HCI_Tr_RxHandler(res);
		}
		REG_ADDR8(0x9d) = 0x08;
	}

	/* UART DMA Tx IRQ */
	if(reg_dma_irq_status & BIT(1)) {
		reg_dma_irq_status = BIT(1);//clear
	}

	/* Timer timeout IRQ */
	if(reg_tmr_sta & BIT(0)){
		reg_tmr_sta = BIT(0);
		HCI_Tr_TimeDisable();
		hciTrCb.trState = HCI_TR_STATE_IDLE;
	}
}
#endif

/******************************************************************************/
/*********    UART DMA Mode                                       *************/
/******************************************************************************/
#if HCI_H4_DMA_MODE_EN
u8 h4TrBackupBuf[HCI_H4_TR_RX_BUF_SIZE];
u8 h4TrRxBuf[HCI_H4_TR_RX_BUF_SIZE * HCI_H4_TR_RX_BUF_NUM];

static hci_fifo_t h4TrRxFifo = {
	HCI_H4_TR_RX_BUF_SIZE,
	HCI_H4_TR_RX_BUF_NUM,
	HCI_H4_TR_RX_BUF_NUM - 1,
	0,0,
	&h4TrRxBuf[0],
};

/*!  HCI Transmit main control block */
typedef struct{
	hci_fifo_t   *pHciRxFifo;  /*!< Point to HCI rx fifo. */
	hci_fifo_t   *pRxFifo;     /*!< Point to H4 rx fifo.  */
	u8           *pBackUpBuf;
	u8            isBackup;
	u8            backupCnt;
	u8            recvd1;
	u8            recvd2;
}HciH4TrCb_t;

static HciH4TrCb_t hciH4TrCB;

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
	hciH4TrCB.isBackup   = 0;
	hciH4TrCB.backupCnt  = 0;

	/* Timer configuration. */
	HCI_Tr_TimeInit(1000);

	/* UART configuration. */
	uart_gpio_set(HCI_TR_TX_PIN, HCI_TR_RX_PIN);
	uart_reset();
	
#if 0
#if 1//(MCU_CORE_TYPE == MCU_CORE_825x)
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
#else ///8278 use 1000000 for xiaomi project.
	//baud rate: 1000000
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

	uart_recbuff_init(hciH4TrCB.pRxFifo->p, hciH4TrCB.pRxFifo->size);

	/* UART Rx/Tx use DMA mode. */
	uart_dma_enable(1, 1);
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);
	uart_irq_enable(0, 0);
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
		hciH4TrCB.isBackup = true;

		HCI_Tr_TimeEnable();
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

				hciH4TrCB.isBackup = false;
				hciH4TrCB.backupCnt = 0;
				HCI_Tr_TimeDisable();
			}
			else
			{
				HCI_Tr_TimeEnable();
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

					hciH4TrCB.isBackup = false;
					hciH4TrCB.backupCnt = 0;
					HCI_Tr_TimeDisable();
				}
				else{
					HCI_Tr_TimeEnable();
				}
				return cnt;
			}
			else
			{
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, len);
				hciH4TrCB.backupCnt += len;
				HCI_Tr_TimeEnable();
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

				hciH4TrCB.isBackup = false;
				hciH4TrCB.backupCnt = 0;
				HCI_Tr_TimeDisable();
			}
			else
			{
				HCI_Tr_TimeEnable();
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

					hciH4TrCB.isBackup = false;
					hciH4TrCB.backupCnt = 0;
					HCI_Tr_TimeDisable();
				}
				else{
					HCI_Tr_TimeEnable();
				}
				return cnt;
			}
			else
			{
				memcpy(hciH4TrCB.pBackUpBuf + hciH4TrCB.backupCnt, pPacket, len);
				hciH4TrCB.backupCnt += len;
				HCI_Tr_TimeEnable();
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
		if(hciH4TrCB.isBackup){
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
	/* Timer timeout IRQ */
	if(reg_tmr_sta & BIT(0)){
		reg_tmr_sta = BIT(0);
		HCI_Tr_TimeDisable();
		hciH4TrCB.isBackup = false;
		hciH4TrCB.backupCnt = 0;
	}

	/* UART DMA Rx IRQ */
	if(reg_dma_irq_status & BIT(0))
	{
		reg_dma_irq_status = BIT(0);

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
	if(reg_dma_irq_status & BIT(1)) {
		reg_dma_irq_status = BIT(1);//clear
	}
}
#endif
