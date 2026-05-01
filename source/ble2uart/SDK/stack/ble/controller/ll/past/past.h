/********************************************************************************************************
 * @file    past.h
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
#ifndef PAST_H_
#define PAST_H_

#include "stack/ble/hci/hci_cmd.h"

/**
 * @brief      This function is used to initialize the PAST module
 * @param[in]  none
 * @return     none
 */
void        blc_ll_initPAST_module(void);


/**
 * @brief      This function is used to instruct the Controller to send synchronization information
 *             about the periodic advertising train identified by the Sync_Handle parameter to a connected
 *             device.
 * @param[in]  connHandle - Connection_Handle Range: 0x0000 to 0x0EFF
 * @param[in]  serviceData - A value provided by the Host
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_periodicAdvSyncTransfer(u16 connHandle, u16 serviceData, u16  syncHandle);


/**
 * @brief      This function is used to to instruct the Controller to send synchronization information
 *             about the periodic advertising in an advertising set to a connected device.
 * @param[in]  connHandle - Connection_Handle Range: 0x0000 to 0x0EFF
 * @param[in]  serviceData - A value provided by the Host
 * @param[in]  advHandle - Used to identify an advertising set: 0x00C0xEF
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_periodicAdvSetInfoTransfer(u16   connHandle, u16     serviceData, u8     advHandle);


/**
 * @brief      This function is used to specify how the Controller will process periodic advertising
 *             synchronization information received from the device identified by the Connection_Handle
 *             parameter (the "transfer mode")
 * @param[in]  connHandle - Connection_Handle Range: 0x0000 to 0x0EFF
 * @param[in]  mode -
 * @param[in]  skip - The number of periodic advertising packets that can be skipped after a successful receive
 *                    Range: 0x0000 to 0x01F3
 * @param[in]  syncTimeout - Synchronization timeout for the periodic advertising train Range: 0x000A to 0x4000
 *                           Time = N*10 ms: Time Range: 100 ms to 163.84 s
 * @param[in]  cteType -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_setPeriodicAdvSyncTransferParams(u16 connHandle, u8 mode, u16 skip, u16 syncTimeout, u8 cteType);


/**
 * @brief      This function is used to specify the initial value for the mode, skip, timeout, and
 *             Constant Tone Extension type (set by the HCI_LE_Set_Periodic_Advertising_Sync_Transfer_Parameters command;
 *             see Section 7.8.91) to be used for all subsequent connections over the LE transport.
 * @param[in]  mode -
 * @param[in]  skip - The number of periodic advertising packets that can be skipped after a successful receive
 *                    Range: 0x0000 to 0x01F3
 * @param[in]  syncTimeout - Synchronization timeout for the periodic advertising train Range: 0x000A to 0x4000
 *                           Time = N*10 ms: Time Range: 100 ms to 163.84 s
 * @param[in]  cteType -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_setDftPeriodicAdvSyncTransferParams(u8 mode, u16 skip, u16 syncTimeout, u8 cteType);

/**
 * @brief      This function is used to enable or disable reports for the periodic advertising train
 *             identified by the Sync_Handle parameter
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @param[in]  enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_periodicAdvertisingReceiveEn(hci_le_setPeriodicAdvReceiveEnCmdParams_t *cmdPara);


/**
 * @brief      This function is used to instruct the Controller to send synchronization information
 *             about the periodic advertising train identified by the Sync_Handle parameter to a connected
 *             device.
 * @param[in]  *cmdPara - refer to 'hci_le_pastCmdParams_t'
 * @param[out] *retPara - refer to 'hci_le_pastRetParams_t'
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_periodicAdvSyncTransfer(hci_le_pastCmdParams_t *cmdPara, hci_le_pastRetParams_t *retPara);


/**
 * @brief      This function is used to to instruct the Controller to send synchronization information
 *             about the periodic advertising in an advertising set to a connected device.
 * @param[in]  *cmdPara - refer to 'hci_le_paSetInfoTransferCmdParams_t'
 * @param[out] *retPara - refer to 'hci_le_paSetInfoTransferRetParams_t'
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_periodicAdvSetInfoTransfer(hci_le_paSetInfoTransferCmdParams_t *cmdPara, hci_le_paSetInfoTransferRetParams_t *retPara);

/**
 * @brief      This function is used to specify how the Controller will process periodic advertising
 *             synchronization information received from the device identified by the Connection_Handle
 *             parameter (the "transfer mode")
 * @param[in]  *cmdPara - refer to 'hci_le_pastParamsCmdParams_t'
 * @param[out] *retPara - refer to 'hci_le_pastParamsRetParams_t'
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_setPeriodicAdvSyncTransferParams(hci_le_pastParamsCmdParams_t *cmdPara, hci_le_pastParamsRetParams_t *retPara);

/**
 * @brief      This function is used to specify the initial value for the mode, skip, timeout, and
 *             Constant Tone Extension type (set by the HCI_LE_Set_Periodic_Advertising_Sync_Transfer_Parameters command;
 *             see Section 7.8.91) to be used for all subsequent connections over the LE transport.
 * @param[in]  *cmdPara - refer to 'hci_le_dftPastParamsCmdParams_t'
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_setDftPeriodicAdvSyncTransferParams(hci_le_dftPastParamsCmdParams_t *cmdPara);

#endif /* LL_PC_H_ */
