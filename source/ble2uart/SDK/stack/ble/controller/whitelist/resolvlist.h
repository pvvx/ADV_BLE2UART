/********************************************************************************************************
 * @file	resolvlist.h
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
#ifndef LL_RESOLVLIST_H_
#define LL_RESOLVLIST_H_


#include <stack/ble/ble_common.h>




/**
 * @brief      This function is used to initialize resolving list && RPA concerned parameters
 * @param[in]  none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
void			blc_ll_resolvListInit(void);

/**
 * @brief      This function is used to reset resolving list
 * @param[in]  none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t		ll_resolvingList_reset(void);


/**
 * @brief      This function is used to add a device to resolving list
 * @param[in]  peerIdAddrType - device mac address type
 * @param[in]  peerIdAddr - device mac address
 * @param[in]  peer_irk - peer IRK pointer
 * @param[in]  local_irk - local IRK pointer
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t		ll_resolvingList_add(u8 peerIdAddrType, u8 *peerIdAddr, u8 *peer_irk, u8 *local_irk);


/**
 * @brief      This function is used to delete a device from resolving list
 * @param[in]  peerIdAddrType - device mac address type
 * @param[in]  peerIdAddr - device mac address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t		ll_resolvingList_delete(u8 peerIdAddrType, u8 *peerIdAddr);


/**
 * @brief      This function is used to enable resolvinglist resolution
 * @param[in]  resolutionEn - 1: enable; 0:disable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t		ll_resolvingList_setAddrResolutionEnable (u8 resolutionEn);


/**
 * @brief      This function is used to get resolving list size
 * @param[out] pointer to size
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  		ll_resolvingList_getSize(u8 *Size);


/**
 * @brief	This function is used to get the current peer Resolvable Private
 * 			Address being used for the corresponding peer Public and Random (static) Identity Address.
 * @param[in]	peerIdAddrType - Peer Identity Address Type
 * @param[in]	peerIdAddr - Peer Identity Address
 * @param[out]  peerResolvableAddr - Peer Resolvable Address
 * @return	Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  		ll_resolvingList_getPeerResolvableAddr (u8 peerIdAddrType, u8* peerIdAddr, u8* peerResolvableAddr);


/**
 * @brief	This function is used to get the current local Resolvable Private
 * 			Address being used for the corresponding peer Identity Address.
 * @param[in]	peerIdAddrType - Peer Identity Address Type
 * @param[in]	peerIdAddr - Peer Identity Address
 * @param[out]  LocalResolvableAddr - Local Resolvable Address
 * @return	Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  		ll_resolvingList_getLocalResolvableAddr(u8 peerIdAddrType, u8* peerIdAddr, u8* LocalResolvableAddr);


/**
 * @brief	This function is used to enable resolution of Resolvable Private Addresses in the Controller.
 * @param[in]	resolutionEn - Address Resolution Enable/Disable
 * @return	Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  		ll_resolvingList_setAddrResolutionEnable (u8 resolutionEn);

/*
 * @brief 	This function is used to set the length of time the controller uses a
 * 			Resolvable Private Address before a new resolvable	private address is
 * 			generated and starts being used. This timeout applies to all addresses
 * 			generated by the controller
*  @param[in]	timeout_s - RPA_Timeout: RPA_Timeout measured in seconds Range: 0x0001 to 0x0E10, Time range: 1 s to 1 hour
 * @return	Status - 0x00: command succeeded; 0x01-0xFF: command failed
 * */
ble_sts_t  		ll_resolvingList_setResolvablePrivateAddrTimer (u16 timeout_s);

/*
 * @brief 	This function is used to allow the Host to specify the privacy mode to
 * 			be used for a given entry on the resolving list.
 * @param[in]	peerIdAddrType - Peer Identity Address Type
 * @param[in]	peerIdAddr - Peer Identity Address
 * @param[in]	privMode - Privacy_Mode: Network/Device Privacy Mode
 * @return	Status - 0x00: command succeeded; 0x01-0xFF: command failed
 * */
ble_sts_t  		ll_resolvingList_setPrivcyMode(u8 peerIdAddrType, u8* peerIdAddr, u8 privMode);


#endif /* LL_RESOLVLIST_H_ */
