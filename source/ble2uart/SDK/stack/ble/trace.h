/********************************************************************************************************
 * @file	trace.h
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
#ifndef STACK_BLE_TRACE_H_
#define STACK_BLE_TRACE_H_





//log_event, ID: 0~31
#define			SLEV_reservd				0

#define			SLEV_rebuild				10
#define			SLEV_rst_sSlot				11




//log_tick, ID: 0~31

#define			SLET_00_systimer			0
#define 		SLET_01_rx					1
#define 		SLET_02_rx_scn				2
#define			SLET_03_rx_slv     			3
#define			SLET_04_rx_mas				4
#define			SLET_timestamp				31

//log_task, ID: 0~31
#define			SL01_01_scan				1
#define			SL01_02_adv     			2
#define			SL01_03_btx1     			3
#define			SL01_04_btx2     			4
#define			SL01_05_btx3     			5
#define			SL01_06_btx4     			6
#define			SL01_07_brx1     			7
#define			SL01_08_brx2     			8
#define			SL01_09_brx3     			9
#define			SL01_10_brx4     			10

#define			SL01_29_sysTimer			29
#define			SL01_30_rf					30
#define			SL01_31_IRQ					31





// 8-bit data: cid0 - cid63
#define			SL08_reserved				0



// 16-bit data: sid0 - sid63
#define			SL16_reserved				0




#endif /* STACK_BLE_TRACE_H_ */
