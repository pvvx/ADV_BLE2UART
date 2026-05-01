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



/**
 * @brief	connMaxRxOctets
 * refer to BLE Core Specification: "4.5.10 Data PDU length management" & "2.4.2.21 LL_LENGTH_REQ and LL_LENGTH_RSP"
 * usage limitation:
 * 1. should be in range of 27 ~ 251
 */
#ifndef ACL_CONN_MAX_RX_OCTETS
#define ACL_CONN_MAX_RX_OCTETS			27  //DLE
#endif

/**
 * @brief	connMaxTxOctets
 * refer to BLE Core Specification: "4.5.10 Data PDU length management" & "2.4.2.21 LL_LENGTH_REQ and LL_LENGTH_RSP"
 *  in this SDK, we separate this value into 2 parts: slaveMaxTxOctets and masterMaxTxOctets,
 *  for purpose to save some SRAM costed by when slave and master use different connMaxTxOctets.
 * usage limitation:
 * 1. slaveMaxTxOctets and masterMaxTxOctets should be in range of 27 ~ 251
 */
#ifndef ACL_MASTER_MAX_TX_OCTETS
#define ACL_MASTER_MAX_TX_OCTETS		27  //DLE
#endif
#ifndef ACL_SLAVE_MAX_TX_OCTETS
#define ACL_SLAVE_MAX_TX_OCTETS			27  //DLE
#endif

/**
 * @brief	LE_ACL_Data_Packet_Length, refer to BLE SPEC "7.8.2 LE Read Buffer Size command"
 * usage limitation:
 * 1. only used for BLE controller project
 * 2. must be an integer multiple of 4, such as 32,80,200...
 * 3. must greater than maximum of ACL_SLAVE_MAX_TX_OCTETS, ACL_MASTER_MAX_TX_OCTETS
 * 4. can not exceed 252(for maximum tx_octets when DLE used)
 */
#ifndef LE_ACL_DATA_PACKET_LENGTH
#define LE_ACL_DATA_PACKET_LENGTH		(28)
#endif



/********************* ACL connection LinkLayer TX & RX data FIFO allocation, Begin ************************************************/
/**
 * @brief	ACL RX buffer size & number
 *  		ACL RX buffer is shared by all connections to hold LinkLayer RF RX data.
 * usage limitation for ACL_RX_FIFO_SIZE:
 * 1. should be greater than or equal to (connMaxRxOctets + 22)
 * 2. should be be an integer multiple of 16 (16 Byte align). user can use formula:  size = CAL_LL_ACL_RX_FIFO_SIZE(connMaxRxOctets)
 * usage limitation for ACL_RX_FIFO_NUM:
 * 1. must be: 2^n, (power of 2)
 * 2. at least 4; recommended value: 8, 16
 */
#ifndef ACL_RX_FIFO_SIZE
#define ACL_RX_FIFO_SIZE				CAL_LL_ACL_RX_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)
#endif
#ifndef ACL_RX_FIFO_NUM
#define ACL_RX_FIFO_NUM					8	// must be: 2^n
#endif

/**
 * @brief	ACL TX buffer size & number
 *  		ACL MASTER TX buffer is shared by all master connections to hold LinkLayer RF TX data.
*			ACL SLAVE  TX buffer is shared by all slave  connections to hold LinkLayer RF TX data.
 * usage limitation for ACL_MASTER_TX_FIFO_SIZE & ACL_SLAVE_TX_FIFO_SIZE:
 * 1. should be greater than or equal to (connMaxRxOctets + 10)
 * 2. should be be an integer multiple of 4 (4 Byte align). user can use formula:  size = CAL_LL_ACL_TX_FIFO_SIZE(connMaxRxOctets)
 * 3. can use:  size = CAL_LL_ACL_RX_FIFO_SIZE(maxRxOct)
 * usage limitation for ACL_MASTER_TX_FIFO_NUM & ACL_SLAVE_TX_FIFO_NUM:
 * 1. must be: (2^n), (power of 2)
 * 2. at least 8; recommended value: 8, 16, 32; other value not allowed.
 */
#define ACL_MASTER_TX_FIFO_SIZE			CAL_LL_ACL_TX_FIFO_SIZE(ACL_MASTER_MAX_TX_OCTETS)	// ACL_MASTER_MAX_TX_OCTETS + 10, then 4 Byte align
#ifndef ACL_MASTER_TX_FIFO_NUM
#define ACL_MASTER_TX_FIFO_NUM			8   //different from eagle. 2^n
#endif

#define ACL_SLAVE_TX_FIFO_SIZE			CAL_LL_ACL_TX_FIFO_SIZE(ACL_SLAVE_MAX_TX_OCTETS)  // ACL_SLAVE_MAX_TX_OCTETS + 10, then 4 Byte align
#ifndef ACL_SLAVE_TX_FIFO_NUM
#define ACL_SLAVE_TX_FIFO_NUM			8   //different from eagle. 2^n
#endif



extern	u8	app_acl_rxfifo[];
extern	u8	app_acl_mstTxfifo[];
extern	u8	app_acl_slvTxfifo[];
/******************** ACL connection LinkLayer TX & RX data FIFO allocation, End ***************************************************/




/***************************** HCI TX & RX data FIFO allocation, Begin *********************************************/
#ifndef HCI_MAX_TX_SIZE
#define HCI_MAX_TX_SIZE					max2(ACL_SLAVE_MAX_TX_OCTETS, ACL_MASTER_MAX_TX_OCTETS) //support common tx max
#endif
#ifndef HCI_TX_FIFO_SIZE
#define HCI_TX_FIFO_SIZE				((7 + HCI_MAX_TX_SIZE + 4)& ~3)//80//HCI_FIFO_SIZE(HCI_MAX_TX_SIZE)
#endif
#ifndef HCI_TX_FIFO_NUM
#define HCI_TX_FIFO_NUM					16
#endif
#ifndef HCI_RX_FIFO_SIZE
#define HCI_RX_FIFO_SIZE				((1+4+ HCI_MAX_TX_SIZE +16) & ~15)//HCI_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)
#endif
#ifndef HCI_RX_FIFO_NUM
#define HCI_RX_FIFO_NUM					8
#endif

#define HCI_ACL_DATA_FIFO_SIZE			CALCULATE_HCI_ACL_DATA_FIFO_SIZE(LE_ACL_DATA_PACKET_LENGTH)
#ifndef HCI_ACL_DATA_FIFO_NUM
#define HCI_ACL_DATA_FIFO_NUM			8
#endif



#if (HCI_NEW_FIFO_FEATURE_ENABLE)
extern	u8	app_bltHci_rxfifo[];
extern	u8	app_bltHci_txfifo[];
extern	u8	app_hci_aclDataFifo[];
#else
extern  u8  hci_rxfifo_b[];
extern  u8  hci_txfifo_b[];
#endif

//HCI UART variables
#define UART_DATA_LEN    			HCI_TX_FIFO_SIZE

typedef struct{
    unsigned int  len;
    unsigned char data[UART_DATA_LEN];
}uart_data_t;

/****************************** HCI TX & RX data FIFO allocation, End *********************************************/


/***************************** ext adv FIFO allocation, Begin *********************************************/
/** Number of Supported Advertising Sets, no exceed "ADV_SETS_NUMBER_MAX" */
#ifndef APP_ADV_SETS_NUMBER
#define	APP_ADV_SETS_NUMBER						   				4  //Max support up to 4 ADV sets
#endif
/** Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough) */
#ifndef APP_MAX_LENGTH_ADV_DATA
#define APP_MAX_LENGTH_ADV_DATA									250//260//288//1024
#endif
/** Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough) */
#ifndef  APP_MAX_LENGTH_SCAN_RESPONSE_DATA
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		  				250//260//288//1024
#endif
#if APP_LE_EXTENDED_ADV_EN
extern u8 app_advSet_buffer[];
extern u8 app_advData_buffer[];
extern u8 app_scanRspData_buffer[];
#endif
/******************************  ext adv  data FIFO allocation, End *********************************************/


/***************************** per adv FIFO allocation, Begin *********************************************/
/** Number of Supported Periodic Advertising Sets, no exceed "PERIODIC_ADV_NUMBER_MAX" */
#ifndef APP_PER_ADV_SETS_NUMBER
#define	APP_PER_ADV_SETS_NUMBER														1
#endif
/** Maximum Periodic Advertising Data Length */
#ifndef APP_MAX_LENGTH_PER_ADV_DATA
#define APP_MAX_LENGTH_PER_ADV_DATA													255
#endif
#if APP_LE_PERIODIC_ADV_EN
extern u8 app_perdAdvSet_buffer[];
extern u8 app_perdAdvData_buffer[];
#endif
/******************************  per adv  data FIFO allocation, End *********************************************/

#endif /* APP_BUFFER_H_ */
