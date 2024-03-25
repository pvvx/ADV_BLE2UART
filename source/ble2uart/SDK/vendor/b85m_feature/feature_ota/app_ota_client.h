/********************************************************************************************************
 * @file	app_ota_client.h
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

#ifndef VENDOR_B91_FEATURE_FEATURE_OTA_APP_OTA_CLIENT_H_
#define VENDOR_B91_FEATURE_FEATURE_OTA_APP_OTA_CLIENT_H_

#if (FEATURE_TEST_MODE == TEST_OTA)


/**
 * @brief	peer device's new firmware storage address on local device
 */
#define NEW_FW_ADDR_128K								0x20000
#define NEW_FW_ADDR_256K								0x40000
#define NEW_FW_ADDR_512K								0x80000
#define NEW_FW_ADDR_768K								0xC0000


#if (MCU_CORE_TYPE == MCU_CORE_9518)
	#define NEW_FW_ADDR0								NEW_FW_ADDR_512K
	#define NEW_FW_ADDR1								NEW_FW_ADDR_768K
	#define FW_SIZE_MIN									0x04000	 //16K
	#define FW_SIZE_MAX									0x7F000	 //508K
#else
	#define NEW_FW_ADDR0								NEW_FW_ADDR_128K
	#define NEW_FW_ADDR1								NEW_FW_ADDR_256K
	#define FW_SIZE_MIN									0x04000	 //16K
	#define FW_SIZE_MAX									0x20000	 //128K
#endif




#define OTA_STEP_0_IDLE									0
#define OTA_STEP_1_CHECK_FW								1	//check Firmware
#define OTA_STEP_2_UPDATE_CONN							2   //update to a small connection interval to speed up OTA process
#define OTA_STEP_3_REQ_OTA_HANDLE						3	//send read_by_type_req to request peer device's OTA attribute handle
#define OTA_STEP_4_WAIT_OTA_HANDLE						4   //wait read_by_type_rsp to get peer device's OTA attribute handle

//step 5 & 6 can
#define OTA_STEP_5_REQ_FW_VERSION						5   //send OTA command "CMD_OTA_FW_VERSION_REQ" to request peer device's firmware version
#define OTA_STEP_6_WAIT_FW_VERSION						6   //wait OTA command "CMD_OTA_FW_VERSION_RSP" to get peer device's firmware version

#define OTA_STEP_7_OTA_START							7   //send OTA command "CMD_OTA_START" or "CMD_OTA_START_EXT" to trigger OTA
#define OTA_STEP_8_OTA_DATA							    8
#define OTA_STEP_9_OTA_END							    9
#define OTA_STEP_10_WAIT_OTA_RESULT						10






#define OTA_TIMEOUT_S									100	//user can change




#if (!OTA_LEGACY_PROTOCOL)
	#define OTA_FW_VERSION_EXCHANGE_ENABLE				0	//user can change
	#define OTA_FW_VERSION_COMPARE_ENABLE				0	//user can change
#endif







typedef struct{
	u8	ota_update_flow;
	u8	version_compare;
	u8	ota_test_mode;
	u8  last_valid_pdu_len;  //maximum value: 240

	u16 ota_attHandle;
	u16 ota_connHandle;
	u16	cur_adr_index;
	u16 last_adr_index;
	u16 new_fw_version_num;
	u16 rsvd;

	u32	ota_new_fw_addr;
	u32 firmware_offset;
	u32 firmware_size;

	u32 ota_start_tick;
	u32 wait_result_begin_tick;

}ota_client_t;

extern ota_client_t blotaClt;

extern u32 ota_trigger_tick;


void app_key_trigger_ota_start(int new_fw_addr, u16 connHandle);

void app_process_ota_mainloop(void);

void app_process_ota_connection_terminate(u16 connHandle);

void app_ota_process_handle_value_notify(rf_packet_att_t *);

void app_ota_process_read_by_type_response(u8 *pkt);

void app_ota_set_result(int result);

void app_updateOtaFlow(int step);

#endif
#endif /* VENDOR_B91_FEATURE_FEATURE_OTA_APP_OTA_CLIENT_H_ */
