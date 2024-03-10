/********************************************************************************************************
 * @file	pda_sync.h
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

#ifndef STACK_BLE_CONTROLLER_LL_PRDADV_PDA_SYNC_H_
#define STACK_BLE_CONTROLLER_LL_PRDADV_PDA_SYNC_H_


/**
 * @brief      for user to initialize periodic advertising synchronization module
 * @param	   none
 * @return     none
 */
void 		blc_ll_initPeriodicAdvertisingSynchronization_module(void);


/**
 * @brief      this function is used to synchronize with a periodic advertising train from an advertiser and begin
			   receiving periodic advertising packets.
 * @param[in]  options - used to determine whether the Periodic Advertiser List is used and whether HCI_Periodic_Advertising_Report events
 *  					 for this periodic advertising train are initially enabled or disabled.
 * @param[in]  adv_sid - Advertising SID subfield in the ADI field used to identify the Periodic Advertising
 * @param[in]  adv_adrType - Advertiser Address Type
 * @param[in]  *adv_addr - Advertiser Address
 * @param[in]  skip - The maximum number of periodic advertising events that can be skipped after a successful receive
 * @param[in]  sync_timeout - Synchronization timeout for the periodic advertising train
 * @param[in]  sync_cte_type -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_periodicAdvertisingCreateSync ( option_msk_t options, u8 adv_sid, u8 adv_adrType, u8 *adv_addr, u16 skip, sync_tm_t sync_timeout, u8 sync_cte_type);

/**
 * @brief      This function is used to cancel the periodic advertising create sync command while it is pending.
 * @param      none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_periodicAdvertisingCreateSyncCancel (void);



/**
 * @brief      This function is used to stop reception of the periodic advertising train identified
 * 			   by the Sync_Handle parameter.
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_periodicAdvertisingTerminateSync (u16 sync_handle);

/**
 * @brief      This function is used to add an entry, consisting of a single device address and SID,
 * 			   to the Periodic Advertiser list stored in the Controller.
 * @param[in]  adv_adrType - Advertiser_Address_Type
 * @param[in]  *adv_addr - Advertiser_Address
 * @param[in]  adv_sid - Advertising_SID
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_addDeivceToPeriodicAdvertiserList (u8 adv_adrType, u8 *adv_addr, u8 adv_sid);

/**
 * @brief      This function is used to remove one entry from the list of Periodic Advertisers
 * 			   stored in the Controller.
 * @param[in]  adv_adrType - Advertiser_Address_Type
 * @param[in]  *adv_addr - Advertiser_Address
 * @param[in]  adv_sid - Advertising_SID
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_removeDeivceFromPeriodicAdvertiserList (u8 adv_adrType, u8 *adv_addr, u8 adv_sid);

/**
 * @brief      This function is used to remove all entries from the list of Periodic Advertisers.
 * @param	   none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_clearPeriodicAdvertiserList (void);

/**
 * @brief      This function is used to read the total number of Periodic Advertiser list entries.
 * @param[in]  none
 * @param[out]  *perdAdvListSize - point to Periodic_Advertiser_List_Size
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_readPeriodicAdvertiserListSize (u8 *perdAdvListSize);

ble_sts_t 	blc_hci_le_periodic_advertising_create_sync(hci_le_periodicAdvCreateSync_cmdParam_t* cmdPara);




#endif /* STACK_BLE_CONTROLLER_LL_PRDADV_PDA_SYNC_H_ */
