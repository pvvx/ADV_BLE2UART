/********************************************************************************************************
 * @file	ll.h
 *
 * @brief	This is the header file for BLE SDK
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
#ifndef LL_H_
#define LL_H_



/**
 * @brief	Telink defined LinkLayer Event Callback
 */
typedef void (*blt_event_callback_t)(u8 e, u8 *p, int n);




typedef enum{
	BLT_EV_FLAG_ADV_DURATION_TIMEOUT	=	0,
	BLT_EV_FLAG_RX_DATA_ABANDOM,
	BLT_EV_FLAG_GPIO_EARLY_WAKEUP,
	BLT_EV_FLAG_SLEEP_ENTER,
	BLT_EV_FLAG_SUSPEND_EXIT,
	BLT_EV_FLAG_LL_REJECT_IND,
	BLT_EV_MAX_NUM,
}blt_ev_flag_t;







typedef enum{
	LL_FEATURE_ENABLE	= 1,
	LL_FEATURE_DISABLE  = 0,
}ll_feature_value_t;

typedef void (*user_irq_handler_cb_t)(void);
extern  user_irq_handler_cb_t  usr_irq_handler_cb;

/**
 * @brief	Telink defined LinkLayer Event callBack
 * @param[in]	e - event number, must use element of "blt_ev_flag_t"
 * @param[in]	p - callBack function
 * @return	none
 */
void 		blc_ll_registerTelinkControllerEventCallback (u8 e, blt_event_callback_t p);

/**
 * @brief      register user irq handler call-back
 * @param      cb     pointer point to function
 * @return
 */
void blc_ll_register_user_irq_handler_cb(user_irq_handler_cb_t cb);

/**
 * @brief	irq_handler for BLE stack, process system tick interrupt and RF interrupt
 * @param	none
 * @return	none
 */
void 		blc_sdk_irq_handler(void);

/**
 * @brief   main_loop for BLE stack, process data and event
 * @param	none
 * @return	none
 */
void 		blc_sdk_main_loop (void);



/**
 * @brief      for user to initialize MCU
 * @param	   none
 * @return     none
 */
void 		blc_ll_initBasicMCU (void);



/**
 * @brief      for user to initialize link layer Standby state
 * @param	   none
 * @return     none
 */
void 		blc_ll_initStandby_module (u8 *public_adr);


/**
 * @brief      this function is used to read MAC address
 * @param[in]  *addr -  The address where the read value(MAC address) prepare to write.
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t   blc_ll_readBDAddr(u8 *addr);


/**
 * @brief      this function is used to set the LE Random Device Address in the Controller
 * @param[in]  *randomAddr -  Random Device Address
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_setRandomAddr(u8 *randomAddr);


/**
 * @brief      This function is used to check if the address's type is public
 * @param[in]  *addr -  The address need to check.
 * @return     bool, 0x00: no public, 0x01: Public
 */
bool 		blc_ll_isValidPublicAddr(u8* addr);


/**
 * @brief      This function is used to check if the address's type is random
 * @param[in]  *addr -  The address need to check.
 * @return     bool, 0x00: no random, 0x01: random
 */
bool 		blc_ll_isValidRandomAddr(u8* addr);


/**
 * @brief      This function is used to check if owner's address type is valid
 * @param[in]  ownAddrType -  Owner address type.
 * @param[in]  randomAddr -  If Owner's address type is Random, input Random address.
 * @return     bool, 0x00: invalid, 0x01: valid
 */
bool 		blc_ll_isValidOwnAddrByAddrType(u8 ownAddrType, u8* randomAddr);


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
 * @brief      this function is used check if any controller buffer initialized by application incorrect.
 * 			   attention: this function must be called at the end of BLE LinkLayer Initialization.
 * @param	   none
 * @return     status, 0x00:  succeed, no buffer error
 * 					   other: buffer error code
 */
ble_sts_t	blc_controller_check_appBufferInitialization(void);



/**
 * @brief      this function is used by the Host to specify a channel classification based on its local information,
 *             only the master role is valid.
 * @param[in]  *map - channel map
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t 	blc_ll_setHostChannel(u8 * chnMap);


/**
 * @brief      this function is used to reset module of all.
 * @param	   none
 * @return     status, 0x00:  succeed, no buffer error
 * 					   other: buffer error code
 */
ble_sts_t  	blc_hci_reset(void);
ble_sts_t 	blc_hci_le_getRemoteSupportedFeatures(u16 connHandle);
ble_sts_t 	blc_hci_le_readChannelMap(u16 connHandle, u8 *returnChannelMap);

/**
 * @brief      this function checks whether the Bluetooth stack task is IDLE
 * @param	   none
 * @return     status, 0:  idle
 *                     1:  task
 */
u32 blc_ll_checkBleTaskIsIdle(void);
#endif /* LL_H_ */
