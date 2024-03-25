/********************************************************************************************************
 * @file	app_audio.c
 *
 * @brief	This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "../common/tl_audio.h"

#if (UI_AUDIO_ENABLE)

#define DEC_BUFFER_SIZE		(MIC_SHORT_DEC_SIZE * MIC_SHORT_DEC_SIZE_NUM)
#define	USB_ISO_IN_SIZE		(MIC_SAMPLE_RATE / 1000)

u8		att_mic_rcvd = 0;
u32		tick_adpcm;

int		abuf_reset = 0;
u8		abuf_mic_wptr, abuf_dec_wptr;
u16		abuf_dec_rptr;

u8		buff_mic_adpcm[MIC_ADPCM_FRAME_SIZE];
u8		abuf_mic[MIC_ADPCM_FRAME_SIZE * MIC_ADPCM_FRAME_SIZE_NUM];
s16		abuf_dec[DEC_BUFFER_SIZE];



void abuf_init ()
{
	abuf_mic_wptr = abuf_dec_wptr = 0;
	abuf_reset = 16;
}

void abuf_mic_add (u32 *p)
{
	u32 *pd = (u32 *) (abuf_mic + (abuf_mic_wptr & (MIC_ADPCM_FRAME_SIZE_NUM - 1)) * MIC_ADPCM_FRAME_SIZE);
	for (int i=0; i<(MIC_ADPCM_FRAME_SIZE>>2); i++)
	{
		*pd ++ = *p++;
	}
	abuf_mic_wptr ++;
}

void abuf_mic_dec ()
{
	static int start = 1;
	if (abuf_reset)
	{
		abuf_dec_wptr = abuf_mic_wptr;
	}
	else
	{
		u16 num_mic = abuf_mic_wptr - abuf_dec_wptr;

		if (num_mic > MIC_ADPCM_FRAME_SIZE_NUM) 		// in case of overflow
		{
			abuf_dec_wptr ++;
		}

		else if ((!start && num_mic>=1) || (start && num_mic>=2))
		{
			adpcm_to_pcm (
					(s16 *) (abuf_mic + (abuf_dec_wptr & (MIC_ADPCM_FRAME_SIZE_NUM - 1)) * MIC_ADPCM_FRAME_SIZE),
					abuf_dec + (abuf_dec_wptr & (MIC_SHORT_DEC_SIZE_NUM - 1)) * MIC_SHORT_DEC_SIZE,
					MIC_SHORT_DEC_SIZE );

			abuf_dec_wptr ++;			// 256-byte = 128-s16
			start = 0;
		}
	}
}
_attribute_ram_code_ void  abuf_dec_usb ()
{
	static u32 tick_usb_iso_in;
	static u8  buffer_empty = 1;
	static u8  n_usb_iso = 0;

	n_usb_iso++;

	if (clock_time_exceed (tick_usb_iso_in, 4000))
	{
		abuf_reset = 16;
	}

	tick_usb_iso_in = clock_time ();
	if (abuf_reset)
	{
		abuf_dec_rptr = abuf_dec_wptr*MIC_SHORT_DEC_SIZE;
		abuf_reset--;
	}

	/////////////////// copy data to usb iso in buffer ///////////////
	reg_usb_ep7_ptr = 0;
	u16 num = abuf_dec_wptr - (abuf_dec_rptr/MIC_SHORT_DEC_SIZE);

	if (num)
	{
		if ( (buffer_empty && num >= (MIC_SHORT_DEC_SIZE_NUM - 1)) || (!buffer_empty && (num >= 1 || (n_usb_iso & (MIC_SHORT_DEC_SIZE_NUM - 1)))) )
		{
			buffer_empty = 0;

			u16 offset = abuf_dec_rptr%DEC_BUFFER_SIZE;
			s16 *ps = abuf_dec + offset;


			if(offset == DEC_BUFFER_SIZE - (USB_ISO_IN_SIZE/2)){
				for (int i=0; i<(USB_ISO_IN_SIZE/2); i++)
				{
					reg_usb_ep7_dat = *ps;
					reg_usb_ep7_dat = *ps++ >> 8;
				}
				ps = abuf_dec;
				for (int i=0; i<(USB_ISO_IN_SIZE/2); i++)
				{
					reg_usb_ep7_dat = *ps;
					reg_usb_ep7_dat = *ps++ >> 8;
				}
			}
			else{
				for (int i=0; i<USB_ISO_IN_SIZE; i++)
				{
					reg_usb_ep7_dat = *ps;
					reg_usb_ep7_dat = *ps++ >> 8;
				}
			}


			abuf_dec_rptr += USB_ISO_IN_SIZE;
			if(abuf_dec_rptr >= (MIC_SHORT_DEC_SIZE<<8) ){
				abuf_dec_rptr = 0;
			}
		}
		else
		{
			for (int i=0; i<USB_ISO_IN_SIZE * 2; i++)
			{
				reg_usb_ep7_dat = 0;
			}
		}
	}
	else
	{
		for (int i=0; i<USB_ISO_IN_SIZE * 2; i++)
		{
			reg_usb_ep7_dat = 0;
		}
		buffer_empty = 1;
	}
	reg_usb_ep7_ctrl = BIT(0);			//ACK iso in
}

_attribute_ram_code_ void  usb_endpoints_irq_handler (void)
{
//	u32 t = clock_time ();
	/////////////////////////////////////
	// ISO IN
	/////////////////////////////////////
	if (reg_usb_irq & BIT(7)) {

		reg_usb_irq = BIT(7);	//clear interrupt flag of endpoint 7

		/////// get MIC input data ///////////////////////////////
		//usb_iso_in_1k_square ();
		//usb_iso_in_from_mic ();
		abuf_dec_usb ();
	}

}

void  att_mic (u16 conn, u8 *p)
{
	att_mic_rcvd = 1;
	memcpy (buff_mic_adpcm, p, MIC_ADPCM_FRAME_SIZE);
	abuf_mic_add ((u32 *)buff_mic_adpcm);
}

_attribute_ram_code_ void  proc_audio (void)
{
	if (att_mic_rcvd)
	{
		tick_adpcm = clock_time ();
		att_mic_rcvd = 0;
	}
	if (clock_time_exceed (tick_adpcm, 200000))
	{
		tick_adpcm = clock_time ();
		abuf_init ();
	}
	abuf_mic_dec ();
}


#endif //#if (UI_AUDIO_ENABLE)
