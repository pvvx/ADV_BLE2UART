/********************************************************************************************************
 * @file	audio_mcp.h
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
#ifndef _AUDIO_MCP_H_
#define _AUDIO_MCP_H_

#if (BLC_AUDIO_MCP_ENABLE)



#define BLT_AUDIO_MCP_TRACK_STEP                    100
#define BLT_AUDIO_MCP_TRACKS_IN_SINGLE_SEGMENT      5
#define BLT_AUDIO_MCP_SEGMENT_MAX                   (0xFFFFFFFF/BLT_AUDIO_MCP_TRACKS_IN_SINGLE_SEGMENT)

#define BLT_AUDIO_MCP_TRACKS_IN_SINGLE_GROUP        10
#define BLT_AUDIO_MCP_GROUP_MAX                     (0xFFFFFFFF/BLT_AUDIO_MCP_TRACKS_IN_SINGLE_GROUP)


typedef enum{
	BLT_AUDIO_MCP_FLAG_NONE         = 0x0000,
	BLT_AUDIO_MCP_FLAG_NOTY_STATE   = 0x0001,
	BLT_AUDIO_MCP_FLAG_NOTY_CPRST   = 0x0002,
	BLT_AUDIO_MCP_FLAG_NOTY_TRACK_TITLE      = 0x0010,
	BLT_AUDIO_MCP_FLAG_NOTY_TRACK_CHANGED    = 0x0020,
	BLT_AUDIO_MCP_FLAG_NOTY_TRACK_DURATION   = 0x0040,
	BLT_AUDIO_MCP_FLAG_NOTY_TRACK_POSITION   = 0x0080,
	BLT_AUDIO_MCP_FLAG_NOTY_SEEKING_SPEED    = 0x0100,
	BLT_AUDIO_MCP_FLAG_NOTY_SEARCH_CONTROL   = 0x0200,
	BLT_AUDIO_MCP_FLAG_NOTY_SEARCH_OBJECT    = 0x0400,
}BLT_AUDIO_MCP_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_MCP_CP_ERRCODE_VALUE_CHANGED = 0x80,
}BLT_AUDIO_MCP_CP_ERRCODE_ENUM;

typedef enum{
	BLT_AUDIO_MCP_CP_RSTCODE_SUCCESS               = 0x01,
	BLT_AUDIO_MCP_CP_RSTCODE_OPCODE_NOT_SUPPORTED  = 0x02,
	BLT_AUDIO_MCP_CP_RSTCODE_MEDIA_PLAYER_INACTIVE = 0x03,
}BLT_AUDIO_MCP_CP_RSTCODE_ENUM;



int blc_audio_mcpAttRead(u16 connHandle, void* p);
int blc_audio_mcpAttWrite(u16 connHandle, void* p);

int blc_audio_mcpEnable(u16 connHandle);
int blc_audio_mcpDisable(u16 connHandle);

int blc_audio_mcpSetMediaState(u16 connHandle, u8 state, bool isNoty);
int blt_audio_mcpSetTrackPosition(u16 connHandle, u32 value, bool isNoty);

int blc_audioMcpPlay(u16 connHandle);
int blc_audioMcpPause(u16 connHandle);
int blc_audioMcpFastRewind(u16 connHandle);
int blc_audioMcpFastForward(u16 connHandle);
int blc_audioMcpStop(u16 connHandle);
int blc_audioMcpMoveRelative(u16 connHandle, s32 offset);
int blc_audioMcpPreviousSegment(u16 connHandle);
int blc_audioMcpNextSegment(u16 connHandle);
int blc_audioMcpFirstSegment(u16 connHandle);
int blc_audioMcpLastSegment(u16 connHandle);
int blc_audioMcpGotoSegment(u16 connHandle, s32 number);
int blc_audioMcpPreviousTrack(u16 connHandle);
int blc_audioMcpNextTrack(u16 connHandle);
int blc_audioMcpFirstTrack(u16 connHandle);
int blc_audioMcpLastTrack(u16 connHandle);
int blc_audioMcpGotoTrack(u16 connHandle, s32 number);
int blc_audioMcpPreviousGroup(u16 connHandle);
int blc_audioMcpNextGroup(u16 connHandle);
int blc_audioMcpFirstGroup(u16 connHandle);
int blc_audioMcpLastGroup(u16 connHandle);
int blc_audioMcpGotoGroup(u16 connHandle, s32 number);
int blc_audio_mcpSendCtrl(u16 connHandle, u8 opcode, u8 *pData, u16 dataLen);



#endif //#if (BLC_AUDIO_MCP_ENABLE)

#endif //_AUDIO_MCP_H_

