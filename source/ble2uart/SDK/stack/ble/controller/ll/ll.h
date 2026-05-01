/********************************************************************************************************
 * @file    ll.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef LL_H_
#define LL_H_



/**
 * @brief	This callback function type for handling IRQ.
 * @param	None.
 * @return	None.
 */
typedef void (*user_irq_handler_cb_t)(void);
extern user_irq_handler_cb_t usr_irq_handler_cb;




/**
 * @brief Enumeration defining values for enabling or disabling a feature.
 */
typedef enum{
	LL_FEATURE_ENABLE	= 1,
	LL_FEATURE_DISABLE  = 0,
}ll_feature_value_t;


/**
 * @brief      register user irq handler call-back
 * @param      cb     pointer point to function
 * @return
 */
void blc_ll_register_user_irq_handler_cb(user_irq_handler_cb_t cb);

/**
 * @brief   irq_handler for BLE stack, process system tick interrupt and RF interrupt
 * @param   none
 * @return  none
 */
void blc_sdk_irq_handler(void);

/**
 * @brief   main_loop for BLE stack, process data and event
 * @param   none
 * @return  none
 */
void blc_sdk_main_loop(void);


/**
 * @brief      for user to initialize MCU
 * @param      none
 * @return     none
 */
void blc_ll_initBasicMCU(void);


/**
 * @brief      for user to initialize link layer Standby state
 * @param      none
 * @return     none
 */
void blc_ll_initStandby_module(u8 *public_adr);


/**
 * @brief      this function is used to read MAC address
 * @param[in]  *addr -  The address where the read value(MAC address) prepare to write.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_readBDAddr(u8 *addr);


/**
 * @brief      this function is used to set the LE Random Device Address in the Controller
 * @param[in]  *randomAddr -  Random Device Address
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_setRandomAddr(u8 *randomAddr);


/**
 * @brief      this function is used by the Host to specify a channel classification based on its local information,
 *             only the master role is valid.
 * @param[in]  bit_number - Bit position in the FeatureSet.
 * @param[in]  bit_value - refer to the struct "ll_feature_value_t".
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t	blc_hci_le_setHostFeature(u8 bit_number, ll_feature_value_t bit_value);



/**
 * @brief      this function is used by the Host to specify a channel classification based on its local information,
 *             only the ACL Central role is valid.
 * @param[in]  *map - channel map
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_setHostChannel(u8 *chnMap);



ble_sts_t 	blc_hci_le_getRemoteSupportedFeatures(u16 connHandle);
ble_sts_t 	blc_hci_le_readChannelMap(u16 connHandle, u8 *returnChannelMap);

/**
 * @brief      this function checks whether the Bluetooth stack task is IDLE, not recommended
 * @param      none
 * @return     status, 0:  idle
 *                     1:  task
 */
u32 blc_ll_checkBleTaskIsIdle(void);


/**
 * @brief      this function checks whether the Bluetooth stack task is IDLE
 * @param      none
 * @return     bool, 0:  ble task running
 *                   1:  idle
 */
bool blc_ll_isBleTaskIdle(void);


/**
 * @brief 		this function is used to solving issue that BLE connection RF IRQ affected by Flash writing status duration
 * 				by finding idle timing to write safely.
 *              If MCU do not support multiple priority IRQ, Flash write status duration influencing BLE RF IRQ, then lead to BLE data error
 * 			    attention: it's for Flash lock & unlock in BLE connection state
 * @param[in]  	type	- the type of status.8 bit or 16 bit.
 * @param[in]  	data	- the value of status.
 * @return 		none.
 */
void blc_ll_acl_write_flash_status(flash_status_typedef_e type , unsigned short data);

/**
 * @brief      This function is used to set some other channel to replace advertising chn37/38/39 in ADV state and SCAN state.
 * @param[in]  chn0 - channel to replace channel 37
 * @param[in]  chn1 - channel to replace channel 38
 * @param[in]  chn2 - channel to replace channel 39
 * @return     none
 */
void 		blc_ll_setCustomizedPrimaryChannel (u8 chn0, u8 chn1, u8 chn2);

#endif /* LL_H_ */
