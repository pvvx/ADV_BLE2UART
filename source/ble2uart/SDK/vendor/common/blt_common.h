/********************************************************************************************************
 * @file	blt_common.h
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
#ifndef BLT_COMMON_H_
#define BLT_COMMON_H_

#include "drivers.h"
#include "vendor/common/user_config.h"



/**
 * @brief	512 K Flash MAC address and calibration data area
 */
#if (MCU_CORE_TYPE == MCU_CORE_9518)
	#define	CFG_ADR_MAC_512K_FLASH								0x7F000
	#define	CFG_ADR_CALIBRATION_512K_FLASH						0x7E000
#else
	#define	CFG_ADR_MAC_512K_FLASH								0x76000
	#define	CFG_ADR_CALIBRATION_512K_FLASH						0x77000
#endif

/**
 * @brief	1 M Flash MAC address and calibration data area
 */
#define	CFG_ADR_MAC_1M_FLASH		   							0xFF000
#define	CFG_ADR_CALIBRATION_1M_FLASH							0xFE000

/**
 * @brief	2 M Flash MAC address and calibration data area
 */
#define	CFG_ADR_MAC_2M_FLASH		   							0x1FF000
#define	CFG_ADR_CALIBRATION_2M_FLASH							0x1FE000







/**
 * @brief	Flash size type supported by this SDK
 */
#define		FLASH_SIZE_512K			   							0x80000
#define		FLASH_SIZE_1M			   							0x100000
#define		FLASH_SIZE_2M			   							0x200000


/**
 * @brief	Flash size default configuration(user can change in app_config.h)
 */
#if (MCU_CORE_TYPE == MCU_CORE_9518)
	#ifndef		FLASH_SIZE_CONFIG
	#define		FLASH_SIZE_CONFIG		   						FLASH_SIZE_1M
	#endif
#else
	#ifndef		FLASH_SIZE_CONFIG
	#define		FLASH_SIZE_CONFIG		   						FLASH_SIZE_512K
	#endif
#endif




/**
 * @brief	Flash using area default Configuration, user can change some of them in app_config.h
 * 			CFG_ADR_MAC:  		  BLE MAC address stored in flash, can not change this value
 * 			CFG_ADR_CALIBRATION:  some calibration data stored in flash, can not change this value
 * 			FLASH_ADR_SMP_PAIRING & FLASH_SMP_PAIRING_MAX_SIZE:
 * 									If Slave or Master SMP enable, use 16K flash for SMP pairing information storage.
 * 									First 8K is for normal use, second 8K is a backup to guarantee SMP information never lose.
 * 									use API blc_smp_configPairingSecurityInfoStorageAddressAndSize(FLASH_ADR_SMP_PAIRING, FLASH_SMP_PAIRING_MAX_SIZE)
 * 									to set the two value.
 * 			FLASH_ADR_CUSTOM_PAIRING & FLASH_CUSTOM_PAIRING_MAX_SIZE:
 * 									If master role is used but master SMP not used, use this flash area to store bonding slave information for custom pair.
 * 			FLASH_SDP_ATT_ADRRESS & FLASH_SDP_ATT_MAX_SIZE
 * 									If master role use service discovery, use this flash area to store some critical information of peer GATT server.

 */
#if(FLASH_SIZE_CONFIG == FLASH_SIZE_512K)

	/* MAC and calibration data area */
	#define		CFG_ADR_MAC		   								CFG_ADR_MAC_512K_FLASH			//can not change this value
	#define		CFG_ADR_CALIBRATION		   						CFG_ADR_CALIBRATION_512K_FLASH	//can not change this value

	#if (MCU_CORE_TYPE == MCU_CORE_9518)
		//TODO
	#else
		/* SMP paring and key information area */
		#ifndef FLASH_ADR_SMP_PAIRING
		#define FLASH_ADR_SMP_PAIRING         						0x78000
		#endif

		#ifndef FLASH_SMP_PAIRING_MAX_SIZE
		#define FLASH_SMP_PAIRING_MAX_SIZE         					(2*4096)  //normal 8K + backup 8K = 16K
		#endif


		/* bonding slave information for custom pair area */
		#ifndef FLASH_ADR_CUSTOM_PAIRING
		#define FLASH_ADR_CUSTOM_PAIRING         					0x7C000
		#endif

		#ifndef FLASH_CUSTOM_PAIRING_MAX_SIZE
		#define FLASH_CUSTOM_PAIRING_MAX_SIZE     					4096
		#endif


		/* bonding slave GATT service critical information area */
		#ifndef FLASH_SDP_ATT_ADRRESS
		#define FLASH_SDP_ATT_ADRRESS         						0x7D000    //for master: store peer slave device's ATT handle
		#endif

		#ifndef FLASH_SDP_ATT_MAX_SIZE
		#define FLASH_SDP_ATT_MAX_SIZE         						(2*4096)   //8K flash for ATT HANLDE storage
		#endif
	#endif

#elif(FLASH_SIZE_CONFIG == FLASH_SIZE_1M)

	/* MAC and calibration data area */
	#define	CFG_ADR_MAC		   									CFG_ADR_MAC_1M_FLASH			//can not change this value
	#define	CFG_ADR_CALIBRATION		   							CFG_ADR_CALIBRATION_1M_FLASH	//can not change this value

	/* SMP paring and key information area */
	#ifndef FLASH_ADR_SMP_PAIRING
	#define FLASH_ADR_SMP_PAIRING         						0xFA000
	#endif

	#ifndef FLASH_SMP_PAIRING_MAX_SIZE
	#define FLASH_SMP_PAIRING_MAX_SIZE         					(2*4096)  //normal 8K + backup 8K = 16K
	#endif


	/* bonding slave information for custom pair area */
	#ifndef FLASH_ADR_CUSTOM_PAIRING
	#define FLASH_ADR_CUSTOM_PAIRING         					0xF8000
	#endif

	#ifndef FLASH_CUSTOM_PAIRING_MAX_SIZE
	#define FLASH_CUSTOM_PAIRING_MAX_SIZE     					4096
	#endif


	/* bonding slave GATT service critical information area */
	#ifndef FLASH_SDP_ATT_ADRRESS
	#define FLASH_SDP_ATT_ADRRESS         						0xF6000    //for master: store peer slave device's ATT handle
	#endif

	#ifndef FLASH_SDP_ATT_MAX_SIZE
	#define FLASH_SDP_ATT_MAX_SIZE         						(2*4096)   //8K flash for ATT HANLDE storage
	#endif

#elif(FLASH_SIZE_CONFIG == FLASH_SIZE_2M)
/* MAC and calibration data area */
	#define	CFG_ADR_MAC		   									CFG_ADR_MAC_2M_FLASH			//can not change this value
	#define	CFG_ADR_CALIBRATION		   							CFG_ADR_CALIBRATION_2M_FLASH	//can not change this value

	/* SMP paring and key information area */
	#ifndef FLASH_ADR_SMP_PAIRING
	#define FLASH_ADR_SMP_PAIRING         						0x1FA000
	#endif

	#ifndef FLASH_SMP_PAIRING_MAX_SIZE
	#define FLASH_SMP_PAIRING_MAX_SIZE         					(2*4096)  //normal 8K + backup 8K = 16K
	#endif


	/* bonding slave information for custom pair area */
	#ifndef FLASH_ADR_CUSTOM_PAIRING
	#define FLASH_ADR_CUSTOM_PAIRING         					0x1F8000
	#endif

	#ifndef FLASH_CUSTOM_PAIRING_MAX_SIZE
	#define FLASH_CUSTOM_PAIRING_MAX_SIZE     					4096
	#endif


	/* bonding slave GATT service critical information area */
	#ifndef FLASH_SDP_ATT_ADRRESS
	#define FLASH_SDP_ATT_ADRRESS         						0x1F6000    //for master: store peer slave device's ATT handle
	#endif

	#ifndef FLASH_SDP_ATT_MAX_SIZE
	#define FLASH_SDP_ATT_MAX_SIZE         						(2*4096)   //8K flash for ATT HANLDE storage
	#endif
#endif





/** Calibration Information FLash Address Offset of  CFG_ADR_CALIBRATION_xx_FLASH ***/
#define		CALIB_OFFSET_CAP_INFO								0x00
#define		CALIB_OFFSET_TP_INFO								0x40

#define		CALIB_OFFSET_ADC_VREF								0xC0

#define		CALIB_OFFSET_FIRMWARE_SIGNKEY						0x180

#define     CALIB_OFFSET_FLASH_VREF								0x1C0





extern const u8 vendor_OtaUUID[];
extern u32 flash_sector_mac_address;
extern u32 flash_sector_calibration;




/**
 * @brief		This function is used to enable the external crystal capacitor
 * @param[in]	en - enable the external crystal capacitor
 * @return      none
 */
static inline void blc_app_setExternalCrystalCapEnable(u8  en)
{
	blt_miscParam.ext_cap_en = en;
	analog_write(0x8a, analog_read(0x8a) | 0x80);//disable internal cap
}


/**
 * @brief		This function is used to load customized parameters from flash sector for application
 * @param[in]	none
 * @return      none
 */
static inline void blc_app_loadCustomizedParameters(void)
{
	/* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */

	if(!blt_miscParam.ext_cap_en)
	{
		//customize freq_offset adjust cap value, if not customized, default ana_81 is 0xd0
		//for 512K Flash, flash_sector_calibration equals to 0x77000
		//for 1M  Flash, flash_sector_calibration equals to 0xFE000
		if(flash_sector_calibration){
		u8 cap_frqoft = *(unsigned char*) (flash_sector_calibration + CALIB_OFFSET_CAP_INFO);
		if( cap_frqoft != 0xff ){
				analog_write(0x8A, (analog_read(0x8A) & 0xc0)|(cap_frqoft & 0x3f));
			}
		}
	}

//////////////////////////////////////////////////////////////////////////////////////////////
#if(MCU_CORE_TYPE == MCU_CORE_827x)
	u16 calib_value = *(unsigned short*)(flash_sector_calibration + CALIB_OFFSET_FLASH_VREF);

	if((0xffff == calib_value) || (0 != (calib_value & 0xf8f8)))
	{
		if(zbit_flash_flag)
		{
			analog_write(0x09, ((analog_read(0x09) & 0x8f) | (FLASH_VOLTAGE_1V95 << 4)));	//ldo mode flash ldo trim 1.95V
			analog_write(0x0c, ((analog_read(0x0c) & 0xf8) | FLASH_VOLTAGE_1V9));			//dcdc mode flash ldo trim 1.90V
		}
	}
	else
	{
		analog_write(0x09, ((analog_read(0x09) & 0x8f)  | ((calib_value & 0xff00) >> 4) ));
		analog_write(0x0c, ((analog_read(0x0c) & 0xf8)  | (calib_value & 0xff)));
	}
#elif(MCU_CORE_TYPE == MCU_CORE_825x)
	u8 calib_value = *(unsigned char*)(flash_sector_calibration + CALIB_OFFSET_FLASH_VREF);

	if((0xff == calib_value))
	{
		if(zbit_flash_flag)
		{
			analog_write(0x0c, ((analog_read(0x0c) & 0xf8)  | FLASH_VOLTAGE_1V95));			//dcdc mode flash ldo trim 1.95V
		}
	}
	else
	{
		analog_write(0x0c, ((analog_read(0x0c) & 0xf8)  | (calib_value&0x7)));
	}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
#if(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
	//load adc vref value from flash
	u8 adc_vref_calib_value_rd[4] = {0};
	if(adc_vref_cfg.adc_calib_en)
	{
		flash_read_page(flash_sector_calibration + CALIB_OFFSET_ADC_VREF, 4, adc_vref_calib_value_rd);
		if((adc_vref_calib_value_rd[2] != 0xff) || (adc_vref_calib_value_rd[3]  != 0xff ))
		{
			/******Method of calculating calibration Flash_vbat_Vref value: ********/
			/******Vref = [1175 +First_Byte-255+Second_Byte] mV =  [920 + First_Byte + Second_Byte] mV  ********/
			adc_vref_cfg.adc_vref = 920 + adc_vref_calib_value_rd[2] + adc_vref_calib_value_rd[3];
		}
		//else use the value init in efuse
	}
#endif
}


/**
 * @brief		This function is used to initialize the MAC address
 * @param[in]	flash_addr - flash address for MAC address
 * @param[in]	mac_public - public address
 * @param[in]	mac_random_static - random static MAC address
 * @return      none
 */
void blc_initMacAddress(int flash_addr, u8 *mac_public, u8 *mac_random_static);




#endif /* BLT_COMMON_H_ */
