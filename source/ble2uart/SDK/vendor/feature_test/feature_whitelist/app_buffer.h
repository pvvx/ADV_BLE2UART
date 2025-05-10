/********************************************************************************************************
 * @file    app_buffer.h
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
#ifndef APP_BUFFER_H_
#define APP_BUFFER_H_

#include "tl_common.h"
#include "app_config.h"


#if (FEATURE_TEST_MODE == TEST_WHITELIST)

/**
 * @brief	connMaxRxOctets
 * refer to BLE Core Specification: "4.5.10 Data PDU length management" & "2.4.2.21 LL_LENGTH_REQ and LL_LENGTH_RSP"
 * usage limitation:
 * 1. should be in range of 27 ~ 251
 */
#define ACL_CONN_MAX_RX_OCTETS			27


/**
 * @brief	connMaxTxOctets
 * refer to BLE Core Specification: "4.5.10 Data PDU length management" & "2.4.2.21 LL_LENGTH_REQ and LL_LENGTH_RSP"
 *  in this SDK, we separate this value into 2 parts: slaveMaxTxOctets and masterMaxTxOctets,
 *  for purpose to save some SRAM costed by when slave and master use different connMaxTxOctets.
 * usage limitation:
 * 1. slaveMaxTxOctets and masterMaxTxOctets should be in range of 27 ~ 251
 */
#define ACL_MASTER_MAX_TX_OCTETS		27
#define ACL_SLAVE_MAX_TX_OCTETS			27




/********************* ACL connection LinkLayer TX & RX data FIFO allocation, Begin ************************************************/
/**
 * @brief	ACL RX buffer size & number
 *  		ACL RX buffer is shared by all connections to hold LinkLayer RF RX data.
 * usage limitation for ACL_RX_FIFO_SIZE:
 * 1. should be greater than or equal to (connMaxRxOctets + 22)
 * 2. should be be an integer multiple of 16 (16 Byte align)
 * 3. user can use formula:  size = CAL_LL_ACL_RX_FIFO_SIZE(connMaxRxOctets)
 * usage limitation for ACL_RX_FIFO_NUM:
 * 1. must be: 2^n, (power of 2)
 * 2. at least 4; recommended value: 8, 16
 */
#define ACL_RX_FIFO_SIZE				CAL_LL_ACL_RX_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)
#define ACL_RX_FIFO_NUM					8	// must be: 2^n


/**
 * @brief	ACL TX buffer size & number
 *  		ACL MASTER TX buffer is shared by all master connections to hold LinkLayer RF TX data.
*			ACL SLAVE  TX buffer is shared by all slave  connections to hold LinkLayer RF TX data.
 * usage limitation for ACL_MASTER_TX_FIFO_SIZE & ACL_SLAVE_TX_FIFO_SIZE:
 * 1. should be greater than or equal to (connMaxTxOctets + 10)
 * 2. should be be an integer multiple of 4 (4 Byte align)
 * 3. user can use formula:  size = CAL_LL_ACL_TX_FIFO_SIZE(connMaxTxOctets)
 * usage limitation for ACL_MASTER_TX_FIFO_NUM & ACL_SLAVE_TX_FIFO_NUM:
 * 1. must be: 2^n  (power of 2)
 * 2. at least 8; recommended value: 8, 16, 32; other value not allowed.
 */
#define ACL_MASTER_TX_FIFO_SIZE			CAL_LL_ACL_TX_FIFO_SIZE(ACL_MASTER_MAX_TX_OCTETS)	// ACL_MASTER_MAX_TX_OCTETS + 10, then 4 Byte align
#define ACL_MASTER_TX_FIFO_NUM			8   //different from eagle. 2^n
#define ACL_SLAVE_TX_FIFO_SIZE			CAL_LL_ACL_TX_FIFO_SIZE(ACL_SLAVE_MAX_TX_OCTETS)  // ACL_SLAVE_MAX_TX_OCTETS + 10, then 4 Byte align
#define ACL_SLAVE_TX_FIFO_NUM			8   //different from eagle. 2^n




extern	u8	app_acl_rxfifo[];
extern	u8	app_acl_mstTxfifo[];
extern	u8	app_acl_slvTxfifo[];
/******************** ACL connection LinkLayer TX & RX data FIFO allocation, End ***************************************************/



/***************** ACL connection L2CAP layer MTU TX & RX data FIFO allocation, Begin ********************************/

/*Note:
 * if support LE Secure Connections, L2CAP buffer must >= 72.([64+6]+3)/4*4), 4B align.
 * MTU Buff size = Extra_Len(6)+ ATT_MTU_MAX
 *  1. should be greater than or equal to (ATT_MTU + 6)
 *  2. should be be an integer multiple of 4 (4 Byte align)
 */
#define ATT_MTU_MASTER_RX_MAX_SIZE  23
#define	MTU_M_BUFF_SIZE_MAX			CAL_MTU_BUFF_SIZE(ATT_MTU_MASTER_RX_MAX_SIZE)

#define ATT_MTU_SLAVE_RX_MAX_SIZE   23
#define	MTU_S_BUFF_SIZE_MAX			CAL_MTU_BUFF_SIZE(ATT_MTU_SLAVE_RX_MAX_SIZE)


extern	u8 mtu_m_rx_fifo[];

extern	u8 mtu_s_rx_fifo[];
extern	u8 mtu_s_tx_fifo[];
/***************** ACL connection L2CAP layer MTU TX & RX data FIFO allocation, End **********************************/





#endif /* APP_BUFFER_H_ */

#endif
