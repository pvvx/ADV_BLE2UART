/********************************************************************************************************
 * @file    app.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "application/keyboard/keyboard.h"

#include "app.h"
#include "app_att.h"
#include "app_ui.h"
#include "app_buffer.h"
#include "rc_ir.h"
#include "rc_ir_learn.h"
#include "app_audio.h"




_attribute_ble_data_retention_	u8 ota_is_working = 0;



const u8	tbl_advData[] = {
	 7,  DT_COMPLETE_LOCAL_NAME, 				'R','e','m','o','t','e',
	 2,	 DT_FLAGS, 								0x05, 					// BLE limited discoverable mode and BR/EDR not supported
	 3,  DT_APPEARANCE, 						0x80, 0x01, 			// 384, Generic Remote Control, Generic category
	 5,  DT_INCOMPLT_LIST_16BIT_SERVICE_UUID,	0x12, 0x18, 0x0F, 0x18,	// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8	tbl_scanRsp [] = {
	 7, DT_COMPLETE_LOCAL_NAME, 				'R','e','m','o','t','e',
};




_attribute_data_retention_	u32	latest_user_event_tick;
/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int 	app_disconnect_event_handle(u8 *p)
{
	event_disconnection_t	*pCon = (event_disconnection_t *)p;

	//terminate reason
	if(pCon->reason == HCI_ERR_CONN_TIMEOUT){  	//connection timeout

	}
	else if(pCon->reason == HCI_ERR_REMOTE_USER_TERM_CONN){  	//peer device send terminate command on link layer

	}
	//master host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
	else if(pCon->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){

	}
	else{

	}


	if(central_disconnect_connhandle == pCon->connHandle){  //un_pair disconnection flow finish, clear flag
		central_disconnect_connhandle = 0;
	}

	dev_char_info_delete_by_connhandle(pCon->connHandle);

#if (BLE_AUDIO_ENABLE)
	if(ui_mic_enable){
		ui_enable_mic (0);
	}
#endif

	return 0;
}


/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
_attribute_ble_data_retention_ u8 dbg_req_cnt = 0;
int app_le_connection_complete_event_handle(u8 *p)
{
	hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;

	if(pConnEvt->status == BLE_SUCCESS){

		dev_char_info_insert_by_conn_event(pConnEvt);

		if(pConnEvt->role == ACL_ROLE_PERIPHERAL){
			bls_l2cap_requestConnParamUpdate (pConnEvt->connHandle, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 99, CONN_TIMEOUT_4S);	// 1 second
//			bls_l2cap_requestConnParamUpdate (pConnEvt->connHandle, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 199, CONN_TIMEOUT_6S);	// 2 second
//			bls_l2cap_requestConnParamUpdate (pConnEvt->connHandle, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 299, CONN_TIMEOUT_8S);	// 3 second

			latest_user_event_tick = clock_time();
		}
	}

	return 0;
}

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_EXIT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void task_suspend_exit(u8 e, u8 *p, int n)
{
	rf_set_power_level_index(RF_POWER_P3dBm);
}

//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback(u32 h, u8 *p, int n)
{
	if (h &HCI_FLAG_EVENT_BT_STD)		//Controller HCI event
	{
		u8 evtCode = h & 0xff;

		//------------ disconnect -------------------------------------
		if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE)  //connection terminate
		{
			app_disconnect_event_handle(p);
		}
		else if(evtCode == HCI_EVT_LE_META)  //LE Event
		{
			u8 subEvt_code = p[0];

			//------hci le event: le connection complete event---------------------------------
			if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)	// connection complete
			{
				app_le_connection_complete_event_handle(p);
			}
			//--------hci le event: le adv report event ----------------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event

//				app_le_adv_report_event_handle(p);
			}
			//------hci le event: le connection update complete event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)	// connection update
			{

			}
		}
	}


	return 0;

}




/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback(u32 h, u8 *para, int n)
{
	u8 event = h & 0xFF;

	switch(event)
	{
		case GAP_EVT_SMP_PAIRING_BEGIN:
		{
		}
		break;

		case GAP_EVT_SMP_PAIRING_SUCCESS:
		{
		}
		break;

		case GAP_EVT_SMP_PAIRING_FAIL:
		{

		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{

		}
		break;

		case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
		{

		}
		break;
		case GAP_EVT_ATT_EXCHANGE_MTU:
		{

		}
		break;

		case GAP_EVT_GATT_HANDLE_VALUE_CONFIRM:
		{

		}
		break;

		default:
		break;
	}

	return 0;
}



/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt)
{
	if( dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL)   //GATT data for Master
	{
		rf_packet_att_t *pAtt = (rf_packet_att_t*)pkt;

		dev_char_info_t* dev_info = dev_char_info_search_by_connhandle (connHandle);
		if(dev_info)
		{
			//-------	user process ------------------------------------------------
			if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)
			{

			}
			else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
			{
				blc_gatt_pushConfirm(connHandle);
			}
		}

		/* The Master does not support GATT Server by default */
		if(!(pAtt->opcode & 0x01)){
			switch(pAtt->opcode){
				case ATT_OP_FIND_INFO_REQ:
				case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
				case ATT_OP_READ_BY_TYPE_REQ:
				case ATT_OP_READ_BY_GROUP_TYPE_REQ:
					blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_ATTR_NOT_FOUND);
					break;
				case ATT_OP_READ_REQ:
				case ATT_OP_READ_BLOB_REQ:
				case ATT_OP_READ_MULTI_REQ:
				case ATT_OP_WRITE_REQ:
				case ATT_OP_PREPARE_WRITE_REQ:
					blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_INVALID_HANDLE);
					break;
				case ATT_OP_EXECUTE_WRITE_REQ:
				case ATT_OP_HANDLE_VALUE_CFM:
				case ATT_OP_WRITE_CMD:
				case ATT_OP_SIGNED_WRITE_CMD:
					//ignore
					break;
				default://no action
					break;
			}
		}
	}
	else{   //GATT data for Slave


	}


	return 0;
}











#if (APP_FLASH_PROTECTION_ENABLE)

/**
 * @brief      flash protection operation, including all locking & unlocking for application
 * 			   handle all flash write & erase action for this demo code. use should add more more if they have more flash operation.
 * @param[in]  flash_op_evt - flash operation event, including application layer action and stack layer action event(OTA write & erase)
 * 			   attention 1: if you have more flash write or erase action, you should should add more type and process them
 * 			   attention 2: for "end" event, no need to pay attention on op_addr_begin & op_addr_end, we set them to 0 for
 * 			   			    stack event, such as stack OTA write new firmware end event
 * @param[in]  op_addr_begin - operating flash address range begin value
 * @param[in]  op_addr_end - operating flash address range end value
 * 			   attention that, we use: [op_addr_begin, op_addr_end)
 * 			   e.g. if we write flash sector from 0x10000 to 0x20000, actual operating flash address is 0x10000 ~ 0x1FFFF
 * 			   		but we use [0x10000, 0x20000):  op_addr_begin = 0x10000, op_addr_end = 0x20000
 * @return     none
 */
_attribute_data_retention_ u16  flash_lockBlock_cmd = 0;
void app_flash_protection_operation(u8 flash_op_evt, u32 op_addr_begin, u32 op_addr_end)
{
	if(flash_op_evt == FLASH_OP_EVT_APP_INITIALIZATION)
	{
		/* ignore "op addr_begin" and "op addr_end" for initialization event
		 * must call "flash protection_init" first, will choose correct flash protection relative API according to current internal flash type in MCU */
		flash_protection_init();

		/* just sample code here, protect all flash area for old firmware and OTA new firmware.
		 * user can change this design if have other consideration */
		u32  app_lockBlock = 0;
		#if (BLE_OTA_SERVER_ENABLE)
			u32 multiBootAddress = blc_ota_getCurrentUsedMultipleBootAddress();
			if(multiBootAddress == MULTI_BOOT_ADDR_0x20000){
				app_lockBlock = FLASH_LOCK_FW_LOW_256K;
			}
			else if(multiBootAddress == MULTI_BOOT_ADDR_0x40000){
				/* attention that 512K capacity flash can not lock all 512K area, should leave some upper sector
				 * for system data(SMP storage data & calibration data & MAC address) and user data
				 * will use a approximate value */
				app_lockBlock = FLASH_LOCK_FW_LOW_512K;
			}
			#if(MCU_CORE_TYPE == MCU_CORE_827x)
			else if(multiBootAddress == MULTI_BOOT_ADDR_0x80000){
				if(blc_flash_capacity < FLASH_SIZE_1M){ //for flash capacity smaller than 1M, OTA can not use 512K as multiple boot address
					blc_flashProt.init_err = 1;
				}
				else{
					/* attention that 1M capacity flash can not lock all 1M area, should leave some upper sector for
					 * system data(SMP storage data & calibration data & MAC address) and user data
					 * will use a approximate value */
					app_lockBlock = FLASH_LOCK_FW_LOW_1M;
				}
			}
			#endif
		#else
			app_lockBlock = FLASH_LOCK_FW_LOW_512K; //just demo value, user can change this value according to application
		#endif


		flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);

		if(blc_flashProt.init_err){
			tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] flash protection initialization error!!!\n");
		}

		tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] initialization, lock flash\n");
		flash_lock(flash_lockBlock_cmd);
	}
#if (BLE_OTA_SERVER_ENABLE)
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_BEGIN)
	{
		/* OTA clear old firmware begin event is triggered by stack, in "blc ota_initOtaServer_module", rebooting from a successful OTA.
		 * Software will erase whole old firmware for potential next new OTA, need unlock flash if any part of flash address from
		 * "op addr_begin" to "op addr_end" is in locking block area.
		 * In this sample code, we protect whole flash area for old and new firmware, so here we do not need judge "op addr_begin" and "op addr_end",
		 * must unlock flash */
		tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA clear old FW begin, unlock flash\n");
		flash_unlock();
	}
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_END)
	{
		/* ignore "op addr_begin" and "op addr_end" for END event
		 * OTA clear old firmware end event is triggered by stack, in "blc ota_initOtaServer_module", erasing old firmware data finished.
		 * In this sample code, we need lock flash again, because we have unlocked it at the begin event of clear old firmware */
		tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA clear old FW end, restore flash locking\n");
		flash_lock(flash_lockBlock_cmd);
	}
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_BEGIN)
	{
		/* OTA write new firmware begin event is triggered by stack, when receive first OTA data PDU.
		 * Software will write data to flash on new firmware area,  need unlock flash if any part of flash address from
		 * "op addr_begin" to "op addr_end" is in locking block area.
		 * In this sample code, we protect whole flash area for old and new firmware, so here we do not need judge "op addr_begin" and "op addr_end",
		 * must unlock flash */
		tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA write new FW begin, unlock flash\n");
		flash_unlock();
	}
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_END)
	{
		/* ignore "op addr_begin" and "op addr_end" for END event
		 * OTA write new firmware end event is triggered by stack, after OTA end or an OTA error happens, writing new firmware data finished.
		 * In this sample code, we need lock flash again, because we have unlocked it at the begin event of write new firmware */
		tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA write new FW end, restore flash locking\n");
		flash_lock(flash_lockBlock_cmd);
	}
#endif
	/* add more flash protection operation for your application if needed */
}


#endif


#if (APP_BATT_CHECK_ENABLE)  //battery check must do before OTA relative operation

_attribute_data_retention_	u32	lowBattDet_tick   = 0;

/**
 * @brief		this function is used to process battery power.
 * 				The low voltage protection threshold 2.0V is an example and reference value. Customers should
 * 				evaluate and modify these thresholds according to the actual situation. If users have unreasonable designs
 * 				in the hardware circuit, which leads to a decrease in the stability of the power supply network, the
 * 				safety thresholds must be increased as appropriate.
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void user_battery_power_check(u16 alarm_vol_mv)
{
	/*For battery-powered products, as the battery power will gradually drop, when the voltage is low to a certain
	  value, it will cause many problems.
		a) When the voltage is lower than operating voltage range of chip, chip can no longer guarantee stable operation.
		b) When the battery voltage is low, due to the unstable power supply, the write and erase operations
			of Flash may have the risk of error, causing the program firmware and user data to be modified abnormally,
			and eventually causing the product to fail. */
	u8 battery_check_returnValue=0;
	if(analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG){
		battery_check_returnValue = app_battery_power_check(alarm_vol_mv+200);
	}
	else{
		battery_check_returnValue = app_battery_power_check(alarm_vol_mv);
	}
	if(battery_check_returnValue)
	{
		analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG) & (~LOW_BATT_FLG));  //clr
	}
	else
	{
		#if (UI_LED_ENABLE)  //led indicate
			for(int k=0;k<3;k++){
				gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
				sleep_us(200000);
				gpio_write(GPIO_LED_RED, !LED_ON_LEVEL);
				sleep_us(200000);
			}
		#endif

		if(analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG){
			tlkapi_printf(APP_BATT_CHECK_LOG_EN, "[APP][BAT] The battery voltage is lower than %dmV, shut down!!!\n", (alarm_vol_mv + 200));
		} else {
			tlkapi_printf(APP_BATT_CHECK_LOG_EN, "[APP][BAT] The battery voltage is lower than %dmV, shut down!!!\n", alarm_vol_mv);
		}


		analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG) | LOW_BATT_FLG);  //mark

		#if (UI_KEYBOARD_ENABLE)
		u32 pin[] = KB_DRIVE_PINS;
		for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
		{
			cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high wakeup deepsleep
		}

		cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
		#endif
	}
}

#endif




///////////////////////////////////////////

/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
//////////////////////////// basic hardware Initialization  Begin //////////////////////////////////

	#if(UART_PRINT_DEBUG_ENABLE)
		tlkapi_debug_init();
		blc_debug_enableStackLog(STK_LOG_DISABLE);
	#endif

	blc_readFlashSize_autoConfigCustomFlashSector();

	/* attention that this function must be called after "blc_readFlashSize_autoConfigCustomFlashSector" !!!*/
	blc_app_loadCustomizedParameters_normal();

	/* attention that this function must be called after "blc_app_loadCustomizedParameters_normal" !!!
	   The reason is that the low battery check need the ADC calibration parameter, and this parameter
	   is loaded in blc_app_loadCustomizedParameters_normal.
	 */
	#if (APP_BATT_CHECK_ENABLE)
		user_battery_power_check(VBAT_DEEP_THRES_MV);
	#endif


	#if (APP_FLASH_PROTECTION_ENABLE)
		app_flash_protection_operation(FLASH_OP_EVT_APP_INITIALIZATION, 0, 0);
		blc_appRegisterStackFlashOperationCallback(app_flash_protection_operation); //register flash operation callback for stack
	#endif
//////////////////////////// basic hardware Initialization  End //////////////////////////////////

//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

	u8  mac_public[6];
	u8  mac_random_static[6];
	/* for 512K Flash, flash_sector_mac_address equals to 0x76000, for 1M  Flash, flash_sector_mac_address equals to 0xFF000 */
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


	//////////// LinkLayer Initialization  Begin /////////////////////////
	blc_ll_initBasicMCU();
	blc_ll_initStandby_module(mac_public);						   //mandatory
    blc_ll_initLegacyAdvertising_module(); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initAclConnection_module();
	blc_ll_initAclPeriphrRole_module();



	blc_ll_setMaxConnectionNumber( ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_MASTER_MAX_TX_OCTETS, ACL_SLAVE_MAX_TX_OCTETS);

	/* all ACL connection share same RX FIFO */
	blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
	/* ACL Slave TX FIFO */
	blc_ll_initAclPeriphrTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE, ACL_SLAVE_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);


	rf_set_power_level_index (RF_POWER_P3dBm);


	//////////// LinkLayer Initialization  End /////////////////////////



	//////////// HCI Initialization  Begin /////////////////////////
	blc_hci_registerControllerDataHandler (blc_l2cap_pktHandler);

	blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

	//bluetooth event
	blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE);

	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd(		HCI_LE_EVT_MASK_CONNECTION_COMPLETE  \
									|	HCI_LE_EVT_MASK_ADVERTISING_REPORT \
									|   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);
	//////////// HCI Initialization  End /////////////////////////


	//////////// Host Initialization  Begin /////////////////////////
	/* Host Initialization */
	/* GAP initialization must be done before any other host feature initialization !!! */
	blc_gap_init();

	/* L2CAP buffer Initialization */
	blc_l2cap_initAclConnSlaveMtuBuffer(mtu_s_rx_fifo, MTU_S_BUFF_SIZE_MAX, mtu_s_tx_fifo, MTU_S_BUFF_SIZE_MAX);

	blc_att_setSlaveRxMTUSize(ATT_MTU_SLAVE_RX_MAX_SIZE); ///must be placed after "blc_gap_init"

	/* GATT Initialization */
	my_gatt_init();

	blc_gatt_register_data_handler(app_gatt_data_handler);

	/* SMP Initialization */
	#if (ACL_PERIPHR_SMP_ENABLE)
		blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
	#endif

	#if (ACL_PERIPHR_SMP_ENABLE)  //Slave SMP Enable
		blc_smp_setSecurityLevel_slave(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
	#else
		blc_smp_setSecurityLevel_slave(No_Security);
	#endif

	blc_smp_smpParamInit();


	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PAIRING_BEGIN 			|  \
						  GAP_EVT_MASK_SMP_PAIRING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PAIRING_FAIL				|  \
						  GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);
	//////////// Host Initialization  End /////////////////////////

	/* Check if any Stack(Controller & Host) Initialization error after all BLE initialization done.
	 * attention that code will stuck in "while(1)" if any error detected in initialization, user need find what error happens and then fix it */
	blc_app_checkControllerHostInitialization();
//////////////////////////// BLE stack Initialization  End //////////////////////////////////

	#if (BLE_APP_PM_ENABLE)
		blc_ll_initPowerManagement_module();
		blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_ACL_SLAVE);

		#if (PM_DEEPSLEEP_RETENTION_ENABLE)
			blc_pm_setDeepsleepRetentionEnable(PM_DeepRetn_Enable);
			blc_pm_setDeepsleepRetentionThreshold(95);
			blc_app_setDeepsleepRetentionSramSize();

			blc_pm_setDeepsleepRetentionEarlyWakeupTiming(350);
		#else
			blc_pm_setDeepsleepRetentionEnable(PM_DeepRetn_Disable);
		#endif

		blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &task_suspend_exit);
	#endif


	#if (REMOTE_IR_LEARN_ENABLE)
		ir_learn_init();
	#endif


//////////////////////////// User Configuration for BLE application ////////////////////////////
	blc_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	blc_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
	blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable

	app_ui_init_normal();

#if (BLE_OTA_SERVER_ENABLE)
	blc_ota_initOtaServer_module();
	blc_ota_registerOtaStartCmdCb(app_enter_ota_mode);
#endif

}



/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void user_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)
	blc_app_loadCustomizedParameters_deepRetn();

	blc_ll_initBasicMCU();   //mandatory

	rf_set_power_level_index (RF_POWER_P0dBm);

	blc_ll_recoverDeepRetention();

#if (APP_BATT_CHECK_ENABLE)
	/* ADC settings will lost during deepsleep retention mode, so here need clear flag */
	battery_clear_adc_setting_flag();
#endif

	irq_enable();

	app_ui_init_deepRetn();
#endif
}




void app_process_power_management(void)
{

#if (BLE_APP_PM_ENABLE)
#if(BLE_AUDIO_ENABLE)
	if(ui_mic_enable)
#else
	if(0)
#endif
	{
		blc_pm_setSleepMask(PM_SLEEP_DISABLE);
	}

	#if(REMOTE_IR_ENABLE)
	else if( ir_send_ctrl.is_sending){
		blc_pm_setSleepMask(PM_SLEEP_DISABLE);
	}
	#endif

	#if(REMOTE_IR_LEARN_ENABLE)
		if( get_ir_learn_state() == 1){
			blc_pm_setSleepMask(PM_SLEEP_DISABLE);
		}
	#endif

	else
	{
		blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_ACL_SLAVE );

		int user_task_flg = ota_is_working;
		#if UI_KEYBOARD_ENABLE
			user_task_flg = user_task_flg || scan_pin_need || key_not_released;
			if(user_task_flg){
				blc_pm_setSleepMask(PM_SLEEP_DISABLE);
			}
		#endif


	}

#endif

}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop(void)
{

	////////////////////////////////////// BLE entry /////////////////////////////////
	blc_sdk_main_loop();

	///////////////////////////////////// Battery Check ////////////////////////////////
	#if (APP_BATT_CHECK_ENABLE)
		/*The frequency of low battery detect is controlled by the variable lowBattDet_tick, which is executed every
		 500ms in the demo. Users can modify this time according to their needs.*/
		if(battery_get_detect_enable() && clock_time_exceed(lowBattDet_tick, 500000) ){
			lowBattDet_tick = clock_time();
			#if(BLE_AUDIO_ENABLE)
			if(!ui_mic_enable)
			#endif
			{
				user_battery_power_check(VBAT_DEEP_THRES_MV);
			}

		}
	#endif

	////////////////////////////////////// UI entry /////////////////////////////////
	#if (BLE_AUDIO_ENABLE)
		proc_audio();
	#endif

	#if (UI_KEYBOARD_ENABLE)
		proc_keyboard(0, 0, 0);
	#endif

	#if (UI_LED_ENABLE)
		device_led_process();
	#endif


	////////////////////////////////////// PM entry /////////////////////////////////
	app_process_power_management();

	return 0; //must return 0 due to SDP flow
}



_attribute_no_inline_ void main_loop(void)
{
	main_idle_loop();
}




