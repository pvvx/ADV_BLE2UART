/********************************************************************************************************
 * @file    smp_storage.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef SMP_STORAGE_H_
#define SMP_STORAGE_H_




/*
 *  Address resolution is not supported by default. After pairing and binding, we need to obtain the central Address Resolution
 *  feature value of the opposite end to determine whether the opposite end supports the address resolution function, and write
 *  the result to smp_bonding_flg. Currently, we leave it to the user to obtain this feature.
 */
#define 	IS_PEER_ADDR_RES_SUPPORT(peerAddrResSuppFlg)	(!(peerAddrResSuppFlg & BIT(7)))


/**
 * @brief   strategy for how to deal with new paring device when bonding device number reach maximum number(set by API "blc_smp_setBondingDeviceMaxNumber")
 */
typedef enum {
	NEW_DEVICE_OVERWRITE_OLD_WITH_PAIRING_ORDER = 0,

	NEW_DEVICE_REJECT_WHEN_PER_MAX_BONDING_NUM	= 4,		//When peripheral reach it's max bonding centrals, it reject new pair req.
//	NEW_DEVICE_REJECT_WHEN_CEN_MAX_BONDING_NUM	= 5,		//Not supported now.
}dev_exceed_max_strategy_t;

/*
 * @brief   smp parameter need save to flash.
 */
typedef struct {
	//0x00
	u8		flag;
	u8		role_dev_idx;  //[7]:1 for master, 0 for slave;   [2:0] slave device index

	// peer_addr_type & peer_addr must be together(SiHui 20200916), cause using flash read packed "type&address" in code
	u8		peer_addr_type;  //address used in link layer connection
	u8		peer_addr[6];

	u8		peer_id_adrType; //peer identity address information in key distribution, used to identify
	u8		peer_id_addr[6];

	//0x10
	u8 		local_peer_ltk[16];   //slave: local_ltk; master: peer_ltk

	//0x20
	u8 		encrypt_key_size;
	u8		local_id_adrType;
	u8		local_id_addr[6];

	u8		random[8];  //8


	//0x30
	u8		peer_irk[16];

	//0x40
	u8		local_irk[16];        // local_csrk can be generated based on this key, to save flash area (delete this note at last, customers can not see it)

	//0x50
	u16 	ediv;       //2
	u8		rsvd[14];	//14  peer_csrk info address if needed(delete this note at last, customers can not see it)
}smp_param_save_t;




/**
 * @brief      This function is used to configure the bonding storage address and size.
 *   		   attention: If this API is used, must be called before "blc_smp_smpParamInit" when initialization !!!
 * @param[in]  address - SMP bonding storage start address.
 * @param[in]  size_byte - SMP bonding storage size(e.g.: 2*4096).
 * @return     none.
 */
void 			blc_smp_configPairingSecurityInfoStorageAddressAndSize (int address, int size_byte);  //address and size must be 4K aligned


/**
 * @brief      This function is used to configure the number of master and slave devices that can be bound.
 *   		   attention: If this API is used, must be called before "blc_smp_smpParamInit"
 *  						and after "blc_gap_init" when initialization !!!
 * @param[in]  master_max_bonNum - The number of master devices that can be bound.
 * @param[in]  slave_max_bondNum - The number of slave devices that can be bound.
 * @return     ble_sts_t.
 */
ble_sts_t 		blc_smp_setBondingDeviceMaxNumber ( u8 master_max_bonNum, int slave_max_bondNum);


/**
 * @brief      This function is used to get the starting address of the current binding information storage area.
 * @param[in]  none.
 * @return     The starting address of the current binding information storage area.
 */
u32 			blc_smp_getBondingInfoCurStartAddr(void);


//Search
// This API is for master only, to search if current slave device is already paired with master
/**
 * @brief      This function is used to obtain binding information according to the slave's address and address type.
 * @param[in]  peer_addr_type - Address type.
 * @param[in]  peer_addr - Address.
 * @return     0: Failed to get binding information;
 *             others: FLASH address of the information area.
 */
u32 			blc_smp_searchBondingSlaveDevice_by_PeerMacAddress( u8 peer_addr_type, u8* peer_addr);

//Delete
/**
 * @brief      This function is used to delete binding information according to the peer device address and device address type.
 * @param[in]  peer_addr_type - Address type.
 * @param[in]  peer_addr - Address.
 * @return     0: Failed to delete binding information;
 *             others: FLASH address of the deleted information area.
 */
int				blc_smp_deleteBondingSlaveInfo_by_PeerMacAddress(u8 peer_addr_type, u8* peer_addr);


/**
 * @brief      This function is used to configure the bonding strategy.
 * @param[in]  strategy - The strategy. Refer to the structure 'dev_exceed_max_strategy_t'.
 * @return     none.
 */
void			blc_smp_setDevExceedMaxStrategy(dev_exceed_max_strategy_t strategy);


/**
 * @brief      This function is used to clear all binding information stored in the local FLASH.
 * @param[in]  none.
 * @return     none.
 */
void			blc_smp_eraseAllBondingInfo(void);


/**
 * @brief      This function is used to check whether the bound storage area reaches the almost full warning threshold.
 * @param[in]  none.
 * @return     0: The warning threshold is not reached.
 *             1: The warning threshold is reached..
 */
bool 			blc_smp_isBondingInfoStorageLowAlarmed(void);


/**
 * @brief      This function is used to load bonding information according to the peer device address and device address type.
 * @param[in]  isMaster - Is it a Master role: 0: slave role, others: master role.
 * @param[in]  slaveDevIdx - Address.
 * @param[in]  addr_type - Address type.
 * @param[in]  addr - Address.
 * @param[out] smp_param_load - bonding information.
 * @return     0: Failed to find the binding information; others: FLASH address of the bonding information area.
 */
u32				blc_smp_loadBondingInfoByAddr(u8 isMaster, u8 slaveDevIdx, u8 addr_type, u8* addr, smp_param_save_t* smp_param_load);


/**
 * @brief      This function is used to get the bonding information numbers.
 * @param[in]  isMaster - Is it a Master role: 0: slave role, others: master role.
 * @param[in]  slaveDevIdx - Address.
 * @return     0: The number of bound devices is 0; others: Number of bound devices.
 */
u8				blc_smp_param_getCurrentBondingDeviceNumber(u8 isMasterRole, u8 slaveDevIdx);


/**
 * @brief      This function is used to load bonding information according to the index.
 * @param[in]  isMaster - Is it a Master role: 0: slave role, others: master role.
 * @param[in]  slaveDevIdx - Address.
 * @param[in]  index - bonding index.
 * @param[out] smp_param_load - bonding information.
 * @return     0: Failed to find the binding information; others: FLASH address of the bonding information area.
 */
u32				blc_smp_loadBondingInfoFromFlashByIndex(u8 isMaster, u8 slaveDevIdx, u8 index, smp_param_save_t* smp_param_load);


/**
 * @brief      This function is used to set bonding information as RPA supported or not.
 * @param[in]  flash_addr - Bonding information address.
 * @param[in]  support - 1:support RPA; 0: not support.
 * @return     0: Failed to set RPA support;
 *             others: FLASH address of the bonding information area.
 */
int				blc_smp_setPeerAddrResSupportFlg(u32 flash_addr, u8 support);


#endif /* SMP_STORAGE_H_ */
