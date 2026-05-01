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

#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"


#if (FEATURE_TEST_MODE == TEST_LL_MD)


int	central_smp_pending = 0; 		// SMP: security & encryption;








const u8	tbl_advData[] = {
	 3,  DT_COMPLETE_LOCAL_NAME, 				'm', 'd',
	 2,	 DT_FLAGS, 								0x05, 					// BLE limited discoverable mode and BR/EDR not supported
	 3,  DT_APPEARANCE, 						0x80, 0x01, 			// 384, Generic Remote Control, Generic category
	 5,  DT_INCOMPLT_LIST_16BIT_SERVICE_UUID,	0x12, 0x18, 0x0F, 0x18,	// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8	tbl_scanRsp [] = {
	 3,  DT_COMPLETE_LOCAL_NAME, 				'm', 'd',
};



int central_disconnect_connhandle;
int central_unpair_enable = 0;
int	central_pairing_enable = 0;
int muliti_ll_md_start = 0;
u32 muliti_ll_md_stopTick = 0;
u8  muliti_ll_md_stop = 0;
u8  pushTxStopStart[MASTER_ACL_PERIPHR_MAX_NUM];

const u8 zero_all[MASTER_ACL_PERIPHR_MAX_NUM] = { 0};




#define 	TEST_DATA_LEN		20
u8	app_test_data[MASTER_ACL_PERIPHR_MAX_NUM][TEST_DATA_LEN];

u8	app_test_start[MASTER_ACL_PERIPHR_MAX_NUM];

u32 device_connection_tick[MASTER_ACL_PERIPHR_MAX_NUM];



u8 write_cmd_seq_num_next;
u8 notify_seq_num_next;

u8 currRcvdSeqNo[MASTER_ACL_PERIPHR_MAX_NUM] = {0};

int AA_dbg_write_cmd_err = 0;
int AA_dbg_notify_err = 0;


const u32 gpio_table[4] = {GPIO_LED_RED, GPIO_LED_WHITE, GPIO_LED_GREEN, GPIO_LED_BLUE};




void proc_master_role_unpair(void)
{



	//terminate and un_pair process, Telink demonstration effect: triggered by "un_pair" key press
	if(central_unpair_enable){

		dev_char_info_t* dev_char_info = dev_char_info_search_by_connhandle(central_unpair_enable); //connHandle has marked on on central_unpair_enable

		if( dev_char_info ){ //un_pair device in still in connection state

			if(blc_ll_disconnect(central_unpair_enable, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS){

				central_unpair_enable = 0;  //every "un_pair" key can only triggers one connection disconnect
				central_disconnect_connhandle = central_unpair_enable; //mark conn_handle

				// delete this device information(mac_address and distributed keys...) on FLash
				#if (ACL_CENTRAL_SMP_ENABLE)
					blc_smp_deleteBondingSlaveInfo_by_PeerMacAddress(dev_char_info->peer_adrType, dev_char_info->peer_addr);
				#endif
			}

		}
		else{ //un_pair device can not find in device list, it's not connected now

			central_unpair_enable = 0;  //every "un_pair" key can only triggers one connection disconnect
		}

	}
}

#if (UI_KEYBOARD_ENABLE)



#define CONSUMER_KEY   	   			1
#define KEYBOARD_KEY   	   			2
#define PAIR_UNPAIR_KEY   	   		3

_attribute_ble_data_retention_	u8 		key_type;


void key_change_proc(void)
{

	u8 key0 = kb_event.keycode[0];

	if (kb_event.cnt == 2)   //two key press
	{

	}
	else if(kb_event.cnt == 1)
	{
		if(key0 == BTN_PAIR)   //Manual pair triggered by Key Press
		{
			central_pairing_enable = 1;
			key_type = PAIR_UNPAIR_KEY;
		}
		else if(key0 == BTN_UNPAIR){

			key_type = PAIR_UNPAIR_KEY;

			/*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, only one "un_pair" key is
			 available. When "un_pair" key pressed, we will choose and un_pair one device in connection state */
			if(acl_conn_central_num){ //at least 1 master connection exist

				if(!central_disconnect_connhandle){  //if one master un_pair disconnection flow not finish, here new un_pair not accepted

					// We choose the earliest connected device to un_pair, conn_dev_list[0] is the earliest connected device.
					// Attention: when acl_conn_central_num none zero, conn_dev_list[0].conn_state is definite 1, no need to judge
					central_unpair_enable = conn_dev_list[0].conn_handle;  //mark connHandle on central_unpair_enable
				}
			}

		}
		else if(key0 == CR_VOL_UP){
			key_type = CONSUMER_KEY;

			if(!muliti_ll_md_stopTick || clock_time_exceed(muliti_ll_md_stopTick , 4000000)){
				muliti_ll_md_stopTick = 0;

				if(!muliti_ll_md_start){ //if already start LL MD test,do nothing
					for(int i=0; i< MASTER_ACL_PERIPHR_MAX_NUM; i++)
					{
						if(app_test_start[i]){  //connection state
							muliti_ll_md_start = 1;

							///////////////set start flg///////////////
							memset(app_test_data, 0, sizeof(app_test_data));
							memset(currRcvdSeqNo, 0, sizeof(currRcvdSeqNo));
							for(int i=0; i< MASTER_ACL_PERIPHR_MAX_NUM; i++){
								app_test_data[i][4] = 1; //start
							}
							break;
						}
					}
				}
			}
		}
		else if(key0 == CR_VOL_DN){
			key_type = CONSUMER_KEY;

			if(muliti_ll_md_start){ //if already stop LL MD test,do nothing
				muliti_ll_md_start = 0;
				muliti_ll_md_stop = 1;
				memcpy(pushTxStopStart, app_test_start, sizeof(pushTxStopStart));
			}
		}
	}
	else   //kb_event.cnt == 0,  key release
	{
		if(key_type == CONSUMER_KEY)
		{

		}
		else if(key_type == KEYBOARD_KEY)
		{

		}
		else if(key_type == PAIR_UNPAIR_KEY){
			if(central_pairing_enable){
				central_pairing_enable = 0;
			}

			if(central_unpair_enable){
				central_unpair_enable = 0;
			}
		}
	}


}



static u32 keyScanTick = 0;


void proc_keyboard(u8 e, u8 *p, int n)
{
	if(clock_time_exceed(keyScanTick, 2000)){
		keyScanTick = clock_time();
	}
	else{
		return;
	}

	kb_event.keycode[0] = 0;
	int det_key = kb_scan_key (0, 1);

	if (det_key){
		key_change_proc();
	}
}

#endif

/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_adv_report_event_handle(u8 *p)
{
	event_adv_report_t *pa = (event_adv_report_t *)p;
	s8 rssi = pa->data[pa->len];


	/*********************** Master Create connection demo: Key press or ADV pair packet triggers pair  ********************/
	if(central_smp_pending ){ 	 //if previous connection SMP not finish, can not create a new connection
		return 1;
	}
	if (central_disconnect_connhandle){ //one master connection is in un_pair disconnection flow, do not create a new one
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
		u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY,  \
								 pa->adr_type, pa->mac, OWN_ADDRESS_PUBLIC, \
								 CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, 0, CONN_TIMEOUT_4S, \
								 0, 0xFFFF);


		if(status == BLE_SUCCESS){ //create connection success

		}
	}
	/*********************** Master Create connection demo code end  *******************************************************/


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

		u8 conn_idx = dev_char_info_insert_by_conn_event(pConnEvt);

		if( pConnEvt->role == ACL_ROLE_CENTRAL ) // master role, process SMP and SDP if necessary
		{
			#if (ACL_CENTRAL_SMP_ENABLE)
				central_smp_pending = pConnEvt->connHandle; // this connection need SMP
			#endif
		}



		device_connection_tick[conn_idx] = clock_time() | 1;
		app_test_data[conn_idx][0] = 0;
		currRcvdSeqNo[conn_idx] = 0;

#if (UI_LED_ENABLE)
		if(pConnEvt->role == ACL_ROLE_CENTRAL){  //master
			gpio_write( gpio_table[conn_idx], LED_ON_LEVAL);
		}
#endif

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

	if(central_disconnect_connhandle == pCon->connHandle){  //un_pair disconnection flow finish, clear flag
		central_disconnect_connhandle = 0;
	}

#if (ACL_CENTRAL_SMP_ENABLE)
	if(central_smp_pending == pCon->connHandle){
		central_smp_pending = 0;
	}
#endif



	u8 conn_idx = dev_char_get_conn_index_by_connhandle(pCon->connHandle);
	app_test_start[conn_idx]    = 0;
	device_connection_tick[conn_idx] = 0;
	memset(pushTxStopStart, 0, sizeof(pushTxStopStart));

#if (UI_LED_ENABLE)
	if(dev_char_get_conn_role_by_connhandle(pCon->connHandle) == ACL_ROLE_CENTRAL){  //master
			gpio_write( gpio_table[conn_idx], !LED_ON_LEVAL);
		}
#endif
	///"dev_char_info_delete_by_connhandle" must be placed after "dev_char_get_conn_index_by_connhandle".
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
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt)
{
	u8 conn_idx = dev_char_get_conn_index_by_connhandle(connHandle);
	if( dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL)   //GATT data for Master
	{
		rf_packet_att_t *pAtt = (rf_packet_att_t*)pkt;

		//so any ATT data before service discovery will be dropped
		dev_char_info_t* dev_info = dev_char_info_search_by_connhandle (connHandle);
		if(dev_info)
		{
			//-------	user process ------------------------------------------------
			u16 attHandle = pAtt->handle;

			if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //slave handle notify
			{

				if(attHandle == SPP_SERVER_TO_CLIENT_DP_H)
				{
					if(muliti_ll_md_start){
						u16 len = pAtt->l2capLen - 3;
						if(len > 0)
						{
							u8 seqNoRcv = pAtt->dat[0];
							if(seqNoRcv == currRcvdSeqNo[conn_idx]){
								currRcvdSeqNo[conn_idx]++;
							}
							else{
								AA_dbg_write_cmd_err = 1;
								//printf("s2c:ERR[0x%x]:seq lost[seqNoRcv:%d][currRcvdSeqNo:%d]\n", connHandle, seqNoRcv, currRcvdSeqNo[conn_idx]);

							#if 1
								irq_disable();
								while(1){
									gpio_write(GPIO_LED_RED, 1);  // GPIO_LED_RED
									sleep_ms(100);
									gpio_write(GPIO_LED_RED, 0);  // GPIO_LED_RED
									sleep_ms(100);
								}
							#endif
							}
						}
					}
				}
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


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
void feature_md_test_mainloop (void)
{

	proc_keyboard( 0, 0, 0);






	for(int i=0; i< MASTER_ACL_PERIPHR_MAX_NUM; i++)
	{


		if(device_connection_tick[i] && clock_time_exceed(device_connection_tick[i] , 1000000)){
			device_connection_tick[i]  = 0;

			app_test_start[i] = 1;
		}

		if(app_test_start[i]){  //connection state
			u16 connHandle = 	conn_dev_list[i].conn_handle ;
			if(dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL){  //Master
				if(muliti_ll_md_start){
					if(BLE_SUCCESS == blc_gatt_pushWriteCommand(connHandle,  SPP_CLIENT_TO_SERVER_DP_H, app_test_data[i], 20)){
						//app_test_data[i][0] ++;
						u32* pSno = (u32*)app_test_data[i];
						(*pSno)++;

						DBG_CHN12_TOGGLE;
					}
				}
			}
			else{ //Slave

				if(muliti_ll_md_start){
					if(BLE_SUCCESS == blc_gatt_pushHandleValueNotify(connHandle, SPP_SERVER_TO_CLIENT_DP_H, app_test_data[i], 20)){
						//app_test_data[i][0] ++;
						u32* pSno = (u32*)app_test_data[i];
						(*pSno)++;

						DBG_CHN12_TOGGLE;
					}
				}
			}
		}
	}

	if(muliti_ll_md_stop){ //if already stop LL MD test,do nothing

		//for M4S3, connection handles are:
		//master: 0x0080/0x0081/0x0082/0x0083
		//slave:  0x0044/0x0045/0x0046
		for(int i=0; i< MASTER_ACL_PERIPHR_MAX_NUM; i++)
		{

			u16 connHandle = 	conn_dev_list[i].conn_handle ;

			if(pushTxStopStart[i]){
				app_test_data[i][4] = 0; //stop
				if(dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL){  //Master
					if(BLE_SUCCESS == blc_gatt_pushWriteCommand(connHandle,  SPP_CLIENT_TO_SERVER_DP_H, app_test_data[i], 20)){
						pushTxStopStart[i] = 0;
					}
				}
				else{ //Slave
					if(BLE_SUCCESS == blc_gatt_pushHandleValueNotify(connHandle, SPP_SERVER_TO_CLIENT_DP_H, app_test_data[i], 20)){
						pushTxStopStart[i] = 0;
					}
				}
			}
		}

		if(!memcmp(pushTxStopStart, zero_all, sizeof(pushTxStopStart))){
			muliti_ll_md_stopTick = clock_time()|1;
			muliti_ll_md_stop = 0;
			//printf("Stop LL MD test %s\n", muliti_ll_md_start? "failed":"succeed");
		}
	}


#if (LL_RX_OVERFLOW_TEST_ENABLE)
	if(clock_time_exceed(block_tick, 2000000)){
		block_tick = clock_time();
	}
	else{
		return;
	}

	WaitMs(1000); //UI enter 1s block
#endif

}



int my_client_2_server_write_callback(u16 connHandle, void * p)
{
	rf_packet_att_data_t *req = (rf_packet_att_data_t*)p;
	if(muliti_ll_md_start){
		u8 seqNoRcv = req->dat[0];
		u8 conn_idx = dev_char_get_conn_index_by_connhandle(connHandle);
		 if(currRcvdSeqNo[conn_idx] == seqNoRcv){
			 currRcvdSeqNo[conn_idx]++;
			 DBG_CHN11_TOGGLE;
		}
		else{
			AA_dbg_notify_err = 1;
			//printf("c2s:ERR[0x%x]:seq lost[seqNoRcv:%d][currRcvdSeqNo:%d]\n", conn_idx, seqNoRcv, currRcvdSeqNo[conn_idx]);

		#if 1
			irq_disable();
			while(1){
				gpio_write(GPIO_LED_WHITE, 1);
				sleep_ms(100);
				gpio_write(GPIO_LED_WHITE, 0);
				sleep_ms(100);
			}
		#endif
		}
	}

	return 1;
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

	blc_ll_initStandby_module(mac_public);						   //mandatory

    blc_ll_initLegacyAdvertising_module(); 	//adv module: 		 mandatory for BLE slave,

    blc_ll_initLegacyScanning_module(); 	//scan module: 		 mandatory for BLE master

    blc_ll_initLegacyInitiating_module();	//initiate module: 	 mandatory for BLE master

	blc_ll_initAclConnection_module();
	blc_ll_initAclCentralRole_module();
	blc_ll_initAclPeriphrRole_module();



	blc_ll_setMaxConnectionNumber( ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_MASTER_MAX_TX_OCTETS, ACL_SLAVE_MAX_TX_OCTETS);

	/* all ACL connection share same RX FIFO */
	blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
	/* ACL Master TX FIFO */
	blc_ll_initAclCentralTxFifo(app_acl_mstTxfifo, ACL_MASTER_TX_FIFO_SIZE, ACL_MASTER_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);
	/* ACL Slave TX FIFO */
	blc_ll_initAclPeriphrTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE, ACL_SLAVE_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);



	blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_31P25MS);

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

	/* L2CAP Initialization */
	blc_l2cap_initAclConnMasterMtuBuffer(mtu_m_rx_fifo, MTU_M_BUFF_SIZE_MAX, 			0,					 0);
	blc_l2cap_initAclConnSlaveMtuBuffer(mtu_s_rx_fifo, MTU_S_BUFF_SIZE_MAX, mtu_s_tx_fifo, MTU_S_BUFF_SIZE_MAX);

	blc_att_setMasterRxMTUSize(ATT_MTU_MASTER_RX_MAX_SIZE); ///must be placed after "blc_gap_init"
	blc_att_setSlaveRxMTUSize(ATT_MTU_SLAVE_RX_MAX_SIZE);   ///must be placed after "blc_gap_init"

	/* GATT Initialization */
	my_gatt_init();
	blc_spp_registerWriteCallback(my_client_2_server_write_callback);
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
		user_master_host_pairing_management_init(); 		//TeLink referenced pairing&bonding without standard pairing in BLE Spec
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
	blc_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	blc_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
	blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable

	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

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

	////////////////////////////////////// BLE entry /////////////////////////////////
	blc_sdk_main_loop();

	////////////////////////////////////// UI entry /////////////////////////////////
    feature_md_test_mainloop ();

	proc_master_role_unpair();

	return 0; //must return 0 due to SDP flow
}



_attribute_no_inline_ void main_loop(void)
{
	main_idle_loop();
}

#endif



