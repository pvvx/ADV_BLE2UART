/********************************************************************************************************
 * @file    app_buffer.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#include "app.h"
#include "app_buffer.h"



/********************* ACL connection LinkLayer TX & RX data FIFO allocation, Begin *******************************/

/**
 * @brief	ACL RX buffer. size & number defined in app_buffer.h
 * ACL RX FIFO is shared by all connections to hold LinkLayer RF RX data, user should define this buffer
 * if either ACl connection master role or ACl connection slave role is e used.
 */
u8	app_acl_rxfifo[ACL_RX_FIFO_SIZE * ACL_RX_FIFO_NUM] = {0};


/**
 * @brief	ACL TX buffer. size & number defined in app_buffer.h
 *  ACL MASTER TX buffer should be defined only when ACl connection master role is used.
 *  ACL SLAVE  TX buffer should be defined only when ACl connection slave role is used.
 */
#if(APP_ACL_MASTER_FUNC_EN)
u8	app_acl_mstTxfifo[ACL_MASTER_TX_FIFO_SIZE * ACL_MASTER_TX_FIFO_NUM * ACL_CENTRAL_MAX_NUM] = {0};
#endif
u8	app_acl_slvTxfifo[ACL_SLAVE_TX_FIFO_SIZE * ACL_SLAVE_TX_FIFO_NUM * ACL_PERIPHR_MAX_NUM] = {0};

/******************** ACL connection LinkLayer TX & RX data FIFO allocation, End ***********************************/



/***************************** HCI TX & RX data FIFO allocation, Begin *********************************************/
#if (HCI_NEW_FIFO_FEATURE_ENABLE)

//_attribute_ble_data_retention_
u8	app_bltHci_rxfifo[HCI_RX_FIFO_SIZE * HCI_RX_FIFO_NUM] = {0};
//_attribute_ble_data_retention_
u8	app_bltHci_txfifo[HCI_TX_FIFO_SIZE * HCI_TX_FIFO_NUM] = {0};
//_attribute_ble_data_retention_
u8	app_hci_aclDataFifo[HCI_ACL_DATA_FIFO_SIZE * HCI_ACL_DATA_FIFO_NUM] = {0};
#else
//_attribute_ble_data_retention_
							u8 		 	hci_rxfifo_b[HCI_RX_FIFO_SIZE * HCI_RX_FIFO_NUM] = {0};
_attribute_ble_data_retention_	my_fifo_t	hci_rxfifo = {
												HCI_RX_FIFO_SIZE,
												HCI_RX_FIFO_NUM,
												0,
												0,
												hci_rxfifo_b,};

//_attribute_ble_data_retention_
							u8 		 	hci_txfifo_b[HCI_TX_FIFO_SIZE * HCI_TX_FIFO_NUM] = {0};
_attribute_ble_data_retention_	my_fifo_t	hci_txfifo = {
												HCI_TX_FIFO_SIZE,
												HCI_TX_FIFO_NUM,
												0,
												0,
												hci_txfifo_b,};
#endif
/****************************** HCI TX & RX data FIFO allocation, ENd *********************************************/



/***************************** ext adv FIFO allocation, Begin *********************************************/
#if APP_LE_EXTENDED_ADV_EN
_attribute_ble_data_retention_	u8  app_advSet_buffer[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8 	app_advData_buffer[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8 	app_scanRspData_buffer[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];
#endif
/******************************  ext adv  data FIFO allocation, End *********************************************/



/***************************** per adv FIFO allocation, Begin *********************************************/
#if APP_LE_PERIODIC_ADV_EN
_attribute_ble_data_retention_	u8	app_perdAdvSet_buffer[PERD_ADV_PARAM_LENGTH * APP_PER_ADV_SETS_NUMBER];
_attribute_ble_data_retention_	u8 	app_perdAdvData_buffer[APP_MAX_LENGTH_PER_ADV_DATA * APP_PER_ADV_SETS_NUMBER];
#endif
/******************************  per adv  data FIFO allocation, End *********************************************/








