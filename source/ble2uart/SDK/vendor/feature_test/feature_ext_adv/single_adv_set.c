/********************************************************************************************************
 * @file    single_adv_set.c
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

#include "application/usbstd/usbkeycode.h"
#include "application/keyboard/keyboard.h"
#include "app_buffer.h"


#if (FEATURE_TEST_MODE == TEST_EXT_ADV)
#if (ADV_SET_MODE == SINGLE_ADV_SET)


extern	u8	tbl_advData[22];
extern	u8	tbl_scanRsp[12];
ble_sts_t   ble_status = BLE_SUCCESS;

//Legacy, Unconnectable Undirected
#define ADV_EVENT_PROP__LEGACY_UNCONNECTABLE										0

//Legacy, Connectable_Scannable, Undirected
#define ADV_EVENT_PROP__LEGACY_CONN_SCAN											1

//Legacy, Connectable_Scannable, Undirected
#define ADV_EVENT_PROP__LEGACY_SCAN_UNDIRECTED										2

// Extended, None_Connectable_None_Scannable undirected, without auxiliary packet
#define ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_UNDIRECTED__WITHOUT_AUX_PKT			10

// Extended, None_Connectable_None_Scannable directed, without auxiliary packet
#define ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_DIRECTED__WITHOUT_AUX_PKT			11

// Extended, None_Connectable_None_Scannable undirected, with auxiliary packet
#define ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_UNDIRECTED__WITH_AUX_PKT			12

// Extended, None_Connectable_None_Scannable Directed, with auxiliary packet
#define ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_DIRECTED__WITH_AUX_PKT				13

// Extended, Scannable, Undirected
#define ADV_EVENT_PROP__EXT_SCAN_UNDIRECTED											14

// Extended, Connectable, Undirected
#define ADV_EVENT_PROP__EXT_CONN_UNDIRECTED											15

/** Number of Supported Advertising Sets, no exceed "ADV_SETS_NUMBER_MAX" */
#define	APP_ADV_SETS_NUMBER															1

/** Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_ADV_DATA														1024

/** Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		  									1024


/* for user to select adv_event_property when single adv_set is tested */
#define ADV_EVENT_PROPERTY_SELECT													13


_attribute_ble_data_retention_	u32 my_adv_interval_min = ADV_INTERVAL_30MS;

_attribute_ble_data_retention_	u32 my_adv_interval_max = ADV_INTERVAL_30MS;

_attribute_ble_data_retention_	u8  app_advSet_buffer[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8 	app_advData_buffer[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8 	app_scanRspData_buffer[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8	testAdvData[APP_MAX_LENGTH_ADV_DATA];

_attribute_ble_data_retention_  u8  testScanRspData[APP_MAX_LENGTH_SCAN_RESPONSE_DATA];

//_attribute_ble_data_retention_	u8	app_primaryAdvPkt_buffer[MAX_LENGTH_PRIMARY_ADV_PKT * APP_ADV_SETS_NUMBER];

//_attribute_ble_data_retention_	u8	app_secondary_adv_pkt[MAX_LENGTH_SECOND_ADV_PKT * APP_ADV_SETS_NUMBER];


void app_single_adv_set_register_buffer(void)
{
	blc_ll_initExtendedAdvertising_module();

	blc_ll_initExtendedAdvSetBuffer(app_advSet_buffer, APP_ADV_SETS_NUMBER);

	blc_ll_initExtendedAdvDataBuffer(app_advData_buffer, APP_MAX_LENGTH_ADV_DATA);

	blc_ll_initExtendedScanRspDataBuffer(app_scanRspData_buffer, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
}


void app_single_adv_set_test(void)
{
#if (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__LEGACY_UNCONNECTABLE)
	//Legacy, non_connectable_non_scannable
	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   0);

	blc_ll_setExtAdvData( ADV_HANDLE0, sizeof(tbl_advData), (u8 *)tbl_advData);

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);

#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__LEGACY_CONN_SCAN)
	//Legacy, Connectable_Scannable, Undirected
	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 								   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,										   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 											   0);

	blc_ll_setExtAdvData( ADV_HANDLE0, sizeof(tbl_advData), (u8 *)tbl_advData);

	blc_ll_setExtScanRspData( ADV_HANDLE0,  sizeof(tbl_scanRsp), (u8 *)tbl_scanRsp);

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);

#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__LEGACY_SCAN_UNDIRECTED)
	//Legacy, Connectable_Scannable, Undirected
	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_LEGACY_SCANNABLE_UNDIRECTED,		my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 							BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,									BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 										0);


	blc_ll_setExtScanRspData( ADV_HANDLE0,  sizeof(tbl_scanRsp), (u8 *)tbl_scanRsp);

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);

#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_UNDIRECTED__WITHOUT_AUX_PKT)
	// Extended, None_Connectable_None_Scannable undirected, without auxiliary packet

//	u16 event_prop = ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED;
	u16 event_prop = ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED | ADV_EVT_PROP_EXTENDED_MASK_TX_POWER_INCLUDE;

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		event_prop, 												   my_adv_interval_min,				my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   0);

//	blc_ll_setExtAdvData( ADV_HANDLE0, 0 , NULL);   //do not set ADV data, or set it with advData_len "0"

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);


#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_DIRECTED__WITHOUT_AUX_PKT)
	// Extended, None_Connectable_None_Scannable directed, without auxiliary packet

	u8 test_peer_type = BLE_ADDR_PUBLIC;  // BLE_ADDR_RANDOM
	u8 test_peer_mac[6] = {0x11,0x11,0x11,0x11,0x11,0x11};

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED,  my_adv_interval_min,				my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   test_peer_type, 					test_peer_mac,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   0);

//	blc_ll_setExtAdvData( ADV_HANDLE0, 0 , NULL);   //do not set ADV data, or set it with advData_len "0"

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);


#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_UNDIRECTED__WITH_AUX_PKT)
	// Extended, None_Connectable_None_Scannable undirected, with auxiliary packet

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);

	for(int i=0;i<1024;i++){
		testAdvData[i] = i;
	}

//	blc_ll_setExtAdvData( ADV_HANDLE0,    31, testAdvData);
//	blc_ll_setExtAdvData( ADV_HANDLE0, 	 200, testAdvData);
//	blc_ll_setExtAdvData( ADV_HANDLE0, 	 400, testAdvData);
//	blc_ll_setExtAdvData( ADV_HANDLE0, 	 600, testAdvData);
	blc_ll_setExtAdvData( ADV_HANDLE0,  1024, testAdvData);

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);

#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__EXT_NONE_CONN_NONE_SCAN_DIRECTED__WITH_AUX_PKT)
	// Extended, None_Connectable_None_Scannable Directed, with auxiliary packet

	u8 test_peer_type = BLE_ADDR_RANDOM;  // BLE_ADDR_RANDOM
	u8 test_peer_mac[6] = {0x11,0x11,0x11,0x11,0x11,0x11};

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED,
																												my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    test_peer_type, 				test_peer_mac,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);


	for(int i=0;i<1024;i++){
		testAdvData[i]=i;
	}

	blc_ll_setExtAdvData( ADV_HANDLE0,  1024, testAdvData);

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);


#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__EXT_SCAN_UNDIRECTED)
	// Extended, Scannable, Undirected

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_SCANNABLE_UNDIRECTED, 					my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);

	//Extended Scannable Event do not have ADV data
	for(int i=0;i<1024;i++){
		testScanRspData[i]=i;
	}

	blc_ll_setExtScanRspData( ADV_HANDLE0, 1024, testScanRspData);

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);


#elif (ADV_EVENT_PROPERTY_SELECT == ADV_EVENT_PROP__EXT_CONN_UNDIRECTED)
	// Extended, Connectable, Undirected

	blc_ll_initChannelSelectionAlgorithm_2_feature();
//	blc_ll_setDefaultConnCodingIndication(CODED_PHY_PREFER_S2);

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, 					my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_2M, 	 		ADV_SID_0, 													   	0);

	blc_ll_setExtAdvData( ADV_HANDLE0, sizeof(tbl_advData) , (u8 *)tbl_advData);


	//Extended Connectable Event do not have scan_rsp data

	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);

#else

#endif



//	blc_ll_setAuxAdvChnIdxByCustomers(1); //debug chn_2406
//	blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);
}


#endif // end of (ADV_SET_MODE == SINGLE_ADV_SET)
#endif // end of (FEATURE_TEST_MODE == TEST_EXT_ADV)
