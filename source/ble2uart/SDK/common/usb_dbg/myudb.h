/********************************************************************************************************
 * @file    myudb.h
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
#ifndef		__MYUDB_H__
#define		__MYUDB_H__
#pragma once



#define	log_hw_ref()

// 4-byte sync word: 00 00 00 00
#define	log_sync(en)
//4-byte (001_id-5bits) id0: timestamp align with hardware gpio output; id1-31: user define
#define	log_tick(en,id)

//1-byte (000_id-5bits)
#define	log_event(en,id)

//1-byte (01x_id-5bits) 1-bit data: id0 & id1 reserved for hardware
#define	log_task(en,id,b)

//2-byte (10-id-6bits) 8-bit data
#define	log_b8(en,id,d)

//3-byte (11-id-6bits) 16-bit data
#define	log_b16(en,id,d)




#define	log_tick_irq(en,id)
#define	log_tick_irq_2(en,id,t)


#define	log_event_irq(en,id)


#define	log_task_irq(en,id,b)

#define	log_task_begin_irq(en,id)
#define	log_task_end_irq(en,id)

#define	log_task_begin_irq_2(en,id,t)
#define	log_task_end_irq_2(en,id,t)



#define	log_b8_irq(en,id,d)

#define	log_b16_irq(en,id,d)




#endif
