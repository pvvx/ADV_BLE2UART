/********************************************************************************************************
 * @file	audio_tbs.h
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
#ifndef _AUDIO_TBS_H_
#define _AUDIO_TBS_H_

#if (BLC_AUDIO_TBS_ENABLE)


#define BLT_AUDIO_TBP_CALL_INVALID_STATE   0xFF


typedef enum{
	BLT_AUDIO_TBP_FLAG_NONE       = 0x00,
	BLT_AUDIO_TBP_FLAG_NOTY_STATE = 0x01,
	BLT_AUDIO_TBP_FLAG_NOTY_CALL  = 0x02,
	BLT_AUDIO_TBP_FLAG_NOTY_CPRST = 0x04,
	BLT_AUDIO_TBP_FLAG_NOTY_SIGNAL = 0x08,
	BLT_AUDIO_TBP_FLAG_NOTY_PRNAME = 0x10, //Provide Name
	BLT_AUDIO_TBP_FLAG_NOTY_BRTECH = 0x20, //Bearer Technology
	BLT_AUDIO_TBP_FLAG_NOTY_STATUS = 0x40, //Status Flags
}BLT_AUDIO_TBP_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_TBP_CALL_FLAG_NONE             = 0x00,
	BLT_AUDIO_TBP_CALL_FLAG_NOTY_STATE       = 0x01,
	BLT_AUDIO_TBP_CALL_FLAG_NOTY_ALL_STATE   = 0x02,
	BLT_AUDIO_TBP_CALL_FLAG_NOTY_INCALL      = 0x04,
	BLT_AUDIO_TBP_CALL_FLAG_NOTY_NAME        = 0x08,
	BLT_AUDIO_TBP_CALL_FLAG_NOTY_INCALL_URI  = 0x10,
	BLT_AUDIO_TBP_CALL_FLAG_NOTY_TERMINATION = 0x20,
}BLT_AUDIO_TBP_CALL_FLAGS_ENUM;


typedef enum{
	BLT_AUDIO_TBP_TERM_REASON_URI_ERROR          = 0x00,
	BLT_AUDIO_TBP_TERM_REASON_CALL_FAILED        = 0x01,
	BLT_AUDIO_TBP_TERM_REASON_REMOTE_ENDED_CALL  = 0x02,
	BLT_AUDIO_TBP_TERM_REASON_SERVER_ENDED_CALL  = 0x03,
	BLT_AUDIO_TBP_TERM_REASON_LINE_BUSY          = 0x04,
	BLT_AUDIO_TBP_TERM_REASON_NETWORK_CONGESTION = 0x05,
	BLT_AUDIO_TBP_TERM_REASON_CLIENT_TERM_CALL   = 0x06,
	BLT_AUDIO_TBP_TERM_REASON_NO_SERVICE         = 0x07,
	BLT_AUDIO_TBP_TERM_REASON_NO_ANSWER          = 0x08,
	BLT_AUDIO_TBP_TERM_REASON_UNSPECIFIED        = 0x09,
}BLT_AUDIO_TBP_TERM_REASON_ENUM;

typedef enum{
	BLT_AUDIO_TBP_CP_RSTCODE_SUCCESS                = 0x00, //The opcode write was successful.
	BLT_AUDIO_TBP_CP_RSTCODE_OPCODE_NOT_SUPPORTED   = 0x01, //An invalid opcode was used for the Call Control Point write.
	BLT_AUDIO_TBP_CP_RSTCODE_OPERATION_NOT_POSSIBLE = 0x02, //The requested operation cannot be completed.
	BLT_AUDIO_TBP_CP_RSTCODE_INVALID_CALL_INDEX     = 0x03, //The Call Index used for the Call Control Point write is invalid.
	BLT_AUDIO_TBP_CP_RSTCODE_STATE_MISMATCH         = 0x04, //The opcode written to the Call Control Point was received when the current Call State for the Call Index was not in the expected state.
	BLT_AUDIO_TBP_CP_RSTCODE_LACK_OF_RESOURCES      = 0x05, //Lack of internal resources to complete the requested action.
	BLT_AUDIO_TBP_CP_RSTCODE_INVALID_OUTGOING_URI   = 0x06, //The Outgoing URI is incorrect or invalid when an Originate opcode is sent.
}BLT_AUDIO_TBP_CP_RSTCODE_ENUM;


int blc_audio_tbpAttRead(u16 connHandle, void* p);
int blc_audio_tbpAttWrite(u16 connHandle, void* p);

int blc_audio_tbpEnable(u16 connHandle);
int blc_audio_tbpDisable(u16 connHandle);

int blc_audio_tbpSendCtrl(u16 connHandle, u8 opcode, u8 *pData, u8 dataLen);

int blc_audio_tbpAccept(u16 connHandle, u8 callIndex);
int blc_audio_tbpTerminal(u16 connHandle, u8 callIndex);
int blc_audio_tbpAlerting(u16 connHandle, u8 callIndex);
int blc_audio_tbpCallHold(u16 connHandle, u8 callIndex);
int blc_audio_tbpCallRetrieve(u16 connHandle, u8 callIndex);
int blc_audio_tbpAddIncomingCall(u16 connHandle, u8 *pURI, u8 URILen, u8 *pName, u8 nameLen);

int blc_audio_tbpSetSignalStrength(u16 connHandle, u8 value);
int blc_audio_tbpSetProvideName(u16 connHandle, u8 *pName, u8 nameLen);
int blc_audio_tbpSetBearerTechnology(u16 connHandle, u8 value);
int blc_audio_tbpSetStatusFlags(u16 connHandle, u16 value);


#endif //#if (BLC_AUDIO_TBS_ENABLE)

#endif

