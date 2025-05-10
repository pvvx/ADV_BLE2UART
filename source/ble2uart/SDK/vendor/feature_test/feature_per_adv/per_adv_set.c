/********************************************************************************************************
 * @file    per_adv_set.c
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


#if (FEATURE_TEST_MODE == TEST_PER_ADV)


/** Number of Supported Advertising Sets, no exceed "ADV_SETS_NUMBER_MAX" */
#define	APP_ADV_SETS_NUMBER															1

/** Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_ADV_DATA														1024

/** Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		  									1024

/** Number of Supported Periodic Advertising Sets, no exceed "PERIODIC_ADV_NUMBER_MAX" */
#define	APP_PER_ADV_SETS_NUMBER														1

/** Maximum Periodic Advertising Data Length */
#define APP_MAX_LENGTH_PER_ADV_DATA													1024


_attribute_ble_data_retention_	u8  app_advSet_buffer[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_	u8 	app_advData_buffer[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_	u8 	app_scanRspData_buffer[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	u8	testAdvData[APP_MAX_LENGTH_ADV_DATA];
_attribute_ble_data_retention_  u8  testScanRspData[APP_MAX_LENGTH_SCAN_RESPONSE_DATA];

_attribute_ble_data_retention_	u8	app_perdAdvSet_buffer[PERD_ADV_PARAM_LENGTH * APP_PER_ADV_SETS_NUMBER];
_attribute_ble_data_retention_	u8 	app_perdAdvData_buffer[APP_MAX_LENGTH_PER_ADV_DATA * APP_PER_ADV_SETS_NUMBER];

_attribute_ble_data_retention_	ble_sts_t	ble_status = BLE_SUCCESS;


void app_periodic_adv_test(void)
{
	//////////// Extended ADV Initialization  Begin ///////////////////////
	blc_ll_initExtendedAdvertising_module();
	blc_ll_initExtendedAdvSetBuffer(app_advSet_buffer, APP_ADV_SETS_NUMBER);
	blc_ll_initExtendedAdvDataBuffer(app_advData_buffer, APP_MAX_LENGTH_ADV_DATA);
	blc_ll_initExtendedScanRspDataBuffer(app_scanRspData_buffer, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
	u32  my_adv_interval_min = ADV_INTERVAL_200MS;
	u32  my_adv_interval_max = ADV_INTERVAL_200MS;
	// Extended, None_Connectable_None_Scannable undirected, with auxiliary packet
	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_3dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);
	for(int i=0;i<1024;i++){
		testAdvData[i] = i;
	}

	blc_ll_setExtAdvData( ADV_HANDLE0,    31, (u8 *)testAdvData);
//	blc_ll_setExtAdvData( ADV_HANDLE0, 	 200, testAdvData);
//	blc_ll_setExtAdvData( ADV_HANDLE0, 	 400, testAdvData);
//	blc_ll_setExtAdvData( ADV_HANDLE0, 	 600, testAdvData);
//	blc_ll_setExtAdvData( ADV_HANDLE0,  1024, testAdvData);
	blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);
	//////////////// Extended ADV Initialization End ///////////////////////




	//////////// Periodic Adv Initialization  Begin ////////////////////////
	blc_ll_initPeriodicAdvertising_module();
	blc_ll_initPeriodicAdvParamBuffer( app_perdAdvSet_buffer, APP_PER_ADV_SETS_NUMBER);
	blc_ll_initPeriodicAdvDataBuffer(app_perdAdvData_buffer, APP_MAX_LENGTH_PER_ADV_DATA);
	u32  my_per_adv_itvl_min = PERADV_INTERVAL_200MS;
	u32  my_per_adv_itvl_max = PERADV_INTERVAL_200MS;
	blc_ll_setPeriodicAdvParam( ADV_HANDLE0, my_per_adv_itvl_min, my_per_adv_itvl_max, PERD_ADV_PROP_MASK_TX_POWER_INCLUDE);
//	blc_ll_setPeriodicAdvData( ADV_HANDLE0, 300, testAdvData);
	blc_ll_setPeriodicAdvData( ADV_HANDLE0, 100, testAdvData);
	blc_ll_setPeriodicAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0);
	//////////////// Periodic Adv Initialization End ///////////////////////


//	blc_ll_setAuxAdvChnIdxByCustomers(1); //debug chn_2406
//	blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);
}


#endif // end of (FEATURE_TEST_MODE == TEST_PER_ADV)


