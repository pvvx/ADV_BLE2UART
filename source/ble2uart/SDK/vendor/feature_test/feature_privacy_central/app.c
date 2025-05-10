/********************************************************************************************************
 * @file    app.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_config.h"
#include "app.h"
#include "app_buffer.h"
#include "../default_att.h"
#include "app_ui.h"


#if (FEATURE_TEST_MODE == TEST_PRIVACY_CENTRAL)




_attribute_ble_data_retention_		int	central_smp_pending = 0; 		// SMP: security & encryption;




/**
 * @brief	BLE Advertising data
 */
const u8	tbl_advData[] = {
	 8,  DT_COMPLETE_LOCAL_NAME, 				'p','r','i','v','a','c','y',
	 2,	 DT_FLAGS, 								0x05, 					// BLE limited discoverable mode and BR/EDR not supported
	 3,  DT_APPEARANCE, 						0x80, 0x01, 			// 384, Generic Remote Control, Generic category
	 5,  DT_INCOMPLT_LIST_16BIT_SERVICE_UUID,	0x12, 0x18, 0x0F, 0x18,	// incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief	BLE Scan Response Packet data
 */
const u8	tbl_scanRsp [] = {
	 8, DT_COMPLETE_LOCAL_NAME, 				'p','r','i','v','a','c','y',
};





/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int AA_dbg_adv_rpt = 0;
u32	tick_adv_rpt = 0;

int app_le_adv_report_event_handle(u8 *p)
{
	event_adv_report_t *pa = (event_adv_report_t *)p;
	s8 rssi = pa->data[pa->len];

	#if 1  //debug, print ADV report number every 5 seconds
		AA_dbg_adv_rpt ++;
		if(clock_time_exceed(tick_adv_rpt, 5000000)){
			tlkapi_send_string_data(1, "[APP][EVT] Adv report", pa->mac, 6);
			tick_adv_rpt = clock_time();
		}
	#endif

	/*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
	#if (ACL_CENTRAL_SMP_ENABLE)
		if(central_smp_pending){ 	 //if previous connection SMP not finish, can not create a new connection
			return 1;
		}
	#endif

	#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
		if(central_sdp_pending){ 	 //if previous connection SDP not finish, can not create a new connection
			return 1;
		}
	#endif

	if (central_disconnect_connhandle){ //one ACL connection central role is in un_pair disconnection flow, do not create a new one
		return 1;
	}

	int central_auto_connect = 0;
	int user_manual_pairing = 0;

	//manual pairing methods 1: key press triggers
	user_manual_pairing = central_pairing_enable && (rssi > -56);  //button trigger pairing(RSSI threshold, short distance)

	#if (ACL_CENTRAL_SMP_ENABLE)
		central_auto_connect = blc_smp_searchBondingSlaveDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
	#endif

	if(central_auto_connect || user_manual_pairing){

		/* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
		 * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
		 * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
		 * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
	  tlkapi_printf(APP_LOG_EN,"le_createConnection mac:%02X %02X %02X %02X %02X %02X",\
				pa->mac[0],pa->mac[1],pa->mac[2],\
				pa->mac[3],pa->mac[4],pa->mac[5]);

		u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY,  \
								 pa->adr_type, pa->mac, OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC, \
								 CONN_INTERVAL_31P25MS, CONN_INTERVAL_48P75MS, 0, CONN_TIMEOUT_4S, \
								 0, 0xFFFF);


		if(status == BLE_SUCCESS){ //create connection success

		}
		else
		{
		  tlkapi_printf(APP_LOG_EN,"blc_ll_createConnection error code :%02X",status);
		}
	}
	/*********************** Central Create connection demo code end  *******************************************************/


	return 0;
}


/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_complete_event_handle(u8 *p)
{
	hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;

	if(pConnEvt->status == BLE_SUCCESS){

		dev_char_info_insert_by_conn_event(pConnEvt);

		if(pConnEvt->role == ACL_ROLE_CENTRAL) // central role, process SMP and SDP if necessary
		{
			#if (ACL_CENTRAL_SMP_ENABLE)
				central_smp_pending = pConnEvt->connHandle; // this connection need SMP
			#endif



			#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
				memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
				cur_sdp_device.conn_handle = pConnEvt->connHandle;
				cur_sdp_device.peer_adrType = pConnEvt->peerAddrType;
				memcpy(cur_sdp_device.peer_addr, pConnEvt->peerAddr, 6);

				u8	temp_buff[sizeof(dev_att_t)];
				dev_att_t *pdev_att = (dev_att_t *)temp_buff;

				/* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
				if( dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->peerAddrType, pConnEvt->peerAddr, pdev_att) ){
					//cur_sdp_device.char_handle[1] = 									//Speaker
					cur_sdp_device.char_handle[2] = pdev_att->char_handle[2];			//OTA
					cur_sdp_device.char_handle[3] = pdev_att->char_handle[3];			//consume report
					cur_sdp_device.char_handle[4] = pdev_att->char_handle[4];			//normal key report
					//cur_sdp_device.char_handle[6] =									//BLE Module, SPP Server to Client
					//cur_sdp_device.char_handle[7] =									//BLE Module, SPP Client to Server

					/* add the peer device att_handle value to conn_dev_list */
					dev_char_info_add_peer_att_handle(&cur_sdp_device);
				}
				else
				{
					central_sdp_pending = pConnEvt->connHandle;  // mark this connection need SDP

					#if (ACL_CENTRAL_SMP_ENABLE)
						 //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
					#else
						 app_register_service(&app_service_discovery); 	//No SMP, service discovery can initiated now
					#endif
				}
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
	hci_disconnectionCompleteEvt_t	*pDisConn = (hci_disconnectionCompleteEvt_t *)p;
	//tlkapi_printf(APP_CONTR_EVT_LOG_EN,"app Disconnect event connHandle:%04X reason:%02X",pDisConn->connHandle,pDisConn->reason);

	//terminate reason
	if(pDisConn->reason == HCI_ERR_CONN_TIMEOUT){  	//connection timeout

	}
	else if(pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN){  	//peer device send terminate command on link layer

	}
	//central host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
	else if(pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){

	}
	else{

	}


	/* if previous connection SMP & SDP not finished, clear flag */
	#if (ACL_CENTRAL_SMP_ENABLE)
		if(central_smp_pending == pDisConn->connHandle){
			central_smp_pending = 0;
		}
	#endif
	#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
		if(central_sdp_pending == pDisConn->connHandle){
			central_sdp_pending = 0;
		}
	#endif

	if(central_disconnect_connhandle == pDisConn->connHandle){  //un_pair disconnection flow finish, clear flag
		central_disconnect_connhandle = 0;
	}

	dev_char_info_delete_by_connhandle(pDisConn->connHandle);

	#if(PRIVACY_TEST_MODE == LEGACY_SCAN_TEST)
		app_configLegacyScanParam();
	#elif(PRIVACY_TEST_MODE == EXTEND_SCAN_TEST)

	#endif

	return 0;
}


/**
 * @brief      BLE Connection update complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_update_complete_event_handle(u8 *p)
{
	hci_le_connectionUpdateCompleteEvt_t *pUpt = (hci_le_connectionUpdateCompleteEvt_t *)p;
	//tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection Update Event", &pUpt->connHandle, 8);

	if(pUpt->status == BLE_SUCCESS){

	}

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
int app_controller_event_callback (u32 h, u8 *p, int n)
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

				app_le_adv_report_event_handle(p);
			}
			//------hci le event: le connection update complete event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)	// connection update
			{
				app_le_connection_update_complete_event_handle(p);
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
int app_host_event_callback (u32 h, u8 *para, int n)
{
	u8 event = h & 0xFF;

	switch(event)
	{
		case GAP_EVT_SMP_PAIRING_BEGIN:
		{
			tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] Pairing Begin",0,0);
		}
		break;

		case GAP_EVT_SMP_PAIRING_SUCCESS:
		{
			gap_smp_pairingSuccessEvt_t* p = (gap_smp_pairingSuccessEvt_t*)para;
			//tlkapi_send_string_u8s(APP_LOG_EN,"[APP][SMP] Pairing success,bond flg", p->bonding ?1:0,0,0,0);

			if(p->bonding_result){
				tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] save smp key succ",0,0);
			}
			else{
				tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] save smp key failed",0,0);
			}
		}
		break;

		case GAP_EVT_SMP_PAIRING_FAIL:
		{
			#if (ACL_CENTRAL_SMP_ENABLE)
				gap_smp_pairingFailEvt_t *pEvt = (gap_smp_pairingFailEvt_t *)para;

				if( dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL){
					if(central_smp_pending == pEvt->connHandle){
						central_smp_pending = 0;
						tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] paring fail", &pEvt->connHandle, sizeof(gap_smp_pairingFailEvt_t));
					}
				}
			#endif
		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{
			gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)para;
			tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Connection encryption done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));
		}
		break;

		case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
		{
			gap_smp_connEncDoneEvt_t* pEvt = (gap_smp_connEncDoneEvt_t*)para;
			tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Security process done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));

			if( dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL){

				#if (ACL_CENTRAL_SMP_ENABLE)
					if( dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL){
						if(central_smp_pending == pEvt->connHandle){
							central_smp_pending = 0;
						}
					}
				#endif

				#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)  //SMP finish
					if(central_sdp_pending == pEvt->connHandle){  //SDP is pending
						app_register_service(&app_service_discovery);  //start SDP now
					}
				#endif
			}
		}
		break;

		case GAP_EVT_SMP_TK_DISPLAY:
		{

		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
		{

		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_OOB:
		{

		}
		break;

		case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
		{

		}
		break;

		case GAP_EVT_ATT_EXCHANGE_MTU:
		{

		}
		break;

		case GAP_EVT_GATT_HANDLE_VLAUE_CONFIRM:
		{

		}
		break;

		default:
		break;
	}

	return 0;
}



#define			HID_HANDLE_CONSUME_REPORT			25
#define			HID_HANDLE_KEYBOARD_REPORT			29
#define			AUDIO_HANDLE_MIC					52
#define			OTA_HANDLE_DATA						48

/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler (u16 connHandle, u8 *pkt)
{
	if( dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL )   //GATT data for Central
	{
		#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
			if(central_sdp_pending == connHandle ){  //ATT service discovery is ongoing on this conn_handle
				//when service discovery function is running, all the ATT data from peripheral
				//will be processed by it,  user can only send your own att cmd after  service discovery is over
				host_att_client_handler (connHandle, pkt); //handle this ATT data by service discovery process
			}
		#endif

		rf_packet_att_t *pAtt = (rf_packet_att_t*)pkt;

		//so any ATT data before service discovery will be dropped
		dev_char_info_t* dev_info = dev_char_info_search_by_connhandle (connHandle);
		if(dev_info)
		{
			//-------	user process ------------------------------------------------
			u16 attHandle = pAtt->handle;

			if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //peripheral handle notify
			{
					//---------------	consumer key --------------------------
				#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
					if(attHandle == dev_info->char_handle[3])  // Consume Report In (Media Key)
				#else
					if(attHandle == HID_HANDLE_CONSUME_REPORT)   //Demo device(825x_ble_sample) Consume Report AttHandle value is 25
				#endif
					{
						//att_keyboard_media (connHandle, pAtt->dat);
					}
					//---------------	keyboard key --------------------------
				#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
					else if(attHandle == dev_info->char_handle[4])     // Key Report In
				#else
					else if(attHandle == HID_HANDLE_KEYBOARD_REPORT)   // Demo device(825x_ble_sample) Key Report AttHandle value is 29
				#endif
					{
						//att_keyboard (connHandle, pAtt->dat);
					}
				#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
					else if(attHandle == dev_info->char_handle[0])     // AUDIO Notify
				#else
					else if(attHandle == AUDIO_HANDLE_MIC)   // Demo device(825x_ble_remote) Key Report AttHandle value is 52
				#endif
					{

					}
					else
					{

					}
			}
			else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
			{
				blc_gatt_pushConfirm(connHandle);
			}
		}

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
	/* random number generator must be initiated here( in the beginning of user_init_normal).
	 * When deepSleep retention wakeUp, no need initialize again */
	#if(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
	random_generator_init();
#endif

	#if (UART_PRINT_DEBUG_ENABLE)
		tlkapi_debug_init();
		blc_debug_enableStackLog(STK_LOG_DISABLE);
	#endif

	blc_readFlashSize_autoConfigCustomFlashSector();

	/* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
	blc_app_loadCustomizedParameters_normal();
//////////////////////////// basic hardware Initialization  End /////////////////////////////////


//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

	u8  mac_public[6];
	u8  mac_random_static[6];
	/* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


	//////////// LinkLayer Initialization  Begin /////////////////////////
	blc_ll_initBasicMCU();

	blc_ll_initStandby_module(mac_public);

    blc_ll_initLegacyScanning_module();

    blc_ll_initLegacyInitiating_module();

	blc_ll_initAclConnection_module();
	blc_ll_initAclCentralRole_module();


	blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_MASTER_MAX_TX_OCTETS, ACL_SLAVE_MAX_TX_OCTETS);

	/* all ACL connection share same RX FIFO */
	blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
	/* ACL Master TX FIFO */
	blc_ll_initAclCentralTxFifo(app_acl_mstTxfifo, ACL_MASTER_TX_FIFO_SIZE, ACL_MASTER_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);

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
									|   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);


	u8 error_code = blc_contr_checkControllerInitialization();
	if(error_code != INIT_SUCCESS){
		/* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
		write_log32(0x88880000 | error_code);
		while(1);
	}
	//////////// HCI Initialization  End /////////////////////////


	//////////// Host Initialization  Begin /////////////////////////
	/* Host Initialization */
	/* GAP initialization must be done before any other host feature initialization !!! */
	blc_gap_init();

	/* L2CAP data buffer Initialization */
	blc_l2cap_initAclConnMasterMtuBuffer(mtu_m_rx_fifo, MTU_M_BUFF_SIZE_MAX, 			0,					 0);
	blc_l2cap_initAclConnSlaveMtuBuffer(mtu_s_rx_fifo, MTU_S_BUFF_SIZE_MAX, mtu_s_tx_fifo, MTU_S_BUFF_SIZE_MAX);

	blc_att_setMasterRxMTUSize(ATT_MTU_MASTER_RX_MAX_SIZE); ///must be placed after "blc_gap_init"
	blc_att_setSlaveRxMTUSize(ATT_MTU_SLAVE_RX_MAX_SIZE);   ///must be placed after "blc_gap_init"

	/* GATT Initialization */
	my_gatt_init();
	#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
		host_att_register_idle_func (main_idle_loop);
	#endif
	blc_gatt_register_data_handler(app_gatt_data_handler);

	/* SMP Initialization */
	#if (ACL_CENTRAL_SMP_ENABLE)
		/* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
		blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
	#endif

	#if (ACL_CENTRAL_SMP_ENABLE)
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

//////////////////////////// BLE stack Initialization  End //////////////////////////////////




//////////////////////////// User Configuration for BLE application ////////////////////////////
	#if(PRIVACY_TEST_MODE == LEGACY_SCAN_TEST)
		app_configLegacyScanParam();
	#elif(PRIVACY_TEST_MODE == EXTEND_SCAN_TEST)

	#endif

	#if (BLE_APP_PM_ENABLE)
		blc_ll_initPowerManagement_module();
		blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_PERIPHR | PM_SLEEP_ACL_CENTRAL);
	#endif


	tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] FEATURE_PRIVACY init", 0, 0);
////////////////////////////////////////////////////////////////////////////////////////////////
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
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop (void)
{

	////////////////////////////////////// BLE entry /////////////////////////////////
	blc_sdk_main_loop();

	////////////////////////////////////// UI entry /////////////////////////////////
	#if (UI_KEYBOARD_ENABLE)
		proc_keyboard (0, 0, 0);
	#endif


	proc_central_role_unpair();


	return 0; //must return 0 due to SDP flow
}



/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop (void)
{
	main_idle_loop ();

	#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
		simple_sdp_loop ();
	#endif
}

#endif //end of (FEATURE_TEST_MODE == ...)



