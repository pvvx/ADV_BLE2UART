/********************************************************************************************************
 * @file	audio_vcp.h
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
#ifndef _AUDIO_VCP_H_
#define _AUDIO_VCP_H_

#if (BLC_AUDIO_VCP_ENABLE)



typedef enum{
	BLT_VCP_OPCODE_RELATIVE_VOLUME_DOWN         = 0x00,
	BLT_VCP_OPCODE_RELATIVE_VOLUME_UP           = 0x01,
	BLT_VCP_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN  = 0x02,
	BLT_VCP_OPCODE_UNMUTE_RELATIVE_VOLUME_UP    = 0x03,
	BLT_VCP_OPCODE_SET_ABSOLUTE_VOLUME          = 0x04,
	BLT_VCP_OPCODE_UNMUTE                       = 0x05,
	BLT_VCP_OPCODE_MUTE                         = 0x06,
}BLT_VCP_OPCODE_ENUM;
typedef enum{
	BLT_VCP_ERRCODE_INVALID_CHANGE_COUNTER       = 0x80,
	BLT_VCP_ERRCODE_OPCODE_NOT_SUPPORTED         = 0x81,
}BLT_VCP_ERRCODE_ENUM;

typedef enum{
	BLT_VCP_AICS_OPCODE_SET_GAIN_VALUE  = 0x01,
	BLT_VCP_AICS_OPCODE_UNMUTE          = 0x02,
	BLT_VCP_AICS_OPCODE_MUTE            = 0x03,
	BLT_VCP_AICS_OPCODE_MANU_GAIN_MODE  = 0x04,
	BLT_VCP_AICS_OPCODE_AUTO_GAIN_MODE  = 0x05,
}BLT_VCP_AICS_OPCODE_ENUM;
typedef enum{
	BLT_VCP_AICS_ERRCODE_INVALID_CHANGE_COUNTER  = 0x80,
	BLT_VCP_AICS_ERRCODE_OPCODE_NOT_SUPPORTED    = 0x81,
	BLT_VCP_AICS_ERRCODE_MUTE_DISABLED           = 0x82,
	BLT_VCP_AICS_ERRCODE_VALUE_OUT_OF_RANGE      = 0x83,
	BLT_VCP_AICS_ERRCODE_MODE_CHANGE_NOT_ALLOWED = 0x84,
}BLT_VCP_AICS_ERRCODE_ENUM;

typedef enum{
	BLT_VCP_VOCS_OPCODE_SET_VOLUME_OFFSET  = 0x01,
}BLT_VCP_VOCS_OPCODE_ENUM;
typedef enum{
	BLT_VCP_VOCS_ERRCODE_INVALID_CHANGE_COUNTER  = 0x80,
	BLT_VCP_VOCS_ERRCODE_OPCODE_NOT_SUPPORTED	 = 0x81,
	BLT_VCP_VOCS_ERRCODE_VALUE_OUT_OF_RANGE 	 = 0x82,
}BLT_VCP_VOCS_ERRCODE_ENUM;


typedef enum{
	BLT_AUDIO_VCP_SDP_FLAG_NONE  = 0x00,
	BLT_AUDIO_VCP_SDP_FLAG_FIRST = 0x80,
	BLT_AUDIO_VCP_SDP_FLAG_INIT  = 0x40,
	BLT_AUDIO_VCP_SDP_FLAG_AICS  = 0x20,
	BLT_AUDIO_VCP_SDP_FLAG_VOCS  = 0x10,
	BLT_AUDIO_VCP_SDP_FLAG_READY = 0x08,
	
	BLT_AUDIO_VCP_SDP_FLAG_PVAL  = 0x04,
	BLT_AUDIO_VCP_SDP_FLAG_SVAL  = 0x02,
}BLT_AUDIO_VCP_SDP_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_VCP_FLAG_NONE         = 0x00,
	BLT_AUDIO_VCP_FLAG_STATE_CHANGE = 0x01,
	BLT_AUDIO_VCP_FLAG_FLAGS_CHANGE = 0x02,
	BLT_AUDIO_VCP_FLAG_AICS_CHANGE  = 0x04,
	BLT_AUDIO_VCP_FLAG_VOCS_CHANGE  = 0x08,
}BLT_AUDIO_VCP_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_VCP_AICS_FLAG_NONE          = 0x00,
	BLT_AUDIO_VCP_AICS_FLAG_STATE_CHANGE  = 0x01,
	BLT_AUDIO_VCP_AICS_FLAG_STATUS_CHANGE = 0x02,
	BLT_AUDIO_VCP_AICS_FLAG_DESC_CHANGE   = 0x04,
}BLT_AUDIO_VCP_AICS_FLAGS_ENUM;
typedef enum{
	BLT_AUDIO_VCP_VOCS_FLAG_NONE		  = 0x00,
	BLT_AUDIO_VCP_VOCS_FLAG_STATE_CHANGE  = 0x01,
	BLT_AUDIO_VCP_VOCS_FLAG_LOCA_CHANGE   = 0x02,
	BLT_AUDIO_VCP_VOCS_FLAG_DESC_CHANGE   = 0x04,
}BLT_AUDIO_VCP_VOCS_FLAGS_ENUM;


int blc_audio_vcpAttRead(u16 connHandle, void* p);
int blc_audio_vcpAttWrite(u16 connHandle, void* p);
int blc_audio_vcpSetVolume(u16 connHandle, u8 volume);
int blc_audio_vcpSetMute(u16 connHandle, u8 mute);
int blc_audio_vcpSetCounter(u16 connHandle, u8 counter);
#if (BLC_AUDIO_VOCS_ENABLE)
int blc_audio_vcpVocsSetCounter(u16 connHandle, u8 counter, int num);
int blc_audio_vcpVocsSetAllCounter(u16 connHandle, u8 counter);
int blc_audio_vcpVocsSetLocation(u16 connHandle, u32 location, int num);
int blc_audio_vcpVocsSetDescrible(u16 connHandle, u8 *pDesc, u8 length, int num);
#if (BLC_AUDIO_VCS_AICS_ENABLE)
int blc_audio_vcpAicsSetCounter(u16 connHandle, u8 counter, int num);
int blc_audio_vcpAicsSetAllCounter(u16 connHandle, u8 counter);
int blc_audio_vcpAicsSetMute(u16 connHandle, u8 mute, int num);
int blc_audio_vcpAicsSetAllMute(u16 connHandle, u8 mute);
int blc_audio_vcpAicsSetGainMode(u16 connHandle, u8 gainMode, int num);
int blc_audio_vcpAicsSetAllGainMode(u16 connHandle, u8 gainMode);
int blc_audio_vcpAicsSetDescrible(u16 connHandle, u8 *pDesc, u8 length, int num);
#endif
#endif



static int  blt_audio_vcpInit(blt_audio_handle_t *pHandle);
static int  blt_audio_vcpGattIn(blt_audio_handle_t *pHandle, u8 *pPkt);
static int  blt_audio_vcpSetServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static void blt_audio_vcpSetConn(blt_audio_handle_t *pHandle, BLC_AUDIO_STATE_ENUM state);
static void blt_audio_vcpProcess(blt_audio_handle_t *pHandle);

static int blt_audio_vcpSetVcsServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static int blt_audio_vcpWriteVcs(blt_audio_handle_t *pHandle, u8 opcode, u8 dataLen, u8 *pData);
#if (BLC_AUDIO_VCS_AICS_ENABLE)
static int blt_audio_vcpSetAicsServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static int blt_audio_vcpWriteAics(blt_audio_handle_t *pHandle, blt_audio_aics_t *pAics, u8 opcode, u8 dataLen, u8 *pData);
#endif
#if (BLC_AUDIO_VOCS_ENABLE)
static int blt_audio_vcpSetVocsServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static int blt_audio_vcpWriteVocs(blt_audio_handle_t *pHandle, blt_audio_vocs_t *pVocs, u8 opcode, u8 dataLen, u8 *pData);
#endif

#endif //#if (BLC_AUDIO_VCP_ENABLE)

#endif
