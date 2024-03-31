/*
 *******************************************************************************
 *  FileName  : hci_dfu.c
 *  Created on: 2021-05-07
 *  Author    : GaoQiu
 *  Copyright :
 *******************************************************************************
 */

#include "hci_dfu.h"
#include "hci_tr_def.h"
#include "hci_dfu_port.h"
#include "hci_dfu_api.h"

#include "drivers.h"
#include "stack/ble/ble_common.h"
#include "stack/ble/hci/hci_const.h"
#include "stack/ble/hci/hci.h"
#include "app_buffer.h"

#define			BLUETOOTH_VER_5_0				9
#define 		BLUETOOTH_VER         			BLUETOOTH_VER_5_0
#define         VENDOR_ID                       0x0211

extern hci_fifo_t bltHci_txfifo;

/* Variable declare ***********************************************************/
DfuCb_t dfuCb;

void HCI_VS_StartDfuCmdHandler(u8 *pParam, u32 len)
{
	u8 *p = pParam;
	u16 opcode  = HCI_OPCODE_VS_START_DFU;
	u8  dfuMode = 0;
	u16 fwVer   = 0;
	u32 fwSize  = 0;

	BSTREAM_TO_UINT8(dfuMode, p);
	BSTREAM_TO_UINT16(fwVer,  p);
	BSTREAM_TO_UINT32(fwSize, p);

	if(dfuCb.status == DFU_STA_START){
		Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_DFU_ENABLEED);
		return;
	}

	if(fwSize > dfuCb.maxFwSize){
		DFU_Reset();
		Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_INVALID_PARAM);
		return;
	}

	switch(dfuMode)
	{
	case DFU_MODE_FW_UPD:
	{
		goto DFU_START;
	}
	case DFU_MODE_FW_UPD_LATEST:
	{
		u16 localVer = Hci_Revision();
		if(fwVer <= localVer){
			Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_FW_VER);
		}else{
			goto DFU_START;
		}
		break;
	}
	case DFU_MODE_FW_UPD_OLDER:
	{
		u16 localVer = Hci_Revision();
		if(fwVer >= localVer){
			Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_FW_VER);
		}else{
			goto DFU_START;
		}
		break;
	}
	default:
		Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_INVALID_PARAM);
		break;
	}
	return;//[!!!important]

DFU_START:
	DFU_Reset();
	dfuCb.status    = DFU_STA_START;
	dfuCb.newFwSize = fwSize;
	dfuCb.timer     = clock_time()|1;//start timer

	u8 param[3] = {0};
	param[0] = HCI_SUCCESS; //status
	param[1] = DFU_MAX_PAYLOAD; //max payload length. TODO: according to HCI ACL buffer size
	param[2] = 0;
	Hci_SendCmdCmplEvt(opcode, param, sizeof(param));
}

void HCI_VS_EndDfuCmdHandler(u8 *pParam, u32 len)
{
	u8 *p = pParam;
	u16 opcode  = HCI_OPCODE_VS_END_DFU;
	u8  endMode = 0;

	BSTREAM_TO_UINT8(endMode, p);

	dfuCb.timer = clock_time()|1;//reset timer

	switch(endMode)
	{
	case DFU_END_MODE_FW_UPD:
		if(dfuCb.fwOffset != dfuCb.newFwSize){
			Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_FW_INCOMPLETE);
			dfuCb.status = DFU_STA_TERM;
		}
		else if(dfuCb.curCrc != 0x00000000){
			Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_FW_CRC);
			dfuCb.status = DFU_STA_TERM;
		}
		else{
			u8 bootFlag = 0x4b;
			u32 baswAddr = dfuCb.nextFwAddrStart;
			FLASH_WritePage(dfuCb.nextFwAddrStart + 0x8, &bootFlag, 1);

			bootFlag = 0x00;
			baswAddr = dfuCb.nextFwAddrStart==0 ? dfuCb.newFwAddrStart:0;
			FLASH_WritePage(baswAddr + 0x8, &bootFlag, 1);

			Hci_SendCmdCmplStatusEvt(opcode, HCI_SUCCESS);

			dfuCb.status = DFU_STA_END;
		}
		break;

	case DFU_END_MODE_TERM_UPD:
		dfuCb.status = DFU_STA_TERM;
		Hci_SendCmdCmplStatusEvt(opcode, HCI_SUCCESS);
		DFU_TRACK_INFO("DFU_Term...\n");
		break;

	default:
		Hci_SendCmdCmplStatusEvt(opcode, HCI_ERR_INVALID_PARAM);
		break;
	}
}

void HCI_VS_FwDataCmdHandler(u8 *pParam, u32 len)
{
	u16 opcode  = HCI_OPCODE_VS_FW_DATA;

	u32 dataLen = len - DFU_CHECKSUM_LEN;
	u32 checkSum = 0;
	BYTES_TO_UINT32(checkSum, pParam + dataLen);

	//TODO:
	if(dfuCb.resendCnt >= DFU_RESEND_CNT){
		return;
	}

	dfuCb.timer = clock_time()|1;//reset timer

	/* Check if DFU is started. */
	if(dfuCb.status != DFU_STA_START){
		Hci_SendCmdStatusEvt(opcode, HCI_ERR_DFU_DISABLED);
		return;
	}

	/* Check if payload length is valid. */
	if(dataLen > DFU_MAX_PAYLOAD){
		Hci_SendCmdStatusEvt(opcode, HCI_ERR_DATA_LENGTH);
		return;
	}

	/* Check if CRC is correct. */
	if(DFU_Crc32Calc(DFU_CRC_INIT_VALUE, pParam, len) != 0x00000000){
		dfuCb.resendCnt++;
		Hci_SendCmdStatusEvt(opcode, HCI_ERR_FW_CHECKSUM);
	}
	else{
		dfuCb.resendCnt = 0;

		if(dfuCb.fwOffset < dfuCb.newFwSize)
		{
			u8 bootFlag = 0;
			if(dfuCb.fwOffset == 0 && dataLen >= 0x8){//TODO: max_payload must >= 8 Bytes
				bootFlag = pParam[8];
				pParam[8] = 0xff;
			}
			/* Write data to flash. */
			FLASH_WritePage(dfuCb.nextFwAddrStart + dfuCb.fwOffset, pParam, dataLen);

			/* Calculate firmware CRC */
			u8 buf[DFU_MAX_PAYLOAD + 4]={0};
			FLASH_ReadPage(dfuCb.nextFwAddrStart + dfuCb.fwOffset, buf, dataLen);
			if(dfuCb.fwOffset == 0){
				buf[8] = bootFlag;
			}
			dfuCb.curCrc = DFU_Crc32Calc(dfuCb.curCrc, buf, dataLen);

			dfuCb.fwOffset += dataLen;

			Hci_SendCmdStatusEvt(opcode, HCI_SUCCESS);
			//DFU_TRACK_INFO("Data: ");
			//DFU_TRACK_DATA(pParam, dataLen);
		}
		else{
			Hci_SendCmdStatusEvt(opcode, HCI_ERR_FW_INCOMPLETE);
		}
	}
}

void HCI_VS_CmdHandler(u8 *pHciTrPkt, u32 len)
{
	u16 opcode;
	u16 paramLen;

	u8 *pPkt = pHciTrPkt;
	pPkt++;//skip HCI type;

	BSTREAM_TO_UINT16(opcode, pPkt);
	BSTREAM_TO_UINT8(paramLen, pPkt);

	switch(opcode)
	{
	case HCI_OPCODE_VS_START_DFU:
		DFU_TRACK_INFO("Rx HCI_Start_Dfu_Cmd...\n");
		HCI_VS_StartDfuCmdHandler(pPkt, paramLen);
		break;

	case HCI_OPCODE_VS_FW_DATA:
		DFU_TRACK_INFO("Rx HCI_Fw_Data_Cmd...\n");
		HCI_VS_FwDataCmdHandler(pPkt, paramLen);
		break;

	case HCI_OPCODE_VS_END_DFU:
		DFU_TRACK_INFO("Rx HCI_End_Dfu_Cmd...\n");
		HCI_VS_EndDfuCmdHandler(pPkt, paramLen);
		break;

	case HCI_OPCODE_RESET:
		DFU_TRACK_INFO("Rx HCI_Reset_Cmd...\n");
		Hci_Reset();

		dfuCb.status = DFU_STA_TERM;

		Hci_SendCmdCmplStatusEvt(opcode, HCI_SUCCESS);
		break;

	case HCI_OPCODE_READ_LOCAL_VER_INFO:
	{
		DFU_TRACK_INFO("Rx HCI_Read_Local_Ver_Cmd...\n");
		u8 buf[9] = {0};
		u8 *p = buf;
		UINT8_TO_BSTREAM(p,  HCI_SUCCESS);   //status
		UINT8_TO_BSTREAM(p,  BLUETOOTH_VER); //HCI version
		UINT16_TO_BSTREAM(p, Hci_Revision());//HCI revision
		UINT8_TO_BSTREAM(p,  BLUETOOTH_VER); //LMP version
		UINT16_TO_BSTREAM(p, VENDOR_ID);     //Company id
		UINT16_TO_BSTREAM(p, 0x0000);        //LMP subversion
		Hci_SendCmdCmplEvt(opcode, buf, sizeof(buf));
		break;
	}
	default:
		Hci_SendCmdStatusEvt(opcode, HCI_ERR_CMD_DISALLOWED);//HCI_ERR_UNKNOWN_HCI_CMD
		break;
	}
}

void DFU_EraseNewFwArea(void)
{
	u32 tmp1 = 0;
	u32 tmp2 = 0;
	u32 tmp3 = 0;

	FLASH_ReadPage(dfuCb.nextFwAddrStart, (u8*)&tmp1, 4);
	FLASH_ReadPage(dfuCb.nextFwAddrStart + (dfuCb.maxFwSize/0x1000/2)*0x1000, (u8*)&tmp2, 4);
	FLASH_ReadPage(dfuCb.nextFwAddrStart + dfuCb.maxFwSize - 4, (u8*)&tmp3, 4);

	if(tmp1 != 0xFFFFFFFF || tmp2 != 0xFFFFFFFF || tmp3 != 0xFFFFFFFF)
	{
		for(int i=0; i<dfuCb.maxFwSize; i += 0x1000)
		{
			FLASH_EraseSector(dfuCb.nextFwAddrStart + i);
		}
	}
}

bool DFU_isEnable(void)
{
	return (dfuCb.status == DFU_STA_START ? true:false);
}

void DFU_Reset(void)
{
	dfuCb.newFwSize = 0;
	dfuCb.fwOffset  = 0;
	dfuCb.curCrc    = DFU_CRC_INIT_VALUE;

	dfuCb.timer     = 0;
	dfuCb.status    = 0;
	dfuCb.resendCnt = 0;
}

void DFU_Init(void)
{
	memset((u8*)&dfuCb, 0, sizeof(DfuCb_t));

	dfuCb.newFwAddrStart = DFU_NEW_FW_ADDR_BASE;
	dfuCb.maxFwSize      = DFU_NEW_FW_MAX_SIZE;//128K - 4K (4K is invalid)

	dfuCb.nextFwAddrStart = DFU_NEW_FW_ADDR_BASE;
	u16 bootFlag = REG_ADDR16(0x63e);
	if (bootFlag)
	{
		//0x63e unit is 1024, so >>10 to meet that.such as 128K,0x63e require 128 not 128*1024(0x20000).
		bootFlag = (bootFlag & 0x0007) | (((dfuCb.maxFwSize>>10)>>2)<<3);//must be 4k aligned.
		REG_ADDR16(0x63e) = bootFlag;
		dfuCb.nextFwAddrStart = 0;
	}

	//dfuCb.timer     = 0;
	//dfuCb.status    = 0;
	//dfuCb.fwOffset  = 0;
	//dfuCb.newFwSize = 0;

	u32 tmp1 = 0;
	u32 tmp2 = 0;
	u32 tmp3 = 0;

	FLASH_ReadPage(dfuCb.nextFwAddrStart, (u8*)&tmp1, 4);
	FLASH_ReadPage(dfuCb.nextFwAddrStart + (dfuCb.maxFwSize/0x1000/2)*0x1000, (u8*)&tmp2, 4);
	FLASH_ReadPage(dfuCb.nextFwAddrStart + dfuCb.maxFwSize - 4, (u8*)&tmp3, 4);

	if(tmp1 != 0xFFFFFFFF || tmp2 != 0xFFFFFFFF || tmp3 != 0xFFFFFFFF)
	{
		for(int i=0; i<dfuCb.maxFwSize; i += 0x1000)
		{
			FLASH_EraseSector(dfuCb.nextFwAddrStart + i);
		}
	}

	Hci_SetRevision(DFU_FW_VERSION);
}

void DFU_TaskStart(void)
{
	hci_fifo_t *pTxFifo = &bltHci_txfifo;

	/* Wait for HCI_Cmd_Cmpl_Evt sent. */
	if(dfuCb.status == DFU_STA_END && pTxFifo->wptr == pTxFifo->rptr && !UART_IsBusy()){
		dfuCb.status = DFU_STA_NONE;
		MCU_Reset();
	}

	if(dfuCb.status == DFU_STA_TERM && pTxFifo->wptr == pTxFifo->rptr && !UART_IsBusy()){
		dfuCb.status = DFU_STA_NONE;
		DFU_Reset();
		DFU_TRACK_INFO("DFU_Term exec...\n");
		DFU_EraseNewFwArea();
	}

	/* DFU timeout handle */
	if(dfuCb.timer && clock_time_exceed(dfuCb.timer, DFU_TIMEOUT * 1000*1000)){
		DFU_EraseNewFwArea();
		DFU_Reset();
	}
}

//x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x+ 1
//0x04C11DB7
u32 DFU_Crc32Calc(u32 crc, u8* data, u32 len)
{
    while(len--)
    {
        crc ^= *data++;

        for(u8 i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}
