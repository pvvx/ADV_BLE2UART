/********************************************************************************************************
 * @file    controller.h
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
#ifndef CONTROLLER_H_
#define CONTROLLER_H_



/* 1: multiple connection SDK; 0 : single connection SDK */
#define	BLE_MULTIPLE_CONNECTION_ENABLE								1



/**
 * @brief	Telink defined Controller Event Callback Declaration
 */
typedef void (*blt_event_callback_t)(u8 e, u8 *p, int n);



/**
 * @brief	Telink defined Controller Event Type
 */
typedef enum{
	BLT_EV_FLAG_ADV_DURATION_TIMEOUT	=	0,
	BLT_EV_FLAG_RX_DATA_ABANDOM,
	BLT_EV_FLAG_GPIO_EARLY_WAKEUP,
	BLT_EV_FLAG_SLEEP_ENTER,
	BLT_EV_FLAG_SUSPEND_EXIT,
	BLT_EV_FLAG_KEY_MISSING,

	BLT_EV_MAX_NUM,
}blt_ev_flag_t;



/**
 * @brief		this function is used to register Controller Event Callback function
 * @param[in]	e - event type when this function is triggered by Controller event
 * @param[in]	p - event callback function
 * @return		none
 */
void 		blc_ll_registerTelinkControllerEventCallback (u8 e, blt_event_callback_t p);


/**
 * @brief      this function is used check if any controller initialization incorrect.
 * 			   attention: this function must be called after all controller Initialization finished.
 * @param	   none
 * @return     status - 0x00:  succeed, no error
 * 			  		    other: error code
 */
init_err_t	blc_contr_checkControllerInitialization(void);



/**
 * @brief      this function is used to set Bluetooth core specification version in controller
 * @param[in]  version - core specification version
 * @return     none
 */
void 		blc_contr_setBluetoothVersion(core_version_t version);



/**
 * @brief      this function is used to reset BLE controller.
 * @param	   none
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t  	blc_hci_reset(void);


#endif /* CONTROLLER_H_ */
