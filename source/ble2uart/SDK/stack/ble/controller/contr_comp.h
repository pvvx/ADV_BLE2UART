/********************************************************************************************************
 * @file    contr_comp.h
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

#ifndef CONTR_COMP_H_
#define CONTR_COMP_H_

/* for some API name in old version SDK compatible with new version SDK */

#define blc_ll_registerTelinkControllerEventCallback    blc_contr_registerTelinkControllerEventCallback

#define	ll_resolvingList_reset							blc_ll_clearResolvingList
#define	ll_resolvingList_add							blc_ll_addDeviceToResolvingList
#define	ll_resolvingList_setAddrResolutionEnable		blc_ll_setAddressResolutionEnable

#define	ll_whiteList_reset								blc_ll_clearWhiteList
#define	ll_whiteList_add								blc_ll_addDeviceToWhiteList
#define	ll_whiteList_delete								blc_ll_removeDeviceFromWhiteList


#define blc_ll_setAdvCustomedChannel					blc_ll_setCustomizedPrimaryChannel
#define blc_ll_initExtAdvDataBuffer                     blc_ll_initExtendedAdvDataBuffer
#define blc_ll_initExtScanRspDataBuffer                 blc_ll_initExtendedScanRspDataBuffer

#define	blc_ll_initInitiating_module                    blc_ll_initLegacyInitiating_module
#define	blc_ll_initLegacyAdvertising_local_rpa_module 	blc_ll_initLegacyAdvertising_module
#define	blc_ll_initLegacyInitiating_local_rpa_module 	blc_ll_initLegacyInitiating_module
#define	blc_ll_initLegacyScanning_local_rpa_module 		blc_ll_initLegacyScanning_module


#define	blc_ll_setAclMasterConnectionInterval		    blc_ll_setAclCentralBaseConnectionInterval


#define blc_controller_check_appBufferInitialization                            blc_contr_checkControllerInitialization

#define blc_ll_addDeivceToPeriodicAdvertiserList                            blc_ll_addDeviceToPeriodicAdvertiserList
#define blc_ll_removeDeivceFromPeriodicAdvertiserList                   blc_ll_removeDeviceFromPeriodicAdvertiserList
#define blc_ll_registerTelinkControllerEventCallback                        blc_contr_registerTelinkControllerEventCallback



#define blt_ll_initLegacyInitiating_module              blc_ll_initLegacyInitiating_module
#define blc_ll_initAclMasterRole_module                 blc_ll_initAclCentralRole_module
#define blc_ll_initAclConnMasterTxFifo                  blc_ll_initAclCentralTxFifo
#define blc_ll_initAclSlaveRole_module                  blc_ll_initAclPeriphrRole_module
#define blc_ll_initAclConnSlaveTxFifo                   blc_ll_initAclPeriphrTxFifo
#define blc_contr_registerTelinkControllerEventCallback blc_ll_registerTelinkControllerEventCallback


#define PM_SLEEP_ACL_SLAVE                              PM_SLEEP_ACL_PERIPHR
#define LL_RSSI_SNIFFER_FEATURE_ENABLE                  LL_RSSI_SNIFFER_MODE_ENABLE


#endif /* CONTR_COMP_H_ */
