/********************************************************************************************************
 * @file	debug.h
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
#ifndef STACK_BLE_DEBUG_H_
#define STACK_BLE_DEBUG_H_




#ifndef	DEBUG_PAIRING_ENCRYPTION
#define DEBUG_PAIRING_ENCRYPTION								0
#endif


/* BLE rf irq timing && log enable */
#ifndef	BLE_IRQ_DBG_EN
#define BLE_IRQ_DBG_EN											0
#endif

/* BLE Tx fifo log enable */
#ifndef	TX_FIFO_DBG_EN
#define TX_FIFO_DBG_EN											0
#endif

/* BLE smp trans.. log enable */
#ifndef	SMP_DBG_EN
#define SMP_DBG_EN												0
#endif


#ifndef	TX_PUSH_DATA_LOG
#define TX_PUSH_DATA_LOG										0
#endif

#ifndef	RX_L2CAP_DATA_LOG
#define RX_L2CAP_DATA_LOG										0
#endif


#ifndef	SCHE_DEBUG_DUMP_EN
#define SCHE_DEBUG_DUMP_EN										0
#endif

#ifndef	SCHE_TIMING_DEBUG_EN
#define SCHE_TIMING_DEBUG_EN									0
#endif


#ifndef	DBG_BOUNDARY_RX
#define DBG_BOUNDARY_RX											0
#endif


#ifndef	DBG_LL_CTRL_LOG_EN
#define DBG_LL_CTRL_LOG_EN									    0
#endif


#ifndef	DBG_DLE_DUMP_EN
#define DBG_DLE_DUMP_EN									       	0
#endif

#ifndef	DBG_SCHE_TIMING_EN
#define DBG_SCHE_TIMING_EN									    0
#endif


#ifndef	DBG_IAL_EN
#define DBG_IAL_EN									   		 	0
#endif

#ifndef	DEB_CIG_MST_EN
#define DEB_CIG_MST_EN											0
#endif

#ifndef	DEB_CIG_SLV_EN
#define DEB_CIG_SLV_EN											0
#endif

#ifndef	DEB_BIG_BCST_EN
#define DEB_BIG_BCST_EN											0
#endif

#ifndef	DEB_BIG_SYNC_EN
#define DEB_BIG_SYNC_EN											0
#endif

#ifndef DEB_ISO_TEST_EN
#define	DEG_ISO_TEST_EN											0
#endif

#ifndef DEB_STRUCT_BUFFER_SIZE_CHECK
#define	DEB_STRUCT_BUFFER_SIZE_CHECK							0
#endif

#ifndef DEG_PRIVATE_FANQH
#define	DEG_PRIVATE_FANQH										0
#endif


#if (DBG_SLAVE_CONN_UPDATE)
#define			SLET_upt_cmd_1				10
#define 		SLET_upt_cmd_2				11
#define 		SLET_upt_cmd_3				12
#define 		SLET_upt_cmd_4				13
#define			SLET_upt_sync_1				20
#define 		SLET_upt_sync_2				21
#define 		SLET_upt_sync_3				22
#define 		SLET_upt_sync_4				23
#endif

#if 0
#define 		SLET_05_rx_crc				5
#define 		SLET_06_rx_1st				6
#define 		SLET_07_rx_new				7
#define			SLET_10_tx					10
#define 		SLET_11_c_cmdone			11
#define 		SLET_12_c_1stRxTmt			12
#define 		SLET_13_c_rxTmt				13
#define 		SLET_14_c_rxCrc2			14
#endif





#if 0
#define			SLEV_txFifo_push			17
#define			SLEV_txFifo_empty			18
#define			SLEV_txFifo_RX				19
#define			SLEV_txFifo_post			20
#define 		SLEV_test_event				31

#define			SL16_tf_hw_push				1
#define			SL16_tf_sw_push				2
#define			SL16_tf_hw_load1			3
#define			SL16_tf_sw_load1			4
#define			SL16_tf_hw_load2			5
#define			SL16_tf_sw_load2			6
#define			SL16_tf_hw_RX				7
#define			SL16_tf_sw_RX				8
#define			SL16_tf_hw_TX				9
#define			SL16_tf_sw_TX				10
#define			SL16_tf_hw_post				11
#define			SL16_tf_sw_post				12
#define			SL16_tf_save				13

#define			SL16_seq_notify				15
#define			SL16_seq_write				16
#define			SL16_test_2B				17
#endif




#endif /* STACK_BLE_DEBUG_H_ */
