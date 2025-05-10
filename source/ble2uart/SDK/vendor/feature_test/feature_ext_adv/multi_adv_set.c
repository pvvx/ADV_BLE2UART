/********************************************************************************************************
 * @file    multi_adv_set.c
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
//#include "app_att.h"
//#include "app_ui.h"


#if (FEATURE_TEST_MODE == TEST_EXT_ADV)
#if (ADV_SET_MODE == MULTI_ADV_SETS)

extern	u8	tbl_advData[25];
extern	u8	tbl_scanRsp[12];

const u8	tbl_advData_0 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'A','A','A','A','A','A','A','A','A','A','A','A','A','A','A','A',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};

const u8	tbl_scanRsp_0 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'A','A','A','A','A','A','A','A','A','A','A','A','A','A','A','A',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};


const u8	tbl_advData_1 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};

const u8	tbl_scanRsp_1 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};



const u8	tbl_advData_2 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'C','C','C','C','C','C','C','C','C','C','C','C','C','C','C','C',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};

const u8	tbl_scanRsp_2 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'C','C','C','C','C','C','C','C','C','C','C','C','C','C','C','C',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};



const u8	tbl_advData_3 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};

const u8	tbl_scanRsp_3 [] = {
	 17,	DT_COMPLETE_LOCAL_NAME, 				'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
	 2, 	DT_FLAGS, 								0x05,
	 3, 	DT_APPEARANCE, 					  		0x80, 0x01,
	 5, 	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID, 	0x12, 0x18, 0x0F, 0x18,
};



/** Number of Supported Advertising Sets, no exceed "ADV_SETS_NUMBER_MAX" */
#define	APP_ADV_SETS_NUMBER						   				4 //Max support up to 4 ADV sets

/** Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_ADV_DATA									1024

/** Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		  				1024

_attribute_ble_data_retention_	u32  my_adv_interval_min = ADV_INTERVAL_50MS;

_attribute_ble_data_retention_	u32  my_adv_interval_max = ADV_INTERVAL_50MS;

_attribute_ble_data_retention_	u8  app_advSet_buffer[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8 	app_advData_buffer[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8 	app_scanRspData_buffer[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8	testAdvData[APP_MAX_LENGTH_ADV_DATA];

_attribute_ble_data_retention_  u8  testScanRspData[APP_MAX_LENGTH_SCAN_RESPONSE_DATA];


void app_multiple_adv_set_register_buffer(void)
{
	blc_ll_initExtendedAdvertising_module();

	blc_ll_initExtendedAdvSetBuffer(app_advSet_buffer, APP_ADV_SETS_NUMBER);

	blc_ll_initExtendedAdvDataBuffer(app_advData_buffer, APP_MAX_LENGTH_ADV_DATA);

	blc_ll_initExtendedScanRspDataBuffer(app_scanRspData_buffer, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
}


void app_multiple_adv_set_test(void)
{
#if 1
	/* adv_set 0: Legacy, Unconnectable Undirected
	   adv_set 1: Legacy, Unconnectable Undirected
	   adv_set 2: Legacy, Unconnectable Undirected
	   adv_set 3: Legacy, Unconnectable Undirected */

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   0);

	blc_ll_setExtAdvData( ADV_HANDLE0,  sizeof(tbl_advData_0) , (u8 *)tbl_advData_0);


	blc_ll_setExtAdvParam( ADV_HANDLE1, 		ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_2M, 	 		ADV_SID_1, 													   0);

	blc_ll_setExtAdvData( ADV_HANDLE1,  sizeof(tbl_advData_1) , (u8 *)tbl_advData_1);


	blc_ll_setExtAdvParam( ADV_HANDLE2, 		ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_CODED, 	 	ADV_SID_2, 													   0);

	blc_ll_setExtAdvData( ADV_HANDLE2,  sizeof(tbl_advData_2) , (u8 *)tbl_advData_2);


	blc_ll_setExtAdvParam( ADV_HANDLE3, 		ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_CODED, 	 	ADV_SID_3, 													   0);

	blc_ll_setExtAdvData( ADV_HANDLE3,  sizeof(tbl_advData_3) , (u8 *)tbl_advData_3);


#elif 0
	/* adv_set 0: Legacy, Connectable_Scannable, Undirected
	   adv_set 1: Legacy, Connectable_Scannable, Undirected
	   adv_set 2: Legacy, Connectable_Scannable, Undirected
	   adv_set 3: Legacy, Connectable_Scannable, Undirected */
	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED,  		   my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   0);

	blc_ll_setExtAdvData	( ADV_HANDLE0,  sizeof(tbl_advData_0), (u8 *)tbl_advData_0);
	blc_ll_setExtScanRspData( ADV_HANDLE0,  sizeof(tbl_scanRsp_0), (u8 *)tbl_scanRsp_0);




	blc_ll_setExtAdvParam( ADV_HANDLE1, 		ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED,  		   my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_2M, 	 		ADV_SID_1, 													   0);

	blc_ll_setExtAdvData	( ADV_HANDLE1,  sizeof(tbl_advData_1), (u8 *)tbl_advData_1);
	blc_ll_setExtScanRspData( ADV_HANDLE1,  sizeof(tbl_scanRsp_1), (u8 *)tbl_scanRsp_1);



	blc_ll_setExtAdvParam( ADV_HANDLE2, 		ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED, 		   my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_CODED, 	 		ADV_SID_2, 													   0);

	blc_ll_setExtAdvData	( ADV_HANDLE2,  sizeof(tbl_advData_2), (u8 *)tbl_advData_2);
	blc_ll_setExtScanRspData( ADV_HANDLE2,  sizeof(tbl_scanRsp_2), (u8 *)tbl_scanRsp_2);



	blc_ll_setExtAdvParam( ADV_HANDLE3, 		ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED, 		   my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_CODED, 	 	ADV_SID_3, 													   0);

	blc_ll_setExtAdvData	( ADV_HANDLE3,  sizeof(tbl_advData_3), (u8 *)tbl_advData_3);
	blc_ll_setExtScanRspData( ADV_HANDLE3,  sizeof(tbl_scanRsp_3), (u8 *)tbl_scanRsp_3);

#elif 0
	/* adv_set 0: Extended, Connectable_Non_scannable
	   adv_set 1: Extended, Non_Connectable_Non_Scannable undirected, with auxiliary packet
	   adv_set 2: Extended, Scannable, Undirected
	   adv_set 3: Extended, Connectable, Undirected */

	for(int i=0;i<1024;i++){
		testAdvData[i]=i;
		testScanRspData[i]=i;
	}

	blc_ll_initChannelSelectionAlgorithm_2_feature();
	blc_ll_setDefaultConnCodingIndication(CODED_PHY_PREFER_S8);

	//adv_set 0: Extended, Connectable_Non_scannable
	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED,					my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 						  					BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,								  					BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 									  					0);

	blc_ll_setExtAdvData( ADV_HANDLE0,  sizeof(tbl_advData_1) , (u8 *)tbl_advData_1);
//	blc_ll_setExtScanRspData( ADV_HANDLE0,  sizeof(tbl_scanRsp_1), (u8 *)tbl_scanRsp_1);




	//adv_set 1: Extended, None_Connectable_None_Scannable undirected, with auxiliary packet
	blc_ll_setExtAdvParam( ADV_HANDLE1, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										     BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												     BLE_PHY_1M, 					0,
						   BLE_PHY_2M, 	 		ADV_SID_1, 													     0);

	blc_ll_setExtAdvData( ADV_HANDLE1, 1024, testAdvData);
//	blc_ll_setExtScanRspData( ADV_HANDLE1, 1024, testScanRspData);




	/* adv_set 2: Extended, Scannable, Undirected */
	blc_ll_setExtAdvParam( ADV_HANDLE2, 		ADV_EVT_PROP_EXTENDED_SCANNABLE_UNDIRECTED, 					 my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										     BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   	 BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_2, 													     0);

	blc_ll_setExtAdvData	( ADV_HANDLE2,  1024, testAdvData);
	blc_ll_setExtScanRspData( ADV_HANDLE2,  1024, testScanRspData);




	/* adv_set 3: Extended, Connectable, Undirected */
	blc_ll_setExtAdvParam( ADV_HANDLE3, 		ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, 					my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_2M, 	 		ADV_SID_3, 													   	0);

	blc_ll_setExtAdvData	( ADV_HANDLE3,  1024, testAdvData);
	blc_ll_setExtScanRspData( ADV_HANDLE3,  1024, testScanRspData);
#endif




	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);
	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE1, 0 , 0);
	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE2, 0 , 0);
	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE3, 0 , 0);

//	blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);
}


#endif // end of (ADV_SET_MODE == MULTI_ADV_SETS)
#endif // of of (FEATURE_TEST_MODE == TEST_EXT_ADV)
