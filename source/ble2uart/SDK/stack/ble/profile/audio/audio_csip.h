/********************************************************************************************************
 * @file	audio_csip.h
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
#ifndef _AUDIO_CSIP_H_
#define _AUDIO_CSIP_H_

#if (BLC_AUDIO_CSIP_ENABLE)

#define BLC_AUDIO_CSIP_LOCK_TIMEOUT_DEAFULT     60000000 // 60s


#define BLC_AUDIO_CSIP_LOCK      0x02
#define BLC_AUDIO_CSIP_UNLOCK    0x01


typedef enum{
	BLT_AUDIO_CSIP_FLAG_NONE        = 0x00,
	BLT_AUDIO_CSIP_FLAG_LOCK_TIMER  = 0x02,

	BLT_AUDIO_CSIP_FLAG_NOTY_SIZE   = 0x04,
	BLT_AUDIO_CSIP_FLAG_NOTY_LOCK   = 0x08,
}BLT_AUDIO_CSIP_FLAGS_ENUM;

typedef enum{
	BLT_AUDIO_CSIP_ERRCODE_LOCK_DENIED          = 0x80, //The lock cannot be granted because the server is already locked.
	BLT_AUDIO_CSIP_ERRCODE_RELEASE_NOT_ACLLOWED = 0x81,
	BLT_AUDIO_CSIP_ERRCODE_INVALID_LOCK_VALUE   = 0x82,
	BLT_AUDIO_CSIP_ERRCODE_OOB_SIRK_ONLY        = 0x83,
	BLT_AUDIO_CSIP_ERRCODE_LOCK_ALREADY_GRANTED = 0x84, //The client that made the request is the current owner of thelock.
}BLT_AUDIO_CSIP_ERRCODE_ENUM;


//readyEvt.size = pCsip->size;
//	readyEvt.lock = pCsip->lock;
//	readyEvt.rank = pCsip->rank;

typedef enum{
	BLT_AUDIO_CSIP_SDP_FLAG_NONE  = 0x00,
	BLT_AUDIO_CSIP_SDP_FLAG_FIRST = 0x80,
	BLT_AUDIO_CSIP_SDP_FLAG_SIRK  = 0x01,
	BLT_AUDIO_CSIP_SDP_FLAG_SIZE  = 0x02,
	BLT_AUDIO_CSIP_SDP_FLAG_RANK  = 0x04,
	BLT_AUDIO_CSIP_SDP_FLAG_LOCK  = 0x08,
}BLT_AUDIO_CSIP_SDP_FLAGS_ENUM;


int blc_audio_csipAttRead(u16 connHandle, void* p);
int blc_audio_csipAttWrite(u16 connHandle, void* p);

int blc_audio_csipSetLock(u16 connHandle, bool isLock);

void blc_audio_csisSih(u8 sirk[16], u8 prand[3], u8 hash[3]);
void blc_audio_csisGeneratePsri(u8 sirk[16], u8 prand[3], u8 psri[6]);
bool blc_audio_csisPsriIsValid(u8 sirk[16], u8 psri[6]);



#endif //#if (BLC_AUDIO_CSIP_ENABLE)

#endif //_AUDIO_CSIP_H_

