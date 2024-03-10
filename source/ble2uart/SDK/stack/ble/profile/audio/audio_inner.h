/********************************************************************************************************
 * @file	audio_inner.h
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
#ifndef _AUDIO_INNER_H_
#define _AUDIO_INNER_H_


#if (BLC_AUDIO_PROFILE_EN)


#define BLT_AUDIO_SINK_AES_MAX        2
#define BLT_AUDIO_SOURCE_AES_MAX      2



typedef enum{
	BLT_AUDIO_BUSY_NONE = 0x0000,
	BLT_AUDIO_BUSY_PACP = BIT(BLC_AUDIO_SERVICE_PACS),
	BLT_AUDIO_BUSY_ASCP = BIT(BLC_AUDIO_SERVICE_ASCS),
	BLT_AUDIO_BUSY_BASP = BIT(BLC_AUDIO_SERVICE_BASS),
	BLT_AUDIO_BUSY_CSIP = BIT(BLC_AUDIO_SERVICE_CSIS),
	BLT_AUDIO_BUSY_RAAP = BIT(BLC_AUDIO_SERVICE_RAAS),
	BLT_AUDIO_BUSY_MICP = BIT(BLC_AUDIO_SERVICE_MICS),
	BLT_AUDIO_BUSY_VCP  = BIT(BLC_AUDIO_SERVICE_VCS),
	BLT_AUDIO_BUSY_MCP  = BIT(BLC_AUDIO_SERVICE_MCS),
	BLT_AUDIO_BUSY_TBP  = BIT(BLC_AUDIO_SERVICE_TBS),
	BLT_AUDIO_BUSY_OTP  = BIT(BLC_AUDIO_SERVICE_OTS),
	
	BLT_AUDIO_BUSY_SDP  = BIT(14),
	BLT_AUDIO_BUSY_SDP_START = BIT(15),
}BLT_AUDIO_BUSY_ENUM;

typedef enum{
	BLT_AUDIO_GFLAG_NONE = 0x0000,
	BLT_AUDIO_GFLAG_PACS = BIT(0),
	BLT_AUDIO_GFLAG_ASCS = BIT(1),
	BLT_AUDIO_GFLAG_BACS = BIT(2),
	BLT_AUDIO_GFLAG_CSIS = BIT(3),
	BLT_AUDIO_GFLAG_RAAS = BIT(4),
	BLT_AUDIO_GFLAG_MICS = BIT(5),
	BLT_AUDIO_GFLAG_VCS  = BIT(6),
	BLT_AUDIO_GFLAG_MCS  = BIT(7),
	BLT_AUDIO_GFLAG_TBS  = BIT(8),
	BLT_AUDIO_GFLAG_OTS  = BIT(9),
}BLT_AUDIO_GFLAGS_ENUM;



#if (BLC_AUDIO_SDP_ENABLE)

#define BLT_AUDIO_SDP_SRV_COUNT       8
#define BLT_AUDIO_SDP_INC_COUNT       2
#define BLT_AUDIO_SDP_CHAR_COUNT      12
#define BLT_AUDIO_SDP_OTHR_COUNT      2

typedef struct blt_audio_srv_s blt_audio_srv_t;
typedef struct blt_audio_sdp_s blt_audio_sdp_t;
typedef void(*BlcAudioSdpFindServiceFunc)(blt_audio_sdp_t *pSdp, blt_audio_srv_t *pSrv);
typedef void(*BlcAudioSdpInitServiceFunc)(blt_audio_sdp_t *pSdp, blt_audio_srv_t *pSrv);
typedef void(*BlcAudioSdpLoopHandlerFunc)(blt_audio_sdp_t *pSdp, blt_audio_srv_t *pSrv);
typedef void(*BlcAudioSdpOverHandlerFunc)(blt_audio_sdp_t *pSdp, blt_audio_srv_t *pSrv);
typedef  int(*BlcAudioSdpGattHandlerFunc)(blt_audio_sdp_t *pSdp, blt_audio_srv_t *pSrv, u8 *pkt);

typedef struct{
	u16 sHandle;
	u16 eHandle;
	u16 srvUUID;
}blt_audio_inc_t;
typedef struct{ // 12Bytes
	u16 charUUID;
	u16 dHandle; //data handle
	u16 othrUUID[BLT_AUDIO_SDP_OTHR_COUNT];
	u16 othrHandle[BLT_AUDIO_SDP_OTHR_COUNT];
}blt_audio_char_t;
struct blt_audio_srv_s{
	u8	used; //Adapter to multi same ServiceUUID.
	u8  mode; //Refer to BLT_AUDIO_CHAR_MODE_ENUM.
	u8  flags; //Refer to BLT_AUDIO_SRV_FLAG_ENUM.
	u8  curChar; //
	u16 srvUUID;
	u16 sHandle;
	u16 eHandle;
	u16 oHandle;
	BlcAudioSdpFindServiceFunc findFunc;
	BlcAudioSdpInitServiceFunc initFunc;
	BlcAudioSdpLoopHandlerFunc loopFunc;
	BlcAudioSdpOverHandlerFunc overFunc;
	BlcAudioSdpGattHandlerFunc gattFunc;
};
struct blt_audio_sdp_s{
	u8  start;
	u8  flags;
	u8  other;
	u8  curSrv;
	u16 handle; //Connect Handle
	u16 sHandle;
	u16 eHandle;
	u16 oHandle;
	u32 startTimer;
	blt_audio_srv_t *servPtr[BLT_AUDIO_SDP_SRV_COUNT];
	blt_audio_inc_t includes[BLT_AUDIO_SDP_INC_COUNT];
	blt_audio_char_t charact[BLT_AUDIO_SDP_CHAR_COUNT]; //This will be Shared by multiple services
};
#endif //#if (BLC_AUDIO_SDP_ENABLE)



#if (BLC_AUDIO_AICS_ENABLE)
typedef struct{
	u16 flags;
	u16 resv0;
	u16 sHandle;
	u16 eHandle;
	u16 ctrlHandle;
	u16 stateHandle;
	u16 propeHandle;
	u16 inputHandle;
	u16 statusHandle;
	u16 descHandle;

	u8  gainStateValue;
	u8  gainStateMute;
	u8  gainStateMode;
	u8  gainStateCount;

	u8  gainPropeUnit;
	u8  gainPropeMin;
	u8  gainPropeMax;
	u8  reserve1;

	u8  gainType;
	u8  gainStatus;
	u8  stateCCC;
	u8  statusCCC;

	#if (BLC_AUDIO_AICS_DESC_ENABLE)
		u32 descLen;
		u8  desc[BLC_AUDIO_AICS_DESC_SIZE];
	#endif
}blt_audio_aics_t;
#endif //BLC_AUDIO_AICS_ENABLE
#if (BLC_AUDIO_VOCS_ENABLE)
typedef struct{
	u16 flags;
	u16 reserve;

	u16 sHandle;
	u16 eHandle;

	u16 stateHandle;
	u16 ctrlHandle;
	u16 locaHandle;
	u16 descHandle;

	u8  counter;
	u8  stateCCC;
	u16 voffset;
		
	u32 location;

	#if (BLC_AUDIO_VOCS_DESC_ENABLE)
		u32 descLen;
		u8  desc[BLC_AUDIO_VOCS_DESC_SIZE];
	#endif
}blt_audio_vocs_t;
#endif //BLC_AUDIO_VOCS_ENABLE

#if (BLC_AUDIO_PACP_ENABLE)
typedef struct{
	u16 flags;
	u16 resv0;
	u8  sinkPacLen;
	u8  srcPacLen;
	u16 avaHandle;
	u16 srcLcaHandle;
	u16 sinkLcaHandle;
	u8 *pSrcPac;
	u8 *pSrcLca; // source location
	u8 *pSinkPac;
	u8 *pSinkLca; // sink location
	u8 *pAvaCtx; // avaliable Context
	u8 *pSupCtx; // supported Context
}blt_audio_pacpCtrl_t;
#endif //#if (BLC_AUDIO_PACP_ENABLE)
#if (BLC_AUDIO_ASCP_ENABLE)
typedef struct{ // 36Bytes
	// Profile Param  16Byte
	u8  codecId[5];
	u8  direction;
	u8  frequency;
	u8  duration;
	u16 context;  // Metadata
	u16 frameOcts;
	u32 location;
	
	// Link Layer Param  16Bytes
	u8  cigID;
	u8  cisID;
	u8  PHY;
	u8  RTN; //Retransmission_Number
	u16 maxSDU;
	u16 latency;
	u8  framing;
	u8  resv000;
	u8  delay[3];
	u8  interval[3];
}blt_audio_aseParam_t;
typedef struct{ // 4Bytes
	u16 attHandle;
	u16 cisHandle;
	u16 flags; //Refer to BLT_AUDIO_ASE_FLAGS_ENUM
	u16 ready;
	u8  aseID;
	u8  state; //Refer to BLT_AUDIO_ASE_STATE_ENUM
	u16 reserve;
	u8  *pAseCCC;
	blt_audio_aseParam_t param;
}blt_audio_ascpAse_t;
typedef struct{
	u8  flags; //For client, this is SdpExFlags.
	u8  aseCount;
	u16 ctrlHandle;
	blt_audio_ascpAse_t ase[BLC_AUDIO_ASE_PER_HANDLE];
}blt_audio_ascpCtrl_t;
#endif //#if (BLC_AUDIO_ASCP_ENABLE)

#if (BLC_AUDIO_CSIP_ENABLE)
typedef struct{
	u8  flags;
	u8  isInLock; //Is Inner Lock
	u8  sdpFlags;
	u8  reserve0;
	u32 lockTimer;
	u32 lockTimeout; //us

	u16 sirkHandle;
	u16 sizeHandle;
	u16 lockHandle;
	u16 rankHandle;
	
	u8 size;
	u8 lock;
	u8 rank;
	u8 lockCCC;
	u8 SIRK[16];
}blt_audio_csipCtrl_t;
#endif //#if (BLC_AUDIO_CSIP_ENABLE)
#if (BLC_AUDIO_MICP_ENABLE)
typedef struct{
	u16 flags;
	u16 sdpFlags;
	u8  aicsCount;
	u8  sdpCount;
	u8  muteCCC;
	u8  muteValue;
	u16 reserve01;
	u16 muteHandle;
	#if (BLC_AUDIO_MICS_AICS_ENABLE)
	blt_audio_aics_t aics[BLC_AUDIO_MICS_AICS_COUNT];
	#endif
}blt_audio_micpCtrl_t;
#endif //#if (BLC_AUDIO_MICP_ENABLE)

#if (BLC_AUDIO_VCP_ENABLE)
typedef struct{
	u16 flags;
	u16 sdpFlags;
	u8  sdpCount;
	u8  aicsCount;
	u8  vocsCount;
	u8  flagCCC;
	u8  flagValue;
	u8  reserve00;
	u16 statHandle;
	u16 flagHandle;
	u16 ctrlHandle;
	struct{
		u8 mute;
		u8 volume;
		u8 counter;
		u8 stateCCC;
	}state;
	#if (BLC_AUDIO_VCS_AICS_ENABLE)
	blt_audio_aics_t aics[BLC_AUDIO_VCS_AICS_COUNT];
	#endif
	#if (BLC_AUDIO_VOCS_ENABLE)
	blt_audio_vocs_t vocs[BLC_AUDIO_VCS_VOCS_COUNT];
	#endif //BLC_AUDIO_VOCS_ENABLE
}blt_audio_vcpCtrl_t;
#endif //#if (BLC_AUDIO_VCP_ENABLE)

#if (BLC_AUDIO_MCP_ENABLE)
typedef struct{
	u16 flags;
	u8  enable;
	u8  serial;
	u8  isValid;
	u8  cpOpcode; //Control Point Opcode
	u8  resultCode;
	u8  mediaState;
	u8  trackIsStart;
	u8  reserve000;
	u16 playNameHandle;
	u16 trackChangedHandle;
	u16 trackTitleHandle;
	u16 trackDurationHandle;
	u16 trackPositionHandle;
	u16 mediaStateHandle;
	u16 mediaCtrlHandle;
	u16 opcodesSuppHandle;
	u16 CCIDHandle;

	u16 trackDurationValue;
	u32 trackPositionValue; //Should be in [0,trackDurationValue]
	
	#if (BLC_AUDIO_MCS_EXTEND_SUPPORT_ENABLE)
	u8  seekingSpeedValue;
	u8  searchContrlStatus;
	u16 seekingSpeedHandle;
	u16 searchControlHandle;
	u16 searchResultHandle;
	u8  searchObjectID[6];
	#endif
}blt_audio_mcpCtrl_t;
#endif //BLC_AUDIO_MCP_ENABLE


//#define CHARACTERISTIC_UUID_TBS_PROVIDER_NAME          //M Mandatory:Read, Notify;   Optional:
//#define CHARACTERISTIC_UUID_TBS_CALLER_IDENTIFIER      //M Mandatory:Read;   Optional:
//#define CHARACTERISTIC_UUID_TBS_TECHNOLOGY             //M Mandatory:Read, Notify;   Optional:
//#define CHARACTERISTIC_UUID_TBS_URI_LIST               //M Mandatory:Read;   Optional:Notify
//#define CHARACTERISTIC_UUID_TBS_CURRENT_CALL_LIST      //M Mandatory:Read, Notify;   Optional:
//#define CHARACTERISTIC_UUID_TBS_CCID                   //M Mandatory:Read;   Optional:
//#define CHARACTERISTIC_UUID_TBS_STATUS_FLAGS           //M Mandatory:Read, Notify;   Optional:
//#define CHARACTERISTIC_UUID_TBS_CALL_STATE             //M Mandatory:Read, Notify;   Optional:
//#define CHARACTERISTIC_UUID_TBS_CALL_CONTROL_POINT     //M Mandatory:Write, Write Without Response, Notify;   Optional:
//#define CHARACTERISTIC_UUID_TBS_CCP_OPTIONAL_OPCODES   //M Mandatory:Read;   Optional:
//#define CHARACTERISTIC_UUID_TBS_TERMINATIONO_REASON    //M Mandatory:Notify;   Optional:
//#define CHARACTERISTIC_UUID_TBS_INCOMING_CALL          //M Mandatory:Read, Notify;   Optional:

#if (BLC_AUDIO_TBS_ENABLE)
typedef struct{
	u8  flags;
	u8  index;
	u8  state;
	u8  callFlags;
	u8  reserve001;
	u8  termReason;
	
	u8  uriLen;
	u8  nameLen;
	u8  uri[BLC_AUDIO_TBS_URL_MAX_LEN];
	u8  name[BLC_AUDIO_TBS_NAME_MAX_LEN];
}blt_audio_tbsCall_t;
typedef struct{
    u16 flags;
	u8  cpOpcode; //Control Point Opcode
	u8  callIndex;
	u8  resultCode;
	u8  callCount;
	u16 reserve01;
	u16 CCIDHandle;
	u16 callCtrlHandle;
	u16 inCallHandle;
	u16 provNameHandle;
	u16 callerIDHandle;	
	u16 callStateHandle;
	u16 bearerTechHandle;
	u16 CCPOpcodesHandle;
	u16 termReasonHandle;
	u16 curCallListHandle;
	blt_audio_tbsCall_t call[BLC_AUDIO_TBS_CALL_COUNT];

	#if (BLC_AUDIO_TBS_EXTEND_SUPPORT_ENABLE)
	u8  signaleStrengthValue;
	u8  provNameLength;
	u8  bearerTechValue;
	u8  reserver0002;
	u16 statusFlagsValue;
	u16 statusFlagsHandle;
	
	u8  provName[BLC_AUDIO_TBS_NAME_MAX_LEN];
	u16 inCallURIHandle;
	u16 signaleStrengthHandle;
	u16 signaleStrengthReportHandle;
	u16 callFriendlyNameHandle;
	#endif
}blt_audio_tbpCtrl_t;
#endif  //BLC_AUDIO_TBS_ENABLE
#if (BLC_AUDIO_OTP_ENABLE)
//#define BLC_AUDIO_OTP_FEATURE_BUFFER_LEN   32
//#define BLC_AUDIO_OTP_NAME_BUFFER_LEN      16
//#define BLC_AUDIO_OTP_FILTER_BUFFER_LEN    32
#define BLC_AUDIO_OTP_FEATURE_BUFFER_LEN   64
#define BLC_AUDIO_OTP_NAME_BUFFER_LEN      64
#define BLC_AUDIO_OTP_FILTER_BUFFER_LEN    64
typedef struct{
	u16 flags;

	u8  oacpOpcode;
	u8  oacpStatus;
	
	u16 featureHandle;
	u16 objectNameHandle;
	u16 objectTypeHandle;
	u16 objectSizeHandle;
	u16 objectPropertiesHandle;
	u16 objectActionCPHandle;
	
	u8  featureLength;
	u8  objectNameLength;
	u16 objectTypeValue;
	u32 objectSizeValue0;
	u32 objectSizeValue1;
	u32 objectPropertiesValue;
	u8  featureValue[BLC_AUDIO_OTP_FEATURE_BUFFER_LEN];
	u8  objectNameValue[BLC_AUDIO_OTP_NAME_BUFFER_LEN];
	
	#if (BLC_AUDIO_OTS_EXTEND_SUPPORT_ENABLE)
	u16 firstCreatedHandle;
	u16 lastCreatedHandle;
	u16 objectIDHandle;
	u16 objectListCPHandle;
	u16 listFilterHandle;
	u16 objectChangedHandle;

	u8  listFilterLength;
	u8  reserve001;
	u8  olcpOpcode;
	u8  olcpStatus;
	
	u8  firstCreatedValue[8]; //Valid:7
	u8  lastCreatedValue[8]; //Valid:7
	u8  objectIDValue[8]; //Valid:6
	u8  listFilterValue[BLC_AUDIO_OTP_FILTER_BUFFER_LEN];
	#endif //BLC_AUDIO_OTS_EXTEND_SUPPORT_ENABLE
	
}blt_audio_otpCtrl_t;
#endif //BLC_AUDIO_OTP_ENABLE


typedef struct{
	u8  role; //Refer to BLC_AUDIO_ROLE_ENUM
	u8  conn; //True if used, false if not used
	u16 busy; //Refer to BLT_AUDIO_BUSY_ENUM
	u16 handle; //Connect Handle
	u16 gFlags; //Global flags (this is not change). Refer to BLT_AUDIO_GFLAGS_ENUM
	BlcAudioEventCB evtCB;
	#if (BLC_AUDIO_PACP_ENABLE)
	blt_audio_pacpCtrl_t pacp;
	#endif
	#if (BLC_AUDIO_ASCP_ENABLE)
	blt_audio_ascpCtrl_t ascp;
	#endif
	#if (BLC_AUDIO_CSIP_ENABLE)
	blt_audio_csipCtrl_t csip;
	#endif
	#if (BLC_AUDIO_MICP_ENABLE)
	blt_audio_micpCtrl_t micp;
	#endif
	#if (BLC_AUDIO_VCP_ENABLE)
	blt_audio_vcpCtrl_t vcp;
	#endif
	#if (BLC_AUDIO_MCP_ENABLE)
	blt_audio_mcpCtrl_t mcp;
	#endif 
	#if (BLC_AUDIO_TBS_ENABLE)
	blt_audio_tbpCtrl_t tbp;
	#endif
	#if (BLC_AUDIO_OTS_ENABLE)
	blt_audio_otpCtrl_t otp;
	#endif

	#if (BLC_AUDIO_SERVER_SDP_ENABLE)
	blt_audio_sdp_t sdp;
	#if !(BLC_AUDIO_MCP_ROLE_SWICH_ENABLE)
		#if (BLC_AUDIO_MCS_ENABLE)
		blt_audio_srv_t mcsSrv;
		#endif //BLC_AUDIO_MCS_ENABLE
	#endif //BLC_AUDIO_MCP_ROLE_SWICH_ENABLE
	#if !(BLC_AUDIO_TBP_ROLE_SWICH_ENABLE)
		#if (BLC_AUDIO_TBS_ENABLE)
		blt_audio_srv_t tbsSrv;
		#endif //BLC_AUDIO_TBS_ENABLE
	#endif //BLC_AUDIO_TBP_ROLE_SWICH_ENABLE
	#if !(BLC_AUDIO_OTP_ROLE_SWICH_ENABLE)
		#if (BLC_AUDIO_OTS_ENABLE)
		blt_audio_srv_t otsSrv;
		#endif //BLC_AUDIO_OTS_ENABLE
	#endif //BLC_AUDIO_OTP_ROLE_SWICH_ENABLE
	#endif
}blt_audio_server_t;

typedef struct{
	u8  role; //Refer to BLC_AUDIO_ROLE_ENUM
	u8  conn; //True if used, false if not used
	u16 busy; //Refer to BLT_AUDIO_BUSY_ENUM
	u16 handle; //Connect Handle
	u16 gFlags; //Global flags. Refer to BLT_AUDIO_GFLAGS_ENUM
	BlcAudioEventCB evtCB;
	#if (BLC_AUDIO_PACP_ENABLE)
	blt_audio_pacpCtrl_t pacp;
	#endif
	#if (BLC_AUDIO_ASCP_ENABLE)
	blt_audio_ascpCtrl_t ascp;
	#endif
	#if (BLC_AUDIO_CSIP_ENABLE)
	blt_audio_csipCtrl_t csip;
	#endif
	#if (BLC_AUDIO_MICP_ENABLE)
	blt_audio_micpCtrl_t micp;
	#endif
	#if (BLC_AUDIO_VCP_ENABLE)
	blt_audio_vcpCtrl_t vcp;
	#endif
	#if (BLC_AUDIO_MCP_ENABLE)
	blt_audio_mcpCtrl_t mcp;
	#endif 
	#if (BLC_AUDIO_TBS_ENABLE)
	blt_audio_tbpCtrl_t tbp;
	#endif
	#if (BLC_AUDIO_OTS_ENABLE)
	blt_audio_otpCtrl_t otp;
	#endif
	
	#if (BLC_AUDIO_CLIENT_SDP_ENABLE)
	blt_audio_sdp_t sdp;
	#if (BLC_AUDIO_PACS_ENABLE)
	blt_audio_srv_t pacsSrv;
	#endif //BLC_AUDIO_PACS_ENABLE
	#if (BLC_AUDIO_ASCS_ENABLE)
	blt_audio_srv_t ascsSrv;
	#endif //BLC_AUDIO_ASCS_ENABLE
	#if (BLC_AUDIO_CSIS_ENABLE)
	blt_audio_srv_t csisSrv;
	#endif //BLC_AUDIO_CSIS_ENABLE
	#if (BLC_AUDIO_MICS_ENABLE)
	blt_audio_srv_t micsSrv;
	#endif //BLC_AUDIO_CSIS_ENABLE
	#if (BLC_AUDIO_VCS_ENABLE)
	blt_audio_srv_t vcsSrv;
	#endif //BLC_AUDIO_VCS_ENABLE
	#if (BLC_AUDIO_MCP_ROLE_SWICH_ENABLE)
		#if (BLC_AUDIO_MCS_ENABLE)
		blt_audio_srv_t mcsSrv;
		#endif //BLC_AUDIO_MCS_ENABLE
	#endif
	#if (BLC_AUDIO_TBP_ROLE_SWICH_ENABLE)
		#if (BLC_AUDIO_TBS_ENABLE)
		blt_audio_srv_t tbsSrv;
		#endif //BLC_AUDIO_TBS_ENABLE
	#endif
	#if (BLC_AUDIO_OTP_ROLE_SWICH_ENABLE)
		#if (BLC_AUDIO_OTS_ENABLE)
		blt_audio_srv_t otsSrv;
		#endif //BLC_AUDIO_OTS_ENABLE
	#endif
	#endif
}blt_audio_client_t;

typedef struct{
	u8  role; //Refer to BLC_AUDIO_ROLE_ENUM
	u8  conn; //True if used, false if not used
	u16 busy; //Refer to BLT_AUDIO_BUSY_ENUM
	u16 handle; //Connect Handle
	u16 gFlags; //Global flags. Refer to BLT_AUDIO_GFLAGS_ENUM
	BlcAudioEventCB evtCB;
	#if (BLC_AUDIO_PACP_ENABLE)
	blt_audio_pacpCtrl_t pacp;
	#endif
	#if (BLC_AUDIO_ASCP_ENABLE)
	blt_audio_ascpCtrl_t ascp;
	#endif
	#if (BLC_AUDIO_CSIP_ENABLE)
	blt_audio_csipCtrl_t csip;
	#endif
	#if (BLC_AUDIO_MICP_ENABLE)
	blt_audio_micpCtrl_t micp;
	#endif
	#if (BLC_AUDIO_VCP_ENABLE)
	blt_audio_vcpCtrl_t vcp;
	#endif
	#if (BLC_AUDIO_MCP_ENABLE)
	blt_audio_mcpCtrl_t mcp;
	#endif 
	#if (BLC_AUDIO_TBS_ENABLE)
	blt_audio_tbpCtrl_t tbp;
	#endif
	#if (BLC_AUDIO_OTS_ENABLE)
	blt_audio_otpCtrl_t otp;
	#endif
}blt_audio_handle_t;



typedef struct{
	int  (*Init)(blt_audio_handle_t *pHandle);
	int  (*GattIn)(blt_audio_handle_t *pHandle, u8 *pPkt);
	int  (*SetServ)(blt_audio_handle_t *pHandle, attribute_t *pService);
	void (*SetConn)(blt_audio_handle_t *pHandle, BLC_AUDIO_STATE_ENUM state);
	void (*Process)(blt_audio_handle_t *pHandle);
}blt_audio_func_t;



#endif //BLC_AUDIO_PROFILE_EN

#endif //_AUDIO_INNER_H_
