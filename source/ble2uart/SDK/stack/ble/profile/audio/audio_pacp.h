/********************************************************************************************************
 * @file	audio_pacp.h
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
#ifndef _AUDIO_PACP_H_
#define _AUDIO_PACP_H_

#if (BLC_AUDIO_PACS_ENABLE)


typedef enum{
	BLT_AUDIO_PACP_FLAG_NONE           = 0x0000,
	BLT_AUDIO_PACP_FLAG_CONTEXT_CHANGE = 0x0001,
}BLT_AUDIO_PACP_FLAGS_ENUM;


typedef enum{
	BLT_AUDIO_PACP_RECORD_TYPE_SINK    = 0x00,
	BLT_AUDIO_PACP_RECORD_TYPE_SOURCE  = 0x01,
}BLT_PACP_RECORD_TYPE_ENUM;


typedef struct{
	u8 counts;
	u8 duration;
	u16 frequency;
	u16 minOctets;
	u16 maxOctets;
	u16 preferredContext;
}blt_audio_pacpRecordParam_t;

int blc_audio_pacpAttRead(u16 connHandle, void* p);
int blc_audio_pacpAttWrite(u16 connHandle, void* p);


u8 blt_audio_mallocContext(blt_audio_handle_t *pHandle, u8 type, u16 context);
u8 blt_audio_freeContext(blt_audio_handle_t *pHandle, u8 type, u16 context);

u16 blt_audio_getAvalibleContext(blt_audio_handle_t *pHandle, u8 type);
void blt_audio_setAvalibleContext(blt_audio_handle_t *pHandle, u8 type, u16 context);

u8 *blt_audio_getRecord(blt_audio_handle_t *pHandle, u8 type, u8 *pCodecId);
u8 blt_audio_getRecordParam(blt_audio_handle_t *pHandle, u8 type, u8 *pCodecId, blt_audio_pacpRecordParam_t *pParam);


#endif //#if (BLC_AUDIO_PACS_ENABLE)

#endif //_AUDIO_PACP_H_

