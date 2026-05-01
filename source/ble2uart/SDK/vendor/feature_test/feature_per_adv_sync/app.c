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

#include "app_config.h"
#include "app.h"
#include "app_buffer.h"
#include "../default_att.h"
#include "app_ui.h"
#include "application/usbstd/usbkeycode.h"

#if (FEATURE_TEST_MODE == TEST_PER_ADV_SYNC)


int	central_smp_pending = 0; 		// SMP: security & encryption;






const u8	tbl_advData[] = {
	 8,  DT_COMPLETE_LOCAL_NAME, 				'f','e','a','t','u','r','e',
	 2,	 DT_FLAGS, 								0x05, 					// BLE limited discoverable mode and BR/EDR not supported
	 3,  DT_APPEARANCE, 						0x80, 0x01, 			// 384, Generic Remote Control, Generic category
	 5,  DT_INCOMPLT_LIST_16BIT_SERVICE_UUID,	0x12, 0x18, 0x0F, 0x18,	// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8	tbl_scanRsp [] = {
	 8, DT_COMPLETE_LOCAL_NAME, 				'f','e','a','t','u','r','e',
};







u32	tick_legadv_rpt = 0;

/**
 * @brief      LE Extended Advertising report event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_ext_adv_report_event_handle(u8 *p, int evt_data_len)
{


	hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

	int offset = 0;

	extAdvEvt_info_t *pExtAdvInfo = NULL;


	for(int i=0; i<pExtAdvRpt->num_reports ; i++)
	{

		pExtAdvInfo = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
		offset += (EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
		//s8 rssi = pExtAdvInfo->rssi;
		//TODO: add a function process data combine



		/* Legacy */
		if(pExtAdvInfo->event_type & EXTADV_RPT_EVT_MASK_LEGACY)
		{
			if(clock_time_exceed(tick_legadv_rpt, 4000 * 1000)){
				//tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]leg adv rpt", pExtAdvInfo, EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
				tick_legadv_rpt = clock_time();
			}
		}
		/* Extended */
		else
		{
			static int aux_adv_cnt = 0;
			if(!(aux_adv_cnt & 0x07)){  //every 16/32
//				tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]aux adv", pExtAdvInfo, EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
				tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]aux adv", pExtAdvInfo, EXTADV_INFO_LENGTH + 25);
			}
			//aux_adv_cnt = 0;
			aux_adv_cnt ++;


			if(pExtAdvInfo->perd_adv_inter != PERIODIC_ADV_INTER_NO_PERIODIC_ADV){
				//tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]aux adv with syncInfo", pExtAdvInfo, EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
				//tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]aux adv with syncInfo", 0, 0);


//				static int sync_cnt = 0;
//				if(sync_cnt){
//					return 0;
//				}
//				sync_cnt ++;

				#if 1
					u8 status = blc_ll_periodicAdvertisingCreateSync(SYNC_ADV_SPECIFY | REPORTING_INITIALLY_EN, pExtAdvInfo->advertising_sid, \
																	 pExtAdvInfo->address_type, pExtAdvInfo->address, 0, SYNC_TIMEOUT_2S, 0);

					if(status != BLE_SUCCESS){
						tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]pda sync cmd fail", &status, 1);
					}
					else{
						tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]pda sync start" , 0, 0);
						//blc_ll_setExtScanEnable( 0, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
					}
				#endif
			}

		}



//		u8 ext_evtType = pExtAdvInfo->event_type & EXTADV_RPT_EVTTYPE_MASK;



	}


	return 0;
}




/**
 * @brief     LE periodic advertising sync established event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_periodic_adv_sync_established_event_handle(u8 *p)
{
	hci_le_periodicAdvSyncEstablishedEvt_t *pExt = (hci_le_periodicAdvSyncEstablishedEvt_t *)p;

	tlkapi_send_string_u32s(APP_LOG_EN, "[APP][EVT]pda sync established ", &pExt->status, 15);

	return 0;
}

/**
 * @brief      LE periodic advertising report event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_periodic_adv_report_event_handle(u8 *p)
{


	return 0;
}

/**
 * @brief      LE periodic advertising sync lost event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_periodic_adv_sync_lost_event_handle(u8 *p)
{
	hci_le_periodicAdvSyncLostEvt_t *pExt = (hci_le_periodicAdvSyncLostEvt_t *)p;

	tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT]pda sync lost ", &pExt->syncHandle, 2);

	return 0;
}



/**
 * @brief      BLE enhanced connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_enhanced_connection_complete_event_handle(u8 *p)
{
	hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;

	if(pConnEvt->status == BLE_SUCCESS){

		dev_char_info_insert_by_enhanced_conn_event(pConnEvt);

		if( pConnEvt->role == ACL_ROLE_CENTRAL ) // master role, process SMP and SDP if necessary
		{
			#if (ACL_CENTRAL_SMP_ENABLE)
				central_smp_pending = pConnEvt->connHandle; // this connection need SMP
			#else

			#endif
		}
	}

	return 0;
}


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



	//if previous connection SMP & SDP not finished, clear flag
#if (ACL_CENTRAL_SMP_ENABLE)
	if(central_smp_pending == pCon->connHandle){
		central_smp_pending = 0;
	}
#endif

	if(central_disconnect_connhandle == pCon->connHandle){  //un_pair disconnection flow finish, clear flag
		central_disconnect_connhandle = 0;
	}

	dev_char_info_delete_by_connhandle(pCon->connHandle);




	return 0;
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
				/* when enhanced connection complete event enable, connection complete event should never come */
				// while(1);
			}
			//------hci le event: le enhanced_connection complete event---------------------------------
			if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE)	// connection complete
			{
				app_le_enhanced_connection_complete_event_handle(p);
			}
			//--------hci le event: le adv report event ----------------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{

			}
			//------hci le event: LE connection update complete event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)	// connection update
			{

			}
			//------hci le event: LE extended advertising report event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT)	// ADV packet
			{
				app_le_ext_adv_report_event_handle(p, n);
			}
			//------hci le event: LE periodic advertising sync established event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED)
			{
				app_le_periodic_adv_sync_established_event_handle(p);
			}
			//------hci le event: LE periodic advertising report event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT)	// ADV packet
			{
				app_le_periodic_adv_report_event_handle(p);
			}
			//------hci le event: LE periodic advertising sync lost event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST)	// ADV packet
			{
				app_le_periodic_adv_sync_lost_event_handle(p);
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
		#if (ACL_CENTRAL_SMP_ENABLE)
			gap_smp_pairingFailEvt_t *p = (gap_smp_pairingFailEvt_t *)para;

			if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){  //master connection
				if(central_smp_pending == p->connHandle){
					central_smp_pending = 0;
				}
			}
		#endif
		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{

		}
		break;

		case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
		{
		#if (ACL_CENTRAL_SMP_ENABLE)
			gap_smp_securityProcessDoneEvt_t* p = (gap_smp_securityProcessDoneEvt_t*)para;

			if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){  //master connection

				if(central_smp_pending == p->connHandle){
					central_smp_pending = 0;
				}

			}
		#endif
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
 * @brief       This function is used to send consumer HID report by USB.
 * @param[in]   conn     - connection handle
 * @param[in]   p        - Pointer point to data buffer.
 * @return
 */
void	att_keyboard_media (u16 conn, u8 *p)
{
	u16 consumer_key = p[0] | p[1]<<8;


#if (1 && UI_LED_ENABLE)	//Demo effect: when slave send Vol+/Vol- to master, LED GPIO toggle to show the result
	if(consumer_key == MKEY_VOL_UP){
		gpio_toggle(GPIO_LED_GREEN);
	}
	else if(consumer_key == MKEY_VOL_DN){
		gpio_toggle(GPIO_LED_BLUE);
	}
#endif
}

#define			HID_HANDLE_CONSUME_REPORT			25
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
			if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //slave handle notify
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


















///////////////////////////////////////////

/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
//////////////////////////// basic hardware Initialization  Begin //////////////////////////////////

	/* random number generator must be initiated here( in the beginning of user_init_nromal).
	 * When deepSleep retention wakeUp, no need initialize again */
	#if(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
	random_generator_init();
#endif

	#if(UART_PRINT_DEBUG_ENABLE)
		tlkapi_debug_init();
		blc_debug_enableStackLog(STK_LOG_DISABLE);
	#endif

	blc_readFlashSize_autoConfigCustomFlashSector();

	/* attention that this function must be called after "blc_readFlashSize_autoConfigCustomFlashSector" !!!*/
	blc_app_loadCustomizedParameters_normal();

//////////////////////////// basic hardware Initialization  End //////////////////////////////////

//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

	u8  mac_public[6];
	u8  mac_random_static[6];
	/* for 512K Flash, flash_sector_mac_address equals to 0x76000, for 1M  Flash, flash_sector_mac_address equals to 0xFF000 */
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


	//////////// LinkLayer Initialization  Begin /////////////////////////
	blc_ll_initBasicMCU();

	blc_ll_initStandby_module(mac_public);

    blc_ll_initLegacyAdvertising_module();

    blc_ll_initExtendedScanning_module();

    blc_ll_initExtendedInitiating_module();

	blc_ll_initPeriodicAdvertisingSynchronization_module();


	blc_ll_initAclConnection_module();
	blc_ll_initAclCentralRole_module();
	blc_ll_initAclPeriphrRole_module();

	blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_MASTER_MAX_TX_OCTETS, ACL_SLAVE_MAX_TX_OCTETS);

	/* all ACL connection share same RX FIFO */
	blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
	/* ACL Master TX FIFO */
	blc_ll_initAclCentralTxFifo(app_acl_mstTxfifo, ACL_MASTER_TX_FIFO_SIZE, ACL_MASTER_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);
	/* ACL Slave TX FIFO */
	blc_ll_initAclPeriphrTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE, ACL_SLAVE_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);

	blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_31P25MS);




	//////////// LinkLayer Initialization  End /////////////////////////



	//////////// HCI Initialization  Begin /////////////////////////
	blc_hci_registerControllerDataHandler (blc_l2cap_pktHandler);

	blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

	//bluetooth event
	blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE);

	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd(		HCI_LE_EVT_MASK_CONNECTION_COMPLETE  \
									|	HCI_LE_EVT_MASK_ADVERTISING_REPORT \
									|   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE \
									|	HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE \
									|   HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT \
									|   HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_ESTABLISHED \
									|	HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT \
									|	HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_LOST);
	//////////// HCI Initialization  End /////////////////////////


	//////////// Host Initialization  Begin /////////////////////////
	/* Host Initialization */
	/* GAP initialization must be done before any other host feature initialization !!! */
	blc_gap_init();

	/* L2CAP Initialization */
	blc_l2cap_initAclConnMasterMtuBuffer(mtu_m_rx_fifo, MTU_M_BUFF_SIZE_MAX, 			0,					 0);
	blc_l2cap_initAclConnSlaveMtuBuffer(mtu_s_rx_fifo, MTU_S_BUFF_SIZE_MAX, mtu_s_tx_fifo, MTU_S_BUFF_SIZE_MAX);

	blc_att_setMasterRxMTUSize(ATT_MTU_MASTER_RX_MAX_SIZE); ///must be placed after "blc_gap_init"
	blc_att_setSlaveRxMTUSize(ATT_MTU_SLAVE_RX_MAX_SIZE);   ///must be placed after "blc_gap_init"

	/* GATT Initialization */
	my_gatt_init();
	blc_gatt_register_data_handler(app_gatt_data_handler);

	/* SMP Initialization */
	#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
		blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
	#endif

	#if (ACL_PERIPHR_SMP_ENABLE)  //Slave SMP Enable
		blc_smp_setSecurityLevel_slave(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
	#else
		blc_smp_setSecurityLevel_slave(No_Security);
	#endif

	#if (ACL_CENTRAL_SMP_ENABLE) //Master SMP Enable
		blc_smp_setSecurityLevel_master(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
	#else
		blc_smp_setSecurityLevel_master(No_Security);
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



//////////////////////////// User Configuration for BLE application ////////////////////////////
	rf_set_power_level_index (RF_POWER_P3dBm);

	blc_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	blc_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
	blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	//blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable





	blc_ll_setExtScanParam( OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M_CODED, \
							SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_90MS,   SCAN_WINDOW_90MS, \
							SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_90MS,   SCAN_WINDOW_90MS);

	blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
}



/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
void user_init_deepRetn(void)
{

}


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop(void)
{

	//sleep_ms(20);  //make RX FIFO block, to see RX report combination effect

	////////////////////////////////////// BLE entry /////////////////////////////////
	blc_sdk_main_loop();


	////////////////////////////////////// UI entry /////////////////////////////////
	#if (UI_KEYBOARD_ENABLE)
		proc_keyboard(0, 0, 0);
	#endif


	return 0; //must return 0 due to SDP flow
}



_attribute_no_inline_ void main_loop(void)
{
	main_idle_loop();
}

#endif



