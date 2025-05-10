/********************************************************************************************************
 * @file    app_legScan.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "app_config.h"
#include "app.h"
#include "app_buffer.h"


#if (FEATURE_TEST_MODE == TEST_PRIVACY_CENTRAL)


void app_configLegacyScanParam(void)
{
	blc_ll_setScanEnable (BLC_SCAN_DISABLE, DUP_FILTER_DISABLE);
	u8 local_addr_type = OWN_ADDRESS_PUBLIC;
	u8 bond_number = blc_smp_param_getCurrentBondingDeviceNumber(1, 0);
	if(bond_number != 0)	//No bondind device
	{
		local_addr_type = OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC;
		tlkapi_printf(APP_LOG_EN,"bond number addr not 0, is %x\n", bond_number);

		smp_param_save_t  bondInfo;
		blc_smp_loadBondingInfoFromFlashByIndex(1, 0, bond_number-1, &bondInfo);  //get the latest bonding device (index: bond_number-1 )
		tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] bondInfo.peer_irk", bondInfo.peer_irk, 16);
		tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] bondInfo.local_irk", bondInfo.local_irk, 16);
		if(!blc_app_isIrkValid(bondInfo.peer_irk))
		{
			memset(bondInfo.peer_irk, 0, 16);
		}
		if(!blc_app_isIrkValid(bondInfo.local_irk))
		{
			local_addr_type = OWN_ADDRESS_PUBLIC;
			memset(bondInfo.local_irk, 0, 16);
		}
		//add bonding message to resolve list
		blc_ll_addDeviceToResolvingList(bondInfo.peer_id_adrType,bondInfo.peer_id_addr,bondInfo.peer_irk,bondInfo.local_irk);



		blc_ll_setAddressResolutionEnable(1);
	}
	//should set scan mode again to scan slave ADV packet
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_50MS,
			local_addr_type, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
}

#endif //end of (FEATURE_TEST_MODE == ...)



