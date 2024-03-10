/********************************************************************************************************
 * @file     eatt.h 
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     Sep. 18, 2019
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *           
 *			 The information contained herein is confidential and proprietary property of Telink 
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms 
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai) 
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in. 
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this 
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided. 
 *           
 *******************************************************************************************************/
#ifndef GATT_H_
#define GATT_H_


#if (L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN)


#define BLC_EATT_NEED_LEN(sdu, cids, credits)


int blc_eatt_registerService(u8 srvCount, u8 chCount, u16 mtu, u16 mps,
	u16 credits, u8 *pBuffer, blc_l2cap_coc_evtcb evtcb);
int blc_eatt_startService(u16 handle);
int blc_eatt_stopService(u16 handle);

int blc_eatt_sendData(u16 handle, u16 scid, u8 *pHead, u8 headLen, u8 *pData, u16 dataLen, bool cidBusy);


int blc_eatt_sendErrorRsp(u16 connHandle, u16 scid, u8 errOpcode, u16 errHandle, u8 errReason);

int blc_eatt_sendReadByTypeReq(u16 connHandle, u16 scid, u16 startAttHandle, 
	u16 endAttHandle, u8 *uuid, int uuidLen);
int blc_eatt_sendReadByTypeRsp(u16 connHandle, u16 scid, u8 typeLen, u8 dataLen, u8 *pData);

int blc_eatt_sendReadByGroupTypeReq(u16 connHandle, u16 scid, u16 startAttHandle, 
	u16 endAttHandle, u8 *uuid, int uuidLen);
int blc_eatt_sendReadByGroupTypeRsp(u16 connHandle, u16 scid, u8 typeLen, u8 *pData, u16 datalen);

int blc_eatt_sendFindInfoReq(u16 connHandle, u16 scid, u16 startAttHandle, u16 endAttHandle);
int blc_eatt_sendFindInfoRsp(u16 connHandle, u16 scid, u8 format, u8 dataLen, u8 *pData);

int blc_eatt_sendFindByTypeReq(u16 connHandle, u16 scid, u16 startAttHandle, u16 endAttHandle, 
	u8 *pUuid, u8* pAttrValue, int valueLen);
int blc_eatt_sendFindByTypeRsp(u16 connHandle, u16 scid, u8 dataLen, u8 *pData);

int blc_eatt_sendReadReq(u16 connHandle, u16 scid, u16 attHandle);
int blc_eatt_sendReadRsp(u16 connHandle, u16 scid, u8 dataLen, u8 *pData);
int blc_eatt_sendReadBlobReq(u16 connHandle, u16 scid, u16 attHandle, u16 offset);
int blc_eatt_sendReadBlobRsp(u16 connHandle, u16 scid, u8 dataLen, u8 *pData);

int blc_eatt_sendReadMultipleReq(u16 connHandle, u16 scid, u8 numHandles, u16 *pHandle);
int blc_eatt_sendReadMultVarReq(u16 connHandle, u16 scid, u8 numHandles, u16 *pHandle);
int blc_eatt_sendReadMultVarRsp(u16 connHandle, u16 scid, u8 numVars, u16 *pVarLen);

int blc_eatt_pushNotify(u16 connHandle, u16 scid, u16 attHandle, u16 dataLen, u8 *pData);
int blc_eatt_pushIndicate(u16 connHandle, u16 scid, u16 attHandle, u16 dataLen, u8 *pData);
int blc_eatt_sendWriteReq(u16 connHandle, u16 scid, u16 attHandle, u16 dataLen, u8 *pData);
int blc_eatt_sendWriteRsp(u16 connHandle, u16 scid);
int blc_eatt_sendWriteCmd(u16 connHandle, u16 scid, u16 attHandle, u16 dataLen, u8 *pData);
int blc_eatt_sendPrepareWriteReq(u16 connHandle, u16 scid, u16 attHandle, 
	u16 offset, u16 dataLen, u8 *pData);
int blc_eatt_sendPrepareWriteRsp(u16 connHandle, u16 scid, u16 attHandle, 
	u16 offset, u16 dataLen, u8 *pData);
int blc_eatt_sendExecuteWriteReq(u16 connHandle, u16 scid, u8 flag);
int blc_eatt_sendExecuteWriteRsp(u16 connHandle, u16 scid);



#endif //#if (L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN)

#endif //#ifndef GATT_H_
