/********************************************************************************************************
 * @file    rf_frame.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef _RF_FRAME_H_
#define _RF_FRAME_H_
#include "../drivers.h"
#include "keyboard/keyboard.h"



#define		RF_PROTO_BYTE		0x51
#define		PIPE0_CODE			0x55556666
#define		PIPE1_CODE			0xaabbccdd



#define MOUSE_FRAME_DATA_NUM   4

typedef struct {
	u8 btn;
	s8 x;
	s8 y;
	s8 wheel;
}mouse_data_t;


typedef struct{
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid0;		//pipe0 code,	used as sync code for control pipe in hamster

	u8	rssi;
	u8	per;
	u8	seq_no;
	u8	rsvd;

	u32 did;

}rf_packet_pairing_t;

typedef struct {
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid;		//pipe0 code,	used as sync code for control pipe in hamster

	u8	rssi;
	u8	per;
	u8	seq_no;
	u8	pno;
	u8 data[MOUSE_FRAME_DATA_NUM*sizeof(mouse_data_t)]; //now the data length is variable, if the previous no ACK, data will send again in next time

}rf_packet_mouse_t;

typedef struct {
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid;		//pipe0 code,	used as sync code for control pipe in hamster

	u8	rssi;
	u8	per;
	u8	seq_no;
	u8	pno;

	u32 did;

	u8 data[4*sizeof(kb_data_t)]; //now the data length is variable, if the previous no ACK, data will send again in next time

}rf_packet_keyboard_t;

////////////////////////// host side ///////////////////////////////
typedef struct{
	u32 dma_len;

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid0;

	u8	rssi;
	u8	per;
	u16	tick;

	u8	chn;
	u8	info0;
	u8	info1;
	u8	info2;

	u32 gid1;
	u32 did;

}rf_packet_debug_t;


typedef struct{
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number
	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid0;		//pipe0 code,	used as sync code for control pipe in hamster

	u8	rssi;
	u8	per;
	u16	tick;
	u8	chn;
}rf_ack_empty_t;


typedef struct{
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid0;		//pipe0 code,	used as sync code for control pipe in hamster

	u8	rssi;
	u8	per;
	u16	tick;
	u8	chn;
	u8	info0;
	u8	info1;
	u8	info2;

	u32 gid1;		//pipe1 code,	used as sync code for data pipe in hamster
	u32 did;

}rf_packet_ack_pairing_t;

typedef struct{
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid;		//pipe0 code,	used as sync code for control pipe in hamster

	u8	rssi;
	u8	per;
	u16	tick;
	u8	chn;

    u8  info;
}rf_packet_ack_mouse_t;

typedef struct{
	u32 dma_len;            //won't be a fixed number as previous, should adjust with the mouse package number

	u8  rf_len;
	u8	proto;
	u8	flow;
	u8	type;

//	u32 gid;		//pipe0 code,	used as sync code for control pipe in hamster

	u8	rssi;
	u8	per;
	u16	tick;
	u8	chn;
	u8	status;
}rf_packet_ack_keyboard_t;

#endif /* LED_RF_FRAME_H_ */
