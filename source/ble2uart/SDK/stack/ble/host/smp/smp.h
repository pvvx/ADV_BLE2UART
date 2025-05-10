/********************************************************************************************************
 * @file    smp.h
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
#ifndef BLE_SMP_H_
#define BLE_SMP_H_

#include "stack/ble/ble_common.h"

/**
 * @brief	SMP first pairing or connecting back definition
 */
#define SMP_STANDARD_PAIR   							0
#define SMP_FAST_CONNECT   								1
/**
 * @brief	SMP pairing fail reason definition
 */
#define PAIRING_FAIL_REASON_PASSKEY_ENTRY				0x01
#define PAIRING_FAIL_REASON_OOB_NOT_AVAILABLE			0x02
#define PAIRING_FAIL_REASON_AUTH_REQUIRE				0x03
#define PAIRING_FAIL_REASON_CONFIRM_FAILED				0x04
#define PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED		0x05
#define PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE			0x06
#define PAIRING_FAIL_REASON_CMD_NOT_SUPPORT				0x07 //-- core 4.2
#define PAIRING_FAIL_REASON_UNSPECIFIED_REASON			0x08
#define PAIRING_FAIL_REASON_REPEATED_ATTEMPT			0x09
#define PAIRING_FAIL_REASON_INVALID_PARAMETER			0x0A
#define PAIRING_FAIL_REASON_DHKEY_CHECK_FAIL			0x0B
#define PAIRING_FAIL_REASON_NUMERIC_FAILED				0x0C
#define PAIRING_FAIL_REASON_BREDR_PAIRING				0x0D
#define PAIRING_FAIL_REASON_CROSS_TRANSKEY_NOT_ALLOW	0x0E
#define PAIRING_FAIL_REASON_PAIRING_TIMEOUT				0x80 //TLK defined
#define PAIRING_FAIL_REASON_CONN_DISCONNECT				0x81 //TLK defined
#define PAIRING_FAIL_REASON_SUPPORT_NC_ONLY				0x82 //TLK defined


/**
 * @brief	"SecReq" refer to "security request"
 */
typedef enum {
	SecReq_NOT_SEND = 0,   // do not send "security request" after link layer connection established
	SecReq_IMM_SEND = BIT(0),   //"IMM" refer to immediate, send "security request" immediately after link layer connection established
	SecReq_PEND_SEND = BIT(1),  //"PEND" refer to pending,  pending "security request" for some time after link layer connection established, when pending time arrived. send it
}secReq_cfg;
/**
 * @brief	"PairReq" refer to "pairing request"
 */
typedef enum {
	PairReq_SEND_upon_SecReq = 0,   // master send "pairing request" when received slave's "security request"
	PairReq_AUTO_SEND		 = 1,   // master send "pairing request" automatically, regardless of "security request"
}PairReq_cfg;

/**
 * @brief	refer to BLE Core Specification: Vol 3, Part C, "10.2 LE SECURITY MODES" for more information.
 */
typedef enum {
	LE_Security_Mode_1_Level_1 = BIT(0),  No_Authentication_No_Encryption			= BIT(0), No_Security = BIT(0),
	LE_Security_Mode_1_Level_2 = BIT(1),  Unauthenticated_Pairing_with_Encryption 	= BIT(1),
	LE_Security_Mode_1_Level_3 = BIT(2),  Authenticated_Pairing_with_Encryption 	    = BIT(2),
	LE_Security_Mode_1_Level_4 = BIT(3),  Authenticated_LE_Secure_Connection_Pairing_with_Encryption = BIT(3),

	LE_Security_Mode_2_Level_1 = BIT(4),  Unauthenticated_Pairing_with_Data_Signing 	= BIT(4),
	LE_Security_Mode_2_Level_2 = BIT(5),  Authenticated_Pairing_with_Data_Signing    = BIT(5),

	LE_Security_Mode_1 = (LE_Security_Mode_1_Level_1 | LE_Security_Mode_1_Level_2 | LE_Security_Mode_1_Level_3 | LE_Security_Mode_1_Level_4)
}le_security_mode_level_t;
/**
 * @brief	Enumeration defining ECDH key distribution modes.
 */
typedef enum {
	non_debug_mode = 0,  // ECDH distribute private/public key pairs
	debug_mode = 1,      // ECDH use debug mode private/public key pairs
} ecdh_keys_mode_t;
/**
 * @brief	Enumeration defining bonding modes.
 */
typedef enum {
	Non_Bondable_Mode = 0,
	Bondable_Mode     = 1,
}bonding_mode_t;
/**
 * @brief	Pairing Methods select
 * 			refer to BLE Core Specification: Vol 3, Part H, "2.3 PAIRING METHODS" for more information.
 */
typedef enum {
	LE_Legacy_Pairing     = 0,   // BLE 4.0/4.2
	LE_Secure_Connection = 1,   // BLE 4.2/5.0/5.1
}pairing_methods_t;
/**
 * @brief   Enumeration defining input and output capabilities for Bluetooth Low Energy pairing.
 */
typedef enum {
	IO_CAPABILITY_UNKNOWN = 0xff,
	IO_CAPABILITY_DISPLAY_ONLY = 0,
	IO_CAPABILITY_DISPLAY_YES_NO = 1,
	IO_CAPABILITY_KEYBOARD_ONLY = 2,
	IO_CAPABILITY_NO_INPUT_NO_OUTPUT = 3,
	IO_CAPABILITY_KEYBOARD_DISPLAY = 4,
} io_capability_t;
/**
 * @brief	Keypress Notification type
 */
typedef enum {
	KEYPRESS_NTF_PKE_START			=	0x00,
	KEYPRESS_NTF_PKE_DIGIT_ENTERED	=	0x01,
	KEYPRESS_NTF_PKE_DIGIT_ERASED	=	0x02,
	KEYPRESS_NTF_PKE_CLEARED		=	0x03,
	KEYPRESS_NTF_PKE_COMPLETED		=	0x04,
} keypress_notify_t;

/**
 * @brief	SC OOB local ECDH key
 */
typedef struct  {
    /** Public Key. */
    u8 public_key[64]; //big--endian
    /** Private Key. */
    u8 private_key[32]; //big--endian
}smp_sc_oob_key_t;
/**
 * @brief	SC OOB data
 */
typedef struct  {
    /** Random Number. */
    u8 random[16]; //big--endian
    /** Confirm Value. */
    u8 confirm[16]; //big--endian
}smp_sc_oob_data_t;

/**
 * @brief	local IRK generating strategy when controller use identity address in RF packet
 * 			default use "LOCIRK_BINDING_WITH_DEVICE" if user not set
 */
typedef enum {
	LOCIRK_BINDING_WITH_DEVICE = 0,  //every device have unique local IRK, never change
	LOCIRK_RANDOM_GENERATE     = 1,  //random generate
}loc_irk_gen_str_t;

/**
 * @brief      This function is used to set local IRK generating strategy.
 * @param[in]  str - local IRK generating strategy.
 * @return     none.
 */
void	 	blc_smp_setLocalIrkGenerateStrategy (loc_irk_gen_str_t  str);
/**
 * @brief      This function is used to initialize each parameter configuration of SMP, including the initialization of the binding area FLASH.
 * @param[in]  none
 * @return     none
 */
void 	blc_smp_smpParamInit(void);


/**
 * @brief      This function is used to configure whether the slave sends a Security Request
 *             to the master immediately after the connection or after the connection is
 *             pending_ms milliseconds, or does not send the Security Request.
 * @param[in]  newConn_cfg - refer to "security request"
 * @param[in]  re_conn_cfg - refer to "security request"
 * @param[in]  pending_ms - Send a Security Request to the master after pending_ms milliseconds
 * @return     none.
 */
void 	blc_smp_configSecurityRequestSending( secReq_cfg newConn_cfg,  secReq_cfg reConn_cfg, u16 pending_ms);


/**
 * @brief      This function is used to check whether active pairing or automatic connection back is required.
 * @param[in]  newConn_cfg - refer to "PairReq_cfg"
 * @param[in]  reConn_cfg - refer to "PairReq_cfg"
 * @return     none.
 */
void    blc_smp_configPairingRequestSending( PairReq_cfg newConn_cfg,  PairReq_cfg reConn_cfg);

/**
 * @brief      This function is used to trigger master sending Pairing Request,
 *             when configured as PairReq_SEND_upon_SecReq but not received slave's Security Request.
 * @param[in]  none.
 * @return     none.
 */
void 	blc_smp_triggerMasterManualSmp(void);

/**
 * @brief      This function is used to set security level.
 * @param[in]  mode_level - The security level value can refer to the structure 'le_security_mode_level_t'.
 * @return     none.
 */
void 	blc_smp_setSecurityLevel(le_security_mode_level_t  mode_level);
void 	blc_smp_setSecurityLevel_master(le_security_mode_level_t  mode_level);
void 	blc_smp_setSecurityLevel_slave(le_security_mode_level_t  mode_level);


/**
 * @brief      This function is used to get security level.
 * @param[in]  none
 * @return     mode_level - The security level value can refer to the structure 'le_security_mode_level_t'.
 */
le_security_mode_level_t blc_smp_getSecurityLevel_master(void);
le_security_mode_level_t blc_smp_getSecurityLevel_slave(void);
le_security_mode_level_t blc_smp_getSecurityLevel_slave_by_connHandle(u16 connHandle);


/**
 * @brief      This function is used to set pairing method.
 * @param[in]  method - The pairing method value can refer to the structure 'pairing_methods_t'.
 *                      0: LE legacy pairing;
 *                      1: LE secure connection
 * @return     none.
 */
void    blc_smp_setPairingMethods(pairing_methods_t  method);
void 	blc_smp_setPairingMethods_master(pairing_methods_t  method);    //select pairing methods
void 	blc_smp_setPairingMethods_slave (pairing_methods_t  method);    //select pairing methods


/**
 * @brief      This function is used to set device's IO capability.
 * @param[in]  ioCapability - The IO capability's value can refer to the structure 'io_capability_t'.
 * @return     none.
 */
void    blc_smp_setIoCapability(io_capability_t ioCapability);
void    blc_smp_setIoCapability_master(io_capability_t ioCapability);
void    blc_smp_setIoCapability_slave(io_capability_t ioCapability);


/**
 * @brief      This function is used to set if enable OOB authentication.
 * @param[in]  OOB_en - 0: Disable OOB authentication;
 *                      1: Enable OOB authentication.
 * @return     none.
 */
void    blc_smp_enableOobAuthentication(int OOB_en);
void 	blc_smp_enableOobAuthentication_master (int OOB_en);
void 	blc_smp_enableOobAuthentication_slave (int OOB_en);


/**
 * @brief      This function is used to set bonding mode.
 * @param[in]  mode - The bonding mode value can refer to the structure 'bonding_mode_t'.
 *                    0: non bondable mode;
 *                    1: bondable mode.
 * @return     none.
 */
void    blc_smp_setBondingMode(bonding_mode_t mode);
void 	blc_smp_setBondingMode_master(bonding_mode_t mode);
void 	blc_smp_setBondingMode_slave(bonding_mode_t mode);


/**
 * @brief      This function is used to set if enable authentication MITM protection.
 * @param[in]  MITM_en - 0: Disable MITM protection;
 *                       1: Enable MITM protection.
 * @return     none.
 */
void    blc_smp_enableAuthMITM(int MITM_en);
void 	blc_smp_enableAuthMITM_master(int MITM_en);
void 	blc_smp_enableAuthMITM_slave(int MITM_en);


/**
 * @brief      This function is used to set device's Keypress.
 * @param[in]  keyPress_en - 0: Disable Keypress;
 *                           1: Enable Keypress.
 * @return     none.
 */
void    blc_smp_enableKeypress(int keyPress_en);
void 	blc_smp_enableKeypress_master(int keyPress_en);
void 	blc_smp_enableKeypress_slave(int keyPress_en);


/**
 * @brief      This function is used to set whether the device uses the ECDH DEBUG key.
 * @param[in]  mode - The ECDH key mode value can refer to the structure 'ecdh_keys_mode_t'.
 *                    0: non debug mode;
 *                    1: debug mode.
 * @return     none.
 */
void    blc_smp_setEcdhDebugMode(ecdh_keys_mode_t mode);
void 	blc_smp_setEcdhDebugMode_master(ecdh_keys_mode_t mode);
void 	blc_smp_setEcdhDebugMode_slave(ecdh_keys_mode_t mode);


/**
 * @brief      This function is used to set the default pincode displayed by our side When using the PasskeyEntry.
 * @param[in]  pinCodeInput - Input pincode value. Value range [000000~999999].
 * @return     none.
 */
void	blc_smp_setDefaultPinCode(u32 pinCodeInput);
void	blc_smp_setDefaultPinCode_master(u32 pinCodeInput);
void	blc_smp_setDefaultPinCode_slave(u32 pinCodeInput);


/**
 * @brief      This function is used to set device's security parameters.
 * @param[in]  mode - The bonding mode value can refer to the structure 'bonding_mode_t'.
 * @param[in]  MITM_en - 0: Disable MITM protection;  1: Enable MITM protection.
 * @param[in]  method - 0: LE_Legacy_Pairing; 1: LE_Secure_Connection.
 * @param[in]  OOB_en - 0: Disable OOB authentication; 1: Enable OOB authentication.
 * @param[in]  keyPress_en - 0: Disable Keypress; 1: Enable Keypress.
 * @param[in]  ioCapability - The IO capability's value can refer to the structure 'io_capability_t'.
 * @return     none.
 */
void 	blc_smp_setSecurityParameters(bonding_mode_t mode, int MITM_en, pairing_methods_t method, int OOB_en,
		                             int keyPress_en, io_capability_t ioCapability);
void 	blc_smp_setSecurityParameters_master(bonding_mode_t  bond_mode, int MITM_en, pairing_methods_t method, int OOB_en,
		                                   int keyPress_en,io_capability_t ioCapability);
void 	blc_smp_setSecurityParameters_slave(bonding_mode_t  bond_mode, int MITM_en, pairing_methods_t method, int OOB_en,
		                                   int keyPress_en,io_capability_t ioCapability);


/**
 * @brief      This function is used to set TK by OOB method.
 * param[in]   connHandle - Current ACL connection handle.
 * @param[in]  oobData - TK's value, size: 16 byte.
 * @return     none.
 */
u8    	blc_smp_setTK_by_OOB              (u16 connHandle, u8 *oobData);      //used for set oob data


/**
 * @brief      This function is used to set TK by passkey entry method.
 * param[in]   connHandle - Current ACL connection handle.
 * @param[in]  pinCodeInput - TK's value, input range [000000, 999999].
 * @return     none.
 */
u8    	blc_smp_setTK_by_PasskeyEntry     (u16 connHandle, u32 pinCodeInput);


/**
 * @brief      This function is used to send keypress notify during TK input phrase.
 * param[in]   connHandle - Current ACL connection handle.
 * @param[in]  ntfType - refer to 'keypress_notify_t'.
 * @return     True: send SUCC
 * 			   False: send FAIL
 */
bool 	blc_smp_sendKeypressNotify (u16 connHandle, keypress_notify_t ntfType);

/**
 * @brief      This function is used to set numeric compare confirm YES or NO.
 * param[in]   connHandle - Current ACL connection handle.
 * @param[in]  YES_or_NO - 1: numeric compare confirm YES;
 *                         0: numeric compare confirm NO.
 * @return     none.
 */
void    blc_smp_setNumericComparisonResult(u16 connHandle, bool YES_or_NO);   //numeric compare confirm, 1: YES  0: NO


/**
 * @brief      This function is used to check if the pairing is busy.
 * @param[in]  connHandle - Current ACL connection handle.
 * @return     1:is pair busy
 * 			   0:isn't pair busy
 */
int 	blc_smp_isPairingBusy(u16 connHandle);


/**
 * @brief      This function is used to check whether the PinCode needs to be input.
 * @param[in]  connHandle - Current ACL connection handle.
 * @return     1: Need to enter PinCode
 * 			   0: No need to enter PinCode
 */
u8      blc_smp_isWaitingToSetTK(u16 connHandle);


/**
 * @brief      This function is used to check whether it is needed to confirm NC YES/NO.
 * @param[in]  connHandle - Current ACL connection handle.
 * @return     1: Need to confirm NC YES/NO
 * 			   0: No need to confirm NC YES/NO
 */
u8 		blc_smp_isWaitingToCfmNumericComparison(u16 connHandle);

/**
 * @brief      This function is used to generate SC OOB local(+ECDH key) data.
 * param[out]  local_oob_data - local SC OOB data.
 * param[out]  local_ecdh_key - local ECDH (Public/Private keys).
 * @return     1: SUCCESS; 0: FAILED
 */
int		blc_smp_generateScOobData(smp_sc_oob_data_t *local_oob_data, smp_sc_oob_key_t *local_ecdh_key);

/**
 * @brief      This function is used to set SC OOB local(+ECDH key), remote data.
 * @param[in]  connHandle - Current ACL connection handle.
 * param[in]   oob_local - local SC OOB data.
 * param[in]   local_ecdh_key - local ECDH (Public/Private keys).
 * param[in]   oob_remote - remote SC OOB data.
 * @return     1: SUCCESS; 0: FAILED
 */
int		blc_smp_setScOobData(u16 connHandle, const smp_sc_oob_data_t *oob_local,
							const smp_sc_oob_key_t *local_ecdh_key,
							const smp_sc_oob_data_t *oob_remote);

/**
 * @brief      This function is used to cancel SMP process.
 * @param[in]  connHandle - Current ACL connection handle.
 * @return     1: SUCCESS; 0: FAILED
 */
bool	blc_smp_cancel_auth(u16 connHandle);


#endif /* BLE_SMP_H_ */

