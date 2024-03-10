/********************************************************************************************************
 * @file	prd_adv.h
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

#ifndef STACK_BLE_CONTROLLER_LL_PRDADV_PRD_ADV_H_
#define STACK_BLE_CONTROLLER_LL_PRDADV_PRD_ADV_H_








/* maximum number of periodic advertising this SDK can support */
#define			PERIODIC_ADV_NUMBER_MAX							2


#define 		PERD_ADV_PARAM_LENGTH						(460)   // Note: user can't modify this value,and this value must 4 byte aligned



/**
 * @brief      for user to initialize periodic advertising module
 * @param	   none
 * @return     none
 */
void 		blc_ll_initPeriodicAdvertising_module(void);


/**
 * @brief      for user to allocate periodic advertising parameters buffer
 * 			   notice that: this API must used after "blc_ll_initPeriodicAdvertising_module",
 * 			                and before any other periodic ADV initialization APIs.
 * @param[in]  pBuff - global buffer allocated by application layer.
 * @param[in]  num_periodic_adv - number of application adv_sets
 * @return     Status - 0x00: command succeeded;
 * 						0x12: num_periodic_adv exceed maximum number of supported periodic advertising.
 */
ble_sts_t	blc_ll_initPeriodicAdvParamBuffer(u8 *pBuff, int num_periodic_adv);

/**
 * @brief      This function is used by the Host to set the parameters for periodic advertising.
 * @param[in]  adv_handle - - Used to identify a periodic advertising train
 * @param[in]  advInter_min - Periodic_Advertising_Interval_Min(Range: 0x0006 to 0xFFFF, Time = N * 1.25 ms Time Range: 7.5 ms to 81.91875 s)
 * @param[in]  advInter_max - Periodic_Advertising_Interval_Max
 * @param[in]  property - Periodic_Advertising_Properties
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_ll_setPeriodicAdvParam(adv_handle_t adv_handle, u16 advInter_min, u16 advInter_max, perd_adv_prop_t property);

/**
 * @brief      initialize Periodic Advertising Data buffer for all adv_set
 * @param[in]  perdAdvData -
 * @param[in]  max_len_perdAdvData -
 * @return     none
 */
void 		blc_ll_initPeriodicAdvDataBuffer(u8 *perdAdvData, int max_len_perdAdvData);

/**
 * @brief      initialize Periodic Advertising Data buffer for specific adv_set.
 *			   notice that:
 * @param[in]  adv_handle - equal to adv_set index here. So if using single adv_set, adv_handle can only be 0x00;
 *                          if using multiple adv_set, for example 3, adv_handle can be 0x00,0x01 and 0x02
 * @param[in]  perdAdvData -
 * @param[in]  max_len_perdAdvData -
 * @return     Status - 0x00: succeed.
 * 						0x12: adv_handle out of range.
 */
ble_sts_t 	blc_ll_initPeriodicAdvDataBuffer_by_advHandle(u8 adv_handle, u8 *perdAdvData, int max_len_perdAdvData);

/**
 * @brief      This function is used to set the data used in periodic advertising PDUs.
 * @param[in]  adv_handle - - equal to adv_set index here.
 * @param[in]  advData_len -
 * @param[in]  *advdata -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  	blc_ll_setPeriodicAdvData(adv_handle_t adv_handle, u16 advData_len, u8 *advdata);



/**
 * @brief      This function is used to enable or disable the periodic advertising for the advertising
 *			   set specified by the Advertising_Handle parameter
 * @param[in]  adv_enable - Advertising_Enable
 * @param[in]  adv_handle - Used to identify an advertising set.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_setPeriodicAdvEnable(adv_en_t per_adv_enable, adv_handle_t adv_handle);








#endif /* STACK_BLE_CONTROLLER_LL_PRDADV_PRD_ADV_H_ */
