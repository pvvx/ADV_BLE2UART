/********************************************************************************************************
 * @file    PAwR_sync.h
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
#ifndef STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_SYNC_H_
#define STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_SYNC_H_

/**
 * @brief      Initialize the PAwR synchronization module with the specified number of PAwR synchronization.
 * @param[in]  num_pawr_sync - The number of PAwR synchronization to be initialized.
 * @return     status, 0x00: succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initPAwRsync_module(int num_pawr_sync);

/**
 * @brief      Initialize the response data buffer for PAwR synchronization.
 * @param[in]  pdaRspData - Pointer to the response data buffer.
 * @param[in]  maxLen_pdaRspData - Maximum length of the response data buffer.
 * @return     status, 0x00: succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initPAwRsync_rspDataBuffer(u8 *pdaRspData, int maxLen_pdaRspData);

/**
 * @brief      Set the parameters for PAwR synchronization subevents.
 * @param[in]  sync_handle - The synchronization handle.
 * @param[in]  pda_prop - The properties of the periodic advertisement.
 * @param[in]  num_subevent - The number of subevents.
 * @param[in]  pSubevent - Pointer to the array of subevents.
 * @return     status, 0x00: succeed
 *                     other: failed
 */
ble_sts_t blc_hci_le_setPeriodicSyncSubevent(u16 sync_handle, perd_adv_prop_t pda_prop, u8 num_subevent, u8* pSubevent);

/**
 * @brief      Set the response data for the PAwR synchronization.
 * @param[in]  sync_handle - The synchronization handle.
 * @param[in]  req_pdaEvtCnt - The requested periodic advertisement event count.
 * @param[in]  req_subEvtCnt - The requested subevent count.
 * @param[in]  rsp_subEvtCnt - The response subevent count.
 * @param[in]  rsp_slotIdx - The response slot index.
 * @param[in]  rspDataLen - The length of the response data.
 * @param[in]  pRspData - Pointer to the response data buffer.
 * @return     status, 0x00: succeed
 *                     other: failed
 */
ble_sts_t blc_hci_le_setPAwRsync_rspData(u16 sync_handle, u16 req_pdaEvtCnt, u8 req_subEvtCnt,
                                         u8 rsp_subEvtCnt, u8 rsp_slotIdx, u8 rspDataLen, u8* pRspData);


#endif
