/********************************************************************************************************
 * @file    ble_flash.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2020
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
#ifndef BLE_FLASH_H_
#define BLE_FLASH_H_

#include "drivers.h"


/**
 * @brief	Flash initialization log enable. Default disable, user can enable it in app_confog.h
 */
#ifndef		APP_FLASH_INIT_LOG_EN
#define		APP_FLASH_INIT_LOG_EN								0
#endif



/**
 * @brief	Flash using area default Configuration, user can change some of them in app_config.h
 * 			CFG_ADR_MAC:  		  BLE MAC address stored in flash, can not change this value
 * 			CFG_ADR_CALIBRATION:  some calibration data stored in flash, can not change this value
 * 			FLASH_ADR_SMP_PAIRING & FLASH_SMP_PAIRING_MAX_SIZE:
 * 									If Peripheral or Central SMP enable, use 16K flash for SMP pairing information storage.
 * 									First 8K is for normal use, second 8K is a backup to guarantee SMP information never lose.
 * 									use API blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE)
 * 									to set the two value.
 * 			FLASH_SDP_ATT_ADDRESS & FLASH_SDP_ATT_MAX_SIZE
 * 									If ACL central role use service discovery, use this flash area to store some critical information of peer GATT server.
 */

#ifndef FLASH_SMP_PAIRING_MAX_SIZE
#define FLASH_SMP_PAIRING_MAX_SIZE         						(2*4096)
#endif

#ifndef FLASH_SDP_ATT_MAX_SIZE
#define FLASH_SDP_ATT_MAX_SIZE         							(2*4096)   //8K flash for ATT HANDLE storage
#endif																	   //if LE Audio used, additional internal backup 8K flash (total 16K)



//////////////////////////// Flash  Address Configuration ///////////////////////////////

/**************************** 512 K Flash *****************************/
#ifndef	CFG_ADR_MAC_512K_FLASH
#define	CFG_ADR_MAC_512K_FLASH									0x76000
#endif

#ifndef	CFG_ADR_CALIBRATION_512K_FLASH
#define	CFG_ADR_CALIBRATION_512K_FLASH							0x77000
#endif


/* SMP pairing and key information area */
#ifndef FLASH_ADR_SMP_PAIRING_512K_FLASH
#define FLASH_ADR_SMP_PAIRING_512K_FLASH         				0x78000	//78000 & 79000 & 7A000 & 7B000
#endif


/* for ACL Central simple SDP: bonding ACL Peripheral GATT service critical information area */
#ifndef FLASH_SDP_ATT_ADDRESS_512K_FLASH
#define FLASH_SDP_ATT_ADDRESS_512K_FLASH          				0x7D000 //7D000 & 7F000
#endif


/**************************** 1 M Flash *******************************/
#ifndef	CFG_ADR_MAC_1M_FLASH
#define	CFG_ADR_MAC_1M_FLASH		   							0xFF000
#endif

#ifndef	CFG_ADR_CALIBRATION_1M_FLASH
#define	CFG_ADR_CALIBRATION_1M_FLASH							0xFE000
#endif


/* SMP pairing and key information area */
#ifndef FLASH_ADR_SMP_PAIRING_1M_FLASH
#define FLASH_ADR_SMP_PAIRING_1M_FLASH         					0xFA000	//FA000 & FB000 & FC000 & FD000
#endif


/* for ACL Central simple SDP: bonding ACL Peripheral GATT service critical information area */
#ifndef FLASH_SDP_ATT_ADDRESS_1M_FLASH
#define FLASH_SDP_ATT_ADDRESS_1M_FLASH          				0xF6000 //F6000 & F7000
#endif


/**************************** 2 M Flash *******************************/
#ifndef CFG_ADR_MAC_2M_FLASH
#define CFG_ADR_MAC_2M_FLASH		   							0x1FF000
#endif

#ifndef CFG_ADR_CALIBRATION_2M_FLASH
#define CFG_ADR_CALIBRATION_2M_FLASH							0x1FE000
#endif


/* SMP pairing and key information area */
#ifndef FLASH_ADR_SMP_PAIRING_2M_FLASH
#define FLASH_ADR_SMP_PAIRING_2M_FLASH         					0x1FA000 //1FA000 & 1FB000 & 1FC000 & 1FD000
#endif

/* for ACL Central simple SDP: bonding ACL Peripheral GATT service critical information area */
#ifndef FLASH_SDP_ATT_ADDRESS_2M_FLASH
#define FLASH_SDP_ATT_ADDRESS_2M_FLASH          				0x1F6000 //1F6000 & 1F7000
#endif




/** Calibration Information FLash Address Offset of  CFG_ADR_CALIBRATION_xx_FLASH ***/
#define		CALIB_OFFSET_CAP_INFO								0x00
#define		CALIB_OFFSET_TP_INFO								0x40

#define		CALIB_OFFSET_ADC_VREF								0xC0

#define		CALIB_OFFSET_FIRMWARE_SIGNKEY						0x180

#define     CALIB_OFFSET_FLASH_VREF								0x1C0


extern	unsigned int  blc_flash_mid;
extern	unsigned int  blc_flash_vendor;
extern	unsigned char blc_flash_capacity;

extern  unsigned int  flash_sector_mac_address;
extern  unsigned int  flash_sector_calibration;
extern  unsigned int  flash_sector_smp_storage;
extern  unsigned int  flash_sector_simple_sdp_att;

/**
 * @brief		This function is used to enable the external crystal capacitor, only 1 can be set
 * @param[in]	en - enable the external crystal capacitor
 * @return      none
 */
static inline void blc_app_setExternalCrystalCapEnable(u8  en)
{
	blt_miscParam.ext_cap_en = en;

	WriteAnalogReg(0x8a,ReadAnalogReg(0x8a)|0x80);//close internal cap

}

/**
 * @brief		This function can automatically recognize the flash size,
 * 				and the system selects different customized sector according
 * 				to different sizes.
 * @param[in]	none
 * @return      none
 */
void blc_readFlashSize_autoConfigCustomFlashSector(void);

/**
 * @brief		This function is used to initialize the MAC address
 * @param[in]	flash_addr - flash address for MAC address
 * @param[in]	mac_public - public address
 * @param[in]	mac_random_static - random static MAC address
 * @return      none
 */
void blc_initMacAddress(int flash_addr, u8 *mac_public, u8 *mac_random_static);

/**
 * @brief		load customized parameters (from Flash/EFUSE) when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
void blc_app_loadCustomizedParameters_normal(void);

/**
 * @brief		load customized parameters (from SRAM) when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void blc_app_loadCustomizedParameters_deepRetn(void);


//TODO
#define blc_app_loadCustomizedParameters	blc_app_loadCustomizedParameters_normal

#endif /* BLE_FLASH_H_ */
