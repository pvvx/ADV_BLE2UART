/********************************************************************************************************
 * @file	per_adv_set.c
 *
 * @brief	This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "application/usbstd/usbkeycode.h"
#include "application/keyboard/keyboard.h"
#include "vendor/common/blt_common.h"
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
	blc_ll_initExtAdvDataBuffer(app_advData_buffer, APP_MAX_LENGTH_ADV_DATA);
	blc_ll_initExtScanRspDataBuffer(app_scanRspData_buffer, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
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


