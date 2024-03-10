/********************************************************************************************************
 * @file	audio_micp.h
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
#ifndef _AUDIO_MICP_H_
#define _AUDIO_MICP_H_

#if (BLC_AUDIO_MICP_ENABLE)




typedef enum{
	BLT_AUDIO_MICP_OPCODE_SET_GAIN       = 0x01,
	BLT_AUDIO_MICP_OPCODE_UNMUTE         = 0x02,
	BLT_AUDIO_MICP_OPCODE_MUTE           = 0x03,
	BLT_AUDIO_MICP_OPCODE_MANUAL_MODE    = 0x04,
	BLT_AUDIO_MICP_OPCODE_AUTOMATIC_MODE = 0x05,
}BLT_AUDIO_MICP_OPCODE_ENUM;

typedef enum{
	BLT_AUDIO_MICP_ERRCODE_MUTE_DISABLED      = 0x80,
	BLT_AUDIO_MICP_ERRCODE_VALUE_OUT_OF_RANGE = 0x81,
}BLT_AUDIO_MICP_ERRCODE_ENUM;

typedef enum{
	BLT_MICP_AICS_OPCODE_SET_GAIN_VALUE  = 0x01,
	BLT_MICP_AICS_OPCODE_UNMUTE          = 0x02,
	BLT_MICP_AICS_OPCODE_MUTE            = 0x03,
	BLT_MICP_AICS_OPCODE_MANU_GAIN_MODE  = 0x04,
	BLT_MICP_AICS_OPCODE_AUTO_GAIN_MODE  = 0x05,
}BLT_MICP_AICS_OPCODE_ENUM;
typedef enum{
	BLT_MICP_AICS_ERRCODE_INVALID_CHANGE_COUNTER  = 0x80,
	BLT_MICP_AICS_ERRCODE_OPCODE_NOT_SUPPORTED    = 0x81,
	BLT_MICP_AICS_ERRCODE_MUTE_DISABLED           = 0x82,
	BLT_MICP_AICS_ERRCODE_VALUE_OUT_OF_RANGE      = 0x83,
	BLT_MICP_AICS_ERRCODE_MODE_CHANGE_NOT_ALLOWED = 0x84,
}BLT_MICP_AICS_ERRCODE_ENUM;


typedef enum{
	BLT_AUDIO_MICP_SDP_FLAG_NONE  = 0x00,
	BLT_AUDIO_MICP_SDP_FLAG_FIRST = 0x80,
	BLT_AUDIO_MICP_SDP_FLAG_INIT  = 0x40,
	BLT_AUDIO_MICP_SDP_FLAG_AICS  = 0x20,
	BLT_AUDIO_MICP_SDP_FLAG_VOCS  = 0x10,
	BLT_AUDIO_MICP_SDP_FLAG_READY = 0x08,
	
	BLT_AUDIO_MICP_SDP_FLAG_PVAL  = 0x04,
	BLT_AUDIO_MICP_SDP_FLAG_SVAL  = 0x02,
}BLT_AUDIO_MICP_SDP_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_MICP_FLAG_NONE         = 0x00,
	BLT_AUDIO_MICP_FLAG_MUTE_CHANGE  = 0x01,
	BLT_AUDIO_MICP_FLAG_AICS_CHANGE  = 0x02,
}BLT_AUDIO_MICP_FLAGS_ENUM;
typedef enum{
	BLT_AUDIO_MICP_AICS_FLAG_NONE          = 0x00,
	BLT_AUDIO_MICP_AICS_FLAG_STATE_CHANGE  = 0x01,
	BLT_AUDIO_MICP_AICS_FLAG_STATUS_CHANGE = 0x02,
}BLT_AUDIO_MICP_AICS_FLAGS_ENUM;



int blc_audio_micpAttRead(u16 connHandle, void *p);
int blc_audio_micpAttWrite(u16 connHandle, void *p);

int blc_audio_micpSetMute(u16 connHandle, u8 mute);


static int  blt_audio_micpInit(blt_audio_handle_t *pHandle);
static int  blt_audio_micpGattIn(blt_audio_handle_t *pHandle, u8 *pPkt);
static int  blt_audio_micpSetServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static void blt_audio_micpSetConn(blt_audio_handle_t *pHandle, BLC_AUDIO_STATE_ENUM state);
static void blt_audio_micpProcess(blt_audio_handle_t *pHandle);

static int blt_audio_micpSetMicsServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static int blt_audio_micpWriteMics(blt_audio_handle_t *pHandle, u8 dataLen, u8 *pData);
#if (BLC_AUDIO_MICS_AICS_ENABLE)
static int blt_audio_micpSetAicsServ(blt_audio_handle_t *pHandle, attribute_t *pService);
static int blt_audio_micpWriteAics(blt_audio_handle_t *pHandle, blt_audio_aics_t *pAics, u8 opcode, u8 dataLen, u8 *pData);
#endif




#endif //#if (BLC_AUDIO_MICP_ENABLE)

#endif //_AUDIO_MICP_H_

