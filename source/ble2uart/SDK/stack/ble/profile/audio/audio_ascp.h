/********************************************************************************************************
 * @file	audio_ascp.h
 *
 * @brief	This is the header file for BLE SDK
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
#ifndef _AUDIO_BAP_H_
#define _AUDIO_BAP_H_

#if (BLC_AUDIO_ASCP_ENABLE)



typedef enum{
	BLT_AUDIO_ASCP_FLAG_NONE  = 0x0000,
	BLT_AUDIO_ASCP_FLAG_FIRST = 0x0001,
	BLT_AUDIO_ASCP_FLAG_RASE  = 0x0002, //Read Ase Attribute
	BLT_AUDIO_ASCP_FLAG_CCC   = 0x0004,
}BLT_AUDIO_ASCP_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_ASE_STATE_IDLE       = 0x00,
	BLT_AUDIO_ASE_STATE_CODEC_CFG  = 0x01,
	BLT_AUDIO_ASE_STATE_QOS_CFG    = 0x02,
	BLT_AUDIO_ASE_STATE_ENABLING   = 0x03,
	BLT_AUDIO_ASE_STATE_STREAMING  = 0x04,
	BLT_AUDIO_ASE_STATE_DISABLING  = 0x05,
	BLT_AUDIO_ASE_STATE_RELEASING  = 0x06,
}BLT_AUDIO_ASE_STATE_ENUM;
typedef enum{
	BLT_AUDIO_ASE_FLAG_NONE          = 0x0000,

	BLT_AUDIOC_ASE_FLAG_ENABLE       = 0x8000,
	BLT_AUDIOC_ASE_FLAG_DISABLE      = 0x4000,
	BLT_AUDIOC_ASE_FLAG_SEND_WAIT    = 0x2000,
	BLT_AUDIOC_ASE_FLAG_SEND_CODEC   = 0x1000,
	BLT_AUDIOC_ASE_FLAG_SEND_QOS     = 0x0800,
	BLT_AUDIOC_ASE_FLAG_SEND_ENABLE  = 0x0400,
	BLT_AUDIOC_ASE_FLAG_SEND_DISABLE = 0x0200,
	BLT_AUDIOC_ASE_FLAG_SEND_START   = 0x0100,
	BLT_AUDIOC_ASE_FLAG_SEND_STOP    = 0x0080,
	BLT_AUDIOC_ASE_FLAG_SEND_RELEASE = 0x0040,
	BLT_AUDIOC_ASE_FLAG_SEND_UPDATE  = 0x0020,
	
	BLT_AUDIOC_ASE_FLAG_ENABLE_MASK  = BLT_AUDIOC_ASE_FLAG_SEND_CODEC|BLT_AUDIOC_ASE_FLAG_SEND_QOS
	                                 | BLT_AUDIOC_ASE_FLAG_SEND_ENABLE|BLT_AUDIOC_ASE_FLAG_SEND_START
	                                 | BLT_AUDIOC_ASE_FLAG_ENABLE,
	BLT_AUDIOC_ASE_FLAG_DISABLE_MASK = BLT_AUDIOC_ASE_FLAG_SEND_DISABLE|BLT_AUDIOC_ASE_FLAG_SEND_STOP
	                                 | BLT_AUDIOC_ASE_FLAG_SEND_RELEASE|BLT_AUDIOC_ASE_FLAG_DISABLE,

	BLT_AUDIOC_ASE_FLAG_CREATE_CIS   = 0x0010,
	BLT_AUDIOC_ASE_FLAG_DESTORY_CIS  = 0x0008,
	BLT_AUDIOC_ASE_FLAG_CREATE_CIS_WAIT  = 0x0004,
	BLT_AUDIOC_ASE_FLAG_DESTORY_CIS_WAIT = 0x0002,
	
	BLT_AUDIOS_ASE_FLAG_NOTY_STATE  = 0x0001,
	
}BLT_AUDIO_ASE_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_ASE_READY_NONE   = 0x0000,
	BLT_AUDIO_ASE_PARAM_READY  = 0x8000,
	BLT_AUDIO_ASE_CODEC_READY  = 0x0001,
}BLT_AUDIO_ASE_READY_ENUM;

typedef enum{
	BLT_AUDIO_ASCS_OPCODE_CONFIG_CODEC            = 0x01,
	BLT_AUDIO_ASCS_OPCODE_CONFIG_QOS              = 0x02,
	BLT_AUDIO_ASCS_OPCODE_CONFIG_ENABLE           = 0x03,
	BLT_AUDIO_ASCS_OPCODE_CONFIG_RECV_START       = 0x04,
	BLT_AUDIO_ASCS_OPCODE_CONFIG_DISABLE          = 0x05,
	BLT_AUDIO_ASCS_OPCODE_CONFIG_RECV_STOP        = 0x06,
	BLT_AUDIO_ASCS_OPCODE_CONFIG_UPDATE_METADATA  = 0x07,
	BLT_AUDIO_ASCS_OPCODE_CONFIG_RELEASE          = 0x08,
}BLT_AUDIO_ASCS_OPCODE_ENUM;
typedef enum{
	BLT_AUDIO_ASCS_ERRCODE_NONE                   = 0x0000,
	BLT_AUDIO_ASCS_ERRCODE_UNSUPPORTED_OPCODE     = 0x0001,
	BLT_AUDIO_ASCS_ERRCODE_TRUNCATED_OPERATION    = 0x0002,
	BLT_AUDIO_ASCS_ERRCODE_INVALID_ASE_ID         = 0x0003,
	BLT_AUDIO_ASCS_ERRCODE_INVALID_ASE_STATE      = 0x0004,
	BLT_AUDIO_ASCS_ERRCODE_UNSUPPORTED_CAPABILITY = 0x0005, // need  ASCS_REASON_ENUM
	BLT_AUDIO_ASCS_ERRCODE_UNSUPPORTED_CONFIG     = 0x0006, // need  ASCS_REASON_ENUM
	BLT_AUDIO_ASCS_ERRCODE_REJECTED_CONFIG        = 0x0007, // need  ASCS_REASON_ENUM
	BLT_AUDIO_ASCS_ERRCODE_INVALID_CONFIG         = 0x0008, // need  ASCS_REASON_ENUM
	BLT_AUDIO_ASCS_ERRCODE_UNSUPPORTED_METADATA   = 0x0009, // need  Value of the Metadata Type field in error
	BLT_AUDIO_ASCS_ERRCODE_REJECTED_METADATA      = 0x000A, // need  Value of the Metadata Type field in error
	BLT_AUDIO_ASCS_ERRCODE_INVALID_METADATA       = 0x000B, // need  Value of the Metadata Type field in error
	BLT_AUDIO_ASCS_ERRCODE_INSUFFICIENT_RESOURCE  = 0x000C,
	BLT_AUDIO_ASCS_ERRCODE_UNSPECIFIED_ERROR      = 0x000D,
}BLT_AUDIO_ASCS_ERRCODE_ENUM;
typedef enum{
	BLT_AUDIO_ASCS_REASON_DIRECTION               = 0x01,
	BLT_AUDIO_ASCS_REASON_CODEC_ID                = 0x02,
	BLT_AUDIO_ASCS_REASON_CONFIG_LENGTH           = 0x03,
	BLT_AUDIO_ASCS_REASON_CONFIG                  = 0x04,
	BLT_AUDIO_ASCS_REASON_SDU_INTERVAL            = 0x05,
	BLT_AUDIO_ASCS_REASON_FRAMING                 = 0x06,
	BLT_AUDIO_ASCS_REASON_PHY                     = 0x07,
	BLT_AUDIO_ASCS_REASON_SDU_SIZE                = 0x08,
	BLT_AUDIO_ASCS_REASON_RETRANS_NUMBER          = 0x09,
	BLT_AUDIO_ASCS_REASON_MAX_LATENCY             = 0x0A,
	BLT_AUDIO_ASCS_REASON_PRESENT_DELAY           = 0x0B,
	BLT_AUDIO_ASCS_REASON_METADATA_LENGTH         = 0x0C,
}BLT_AUDIO_ASCS_REASON_ENUM;




typedef struct{ // 42Bytes
	u8  direction;                    /*<! audio sink and audio source*/
	
	u8  prefSDUMinInterval[3];        /*<! Range: 0x0000FF - 0xFFFFFF */
	u8  prefSDUMaxInterval[3];        /*<! Range: 0x0000FF - 0xFFFFFF */
	u8  prefFraming;                  /*<! Preferred Frame            */
	u8  prefPHY;                      /*<! Preferred PHY              */
	u16 prefMaxSDU;
	u8  prefRetransmitNum;            /*<! Range: 0x00 - 0xFF         */
	u16 prefMaxTransportLatency;      /*<! Range: 0x0005-0x0FA0       */
	u8  prefMinPresentationDelay[3];  /*<! Unit: us                   */
	u8  prefMaxPresentationDelay[3];  /*<! Unit: us                   */
	
	u8  codecId[5];
	u8  codecSpecificCfgLen;
	u8  codecSpecificCfg[16];          /*<! max = 16Byte. Shall exist only if the Codec_Specific_Configuration_Length field is not 0x00. */
}blt_audio_ascpCodecParam_t;
typedef struct{ // 15Bytes
	u8  cigID;
	u8  cisID;
	u8  SDUInterval[3];
	u8  framing;              // refer AUDIO_FRAMING_UNFRAMED
	u8  PHY;                  // refer AUDIO_PHY_FLAG_1M
	u16 maxSDU;               /*<! Range: 0x00C0xFFF        */
	u8  retransmitNum;        /*<! Range: 0x00C0x0F          */
	u16 maxTransportLatency;  /*<! Range: 0x0005C0x0FA0      */
	u8  presentationDelay[3]; /*<! Unit: us                  */
}blt_audio_ascpQosParam_t;
typedef struct{ // 7Bytes
	u8  cigID;
	u8  cisID;
	u8  metaDataLen;
	u8  metaData[4];
}blt_audio_ascpEnableParam_t, blt_audio_ascpDisableParam_t, blt_audio_ascpStreamParam_t;
typedef struct{
	u8 aseID;
	u8 state; // refer ASCS_ASE_STATE_ENUM
	union{
		blt_audio_ascpCodecParam_t   codec;
		blt_audio_ascpQosParam_t     qos;
		blt_audio_ascpEnableParam_t  enable;
		blt_audio_ascpDisableParam_t disable;
		blt_audio_ascpStreamParam_t  stream;
	}u;
}blt_audio_ascpState_t;

typedef struct{
	u8  SDUMinInterval[3];        /*<! Range: 0x0000FF - 0xFFFFFF */
	u8  SDUMaxInterval[3];        /*<! Range: 0x0000FF - 0xFFFFFF */
	u8  framing;                  /*<! Preferred Frame            */
	u8  PHY;                      /*<! Preferred PHY              */
	u16 maxSDU;
	u8  retransmitNum;            /*<! Range: 0x00 - 0x0F         */
	u16 maxTransportLatency;      /*<! Range: 0x0005C0x0FA0      */
	u8  minPresentationDelay[3];  /*<! Unit: us                   */
	u8  maxPresentationDelay[3];  /*<! Unit: us                   */
}blt_audio_ascpPreferredParam_t;
typedef struct{
	u8  frequency;
	u8  duration;
	u16 frameOcts;
	u32 allocation;
}blt_audio_ascpSpecificParam_t;
typedef struct{
	u16 streamingContext;
}blt_audio_ascpMetadataParam_t;



int blc_audio_ascpAttRead(u16 connHandle, void *p);
int blc_audio_ascpAttWrite(u16 connHandle, void *p);

int blc_audio_ascpCisRequestEvt(u16 connHandle, u16 cisHandle, u8 *pPkt);
int blc_audio_ascpCisConnectEvt(u16 connHandle, u16 cisHandle, u8 *pPkt);
int blc_audio_ascpCisDisconnEvt(u16 connHandle, u16 cisHandle, u8 *pPkt);
int blc_audio_ascpRemoveCIGByAse(u16 connHandle, u8 aseID);

int blc_audio_ascpSetAseCfg(u16 connHandle, u8 aseID, blc_audio_aseConfig_t *pCfg);
int blc_audio_ascpSetAllAseCfg(u16 connHandle, blc_audio_aseConfig_t *pCfg);

int blc_audio_ascpEnableAse(u16 connHandle, u8 aseID);
int blc_audio_ascpDisableAse(u16 connHandle, u8 aseID);
int blc_audio_ascpReleaseAse(u16 connHandle, u8 aseID);
int blc_audio_ascpSendCtrlData(u16 connHandle, u8 aseID, u8 *pData, u16 dataLen);




static int  blt_audio_ascpInit(blt_audio_handle_t *pHandle);
static int  blt_audio_ascpGattIn(blt_audio_handle_t *pHandle, u8 *pPkt);
static int  blt_audio_ascpSetServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static void blt_audio_ascpSetConn(blt_audio_handle_t *pHandle, BLC_AUDIO_STATE_ENUM state);
static void blt_audio_ascpProcess(blt_audio_handle_t *pHandle);

static int  blt_audio_ascpCreateCis(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static void blt_audio_ascpAseClientHandler(blt_audio_handle_t *pHandle, u8 aseNum);
static void blt_audio_ascpAseServerHandler(blt_audio_handle_t *pHandle, u8 aseNum);
static int  blt_audio_ascpClientNotyHandler(blt_audio_handle_t *pHandle, blt_audio_attNotify_t *pNoty);

static u8 blt_audio_ascpCisIndex(blt_audio_handle_t *pHandle, u8 cigID, u8 cisID, u8 excludeAseID);
static u8 blt_audio_ascpCisIndex2(blt_audio_handle_t *pHandle, u8 cigID, u8 cisID);

static blt_audio_ascpAse_t *blt_audio_ascpFindAse(blt_audio_handle_t *pHandle, u8 aseID);
static u8 blt_audio_ascpGetSpecificParam(u8 specLen, u8 *pSpecific, blt_audio_ascpSpecificParam_t *pParam);
static u8 blt_audio_ascpGetMetadataParam(u8 metaLen, u8 *pMeta, blt_audio_ascpMetadataParam_t *pParam);

static int blt_audio_ascpSendAseCodec(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static int blt_audio_ascpSendAseQOS(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static int blt_audio_ascpSendAseEnable(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static int blt_audio_ascpSendAseStart(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static int blt_audio_ascpSendAseDisable(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static int blt_audio_ascpSendAseStop(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static int blt_audio_ascpSendAseUpdate(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static int blt_audio_ascpSendAseRelease(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);

static bool blt_audio_ascpSendAseState(blt_audio_handle_t *pHandle, u8 num, u8 isNotify);
static void blt_audio_ascpConfigHandle(blt_audio_handle_t *pHandle, u8 opcode, u8 dataLen, u8 *pData);

static void blt_audio_ascpSendEnableEvt(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse,
	u8 errCode, u8 reason);
static void blt_audio_ascpSendUpdateEvt(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static void blt_audio_ascpSendStartEvt(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static void blt_audio_ascpSendDisableEvt(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static void blt_audio_ascpSendStopEvt(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);
static void blt_audio_ascpSendReleaseEvt(blt_audio_handle_t *pHandle, blt_audio_ascpAse_t *pAse);



#endif //#if (BLC_AUDIO_ASCS_ENABLE)

#endif //_AUDIO_BAP_H_

