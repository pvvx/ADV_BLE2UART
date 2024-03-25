/********************************************************************************************************
 * @file	device_manage.c
 *
 * @brief	This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "device_manage.h"




/*
 * Used for store information of connected devices.
 *
 * 0 ~ (MASTER_MAX_NUM - 1) is for master,  MASTER_MAX_NUM ~ (MASTER_MAX_NUM + SLAVE_MAX_NUM - 1) s for slave
 *
 * e.g.    MASTER_MAX_NUM	SLAVE_MAX_NUM			master					  slave
 *                0				  1				     none				conn_dev_list[0]
 *                0				  2					 none				conn_dev_list[0..1]
 *                0				  3					 none				conn_dev_list[0..2]
 *                0				  4					 none				conn_dev_list[0..3]
 *
 *                1				  0				conn_dev_list[0]			   none
 *                1				  1				conn_dev_list[0]		conn_dev_list[1]
 *                1				  2				conn_dev_list[0]		conn_dev_list[1..2]
 *                1				  3				conn_dev_list[0]		conn_dev_list[1..3]
 *                1				  4				conn_dev_list[0]		conn_dev_list[1..4]
 *
 *                2				  0				conn_dev_list[0..1]			   none
 *                2				  1				conn_dev_list[0..1]		conn_dev_list[2]
 *                2				  2				conn_dev_list[0..1]		conn_dev_list[2..3]
 *                2				  3				conn_dev_list[0..1]		conn_dev_list[2..4]
 *                2				  4				conn_dev_list[0..1]		conn_dev_list[2..5]
 *
 *                3				  0				conn_dev_list[0..2]			   none
 *                3				  1				conn_dev_list[0..2]		conn_dev_list[3]
 *                3				  2				conn_dev_list[0..2]		conn_dev_list[3..4]
 *                3				  3				conn_dev_list[0..2]		conn_dev_list[3..5]
 *                3				  4				conn_dev_list[0..2]		conn_dev_list[3..6]
 *
 *                4				  0				conn_dev_list[0..3]			   none
 *                4				  1				conn_dev_list[0..3]		conn_dev_list[4]
 *                4				  2				conn_dev_list[0..3]		conn_dev_list[4..5]
 *                4				  3				conn_dev_list[0..3]		conn_dev_list[4..6]
 *                4				  4				conn_dev_list[0..3]		conn_dev_list[4..7]
 */
_attribute_ble_data_retention_	dev_char_info_t	conn_dev_list[DEVICE_CHAR_INFO_MAX_NUM];



_attribute_ble_data_retention_	int conn_master_num = 0;   //current master number in connection state
_attribute_ble_data_retention_	int conn_slave_num = 0;    //current slave number in connection state





/**
 * @brief       Used for add device information to conn_dev_list.
 * @param[in]   dev_char_info       - Pointer point to data buffer.
 * @return      0 ~ DEVICE_CHAR_INFO_MAX_NUM - 1: new connection index, insert success
 *              0xFF: insert failed
 */
int dev_char_info_insert (dev_char_info_t* dev_char_info)
{
	int index = INVALID_CONN_IDX;
	if( dev_char_info->conn_role == LL_ROLE_MASTER ){  //master
		for(int i = 0; i < MASTER_MAX_NUM; i++){
			if(conn_dev_list[i].conn_state == 0){
				index = i;
				conn_master_num ++;
				break;
			}
		}
	}
	else if( dev_char_info->conn_role == LL_ROLE_SLAVE ){  //slave
		for(int i = MASTER_MAX_NUM; i < MASTER_MAX_NUM + SLAVE_MAX_NUM; i++){
			if(conn_dev_list[i].conn_state == 0){
				index = i;
				conn_slave_num ++;
				break;
			}
		}
	}



	if(index != INVALID_CONN_IDX){
		memcpy(&conn_dev_list[index], dev_char_info, sizeof(dev_char_info_t));
		conn_dev_list[index].conn_state = 1;
	}

	return index;
}



/**
 * @brief       Used for add device information to conn_dev_list.
 * @param[in]   pConnEvt - LE connection complete event data buffer address.
 * @return      0 ~ DEVICE_CHAR_INFO_MAX_NUM - 1: new connection index, insert success
 *              0xFF: insert failed
 */
int dev_char_info_insert_by_conn_event(hci_le_connectionCompleteEvt_t* pConnEvt)
{
	int index = INVALID_CONN_IDX;
	if( pConnEvt->role == LL_ROLE_MASTER ){  //master
		for(int i = 0; i < MASTER_MAX_NUM; i++){
			if(conn_dev_list[i].conn_state == 0){
				index = i;
				conn_master_num ++;
				break;
			}
		}
	}
	else if( pConnEvt->role == LL_ROLE_SLAVE ){  //slave
		for(int i = MASTER_MAX_NUM; i < MASTER_MAX_NUM + SLAVE_MAX_NUM; i++){
			if(conn_dev_list[i].conn_state == 0){
				index = i;
				conn_slave_num ++;
				break;
			}
		}
	}

	if(index != INVALID_CONN_IDX){
		memset(&conn_dev_list[index], 0, sizeof(dev_char_info_t));

		conn_dev_list[index].conn_handle = pConnEvt->connHandle;
		conn_dev_list[index].conn_role = pConnEvt->role;
		conn_dev_list[index].conn_state = 1;
		conn_dev_list[index].peer_adrType = pConnEvt->peerAddrType;
		memcpy(conn_dev_list[index].peer_addr, pConnEvt->peerAddr, 6);
	}

	return index;
}




/**
 * @brief       Used for add device information to conn_dev_list.
 * @param[in]   pConnEvt - LE enhanced connection complete event data buffer address.
 * @return      0 ~ DEVICE_CHAR_INFO_MAX_NUM - 1: new connection index, insert success
 *              0xFF: insert failed
 */
int dev_char_info_insert_by_enhanced_conn_event(hci_le_enhancedConnCompleteEvt_t* pConnEvt)
{
	int index = INVALID_CONN_IDX;
	if( pConnEvt->role == LL_ROLE_MASTER ){  //master
		for(int i = 0; i < MASTER_MAX_NUM; i++){
			if(conn_dev_list[i].conn_state == 0){
				index = i;
				conn_master_num ++;
				break;
			}
		}
	}
	else if( pConnEvt->role == LL_ROLE_SLAVE ){  //slave
		for(int i = MASTER_MAX_NUM; i < MASTER_MAX_NUM + SLAVE_MAX_NUM; i++){
			if(conn_dev_list[i].conn_state == 0){
				index = i;
				conn_slave_num ++;
				break;
			}
		}
	}

	if(index != INVALID_CONN_IDX){
		memset(&conn_dev_list[index], 0, sizeof(dev_char_info_t));

		conn_dev_list[index].conn_handle = pConnEvt->connHandle;
		conn_dev_list[index].conn_role = pConnEvt->role;
		conn_dev_list[index].conn_state = 1;
		conn_dev_list[index].peer_adrType = pConnEvt->PeerAddrType;
		memcpy(conn_dev_list[index].peer_addr, pConnEvt->PeerAddr, 6);
	}

	return index;
}




/**
 * @brief       Used for delete device information from conn_dev_list by connHandle
 * @param[in]   connhandle       - connection handle.
 * @return      0: success
 *              1: no find
 */
int dev_char_info_delete_by_connhandle (u16 connhandle)
{
	foreach (i, DEVICE_CHAR_INFO_MAX_NUM)
	{
		if(conn_dev_list[i].conn_handle == connhandle && conn_dev_list[i].conn_state)  //match
		{
			if(conn_dev_list[i].conn_role == LL_ROLE_MASTER){   //master
				conn_master_num --;
			}
			else{  //slave
				conn_slave_num --;
			}

			memset(&conn_dev_list[i], 0, sizeof(dev_char_info_t));

			return 0;
		}
	}

	return 1;  //not find.
}


/**
 * @brief       Used for delete device information from conn_dev_list by peer mac_address
 * @param[in]   adr_type       - peer address type.
 * @param[in]   addr           - Pointer point to peer address.
 * @return      0: success
 *              1: no find
 */
int 	dev_char_info_delete_by_peer_mac_address (u8 adr_type, u8* addr)
{
	foreach (i, DEVICE_CHAR_INFO_MAX_NUM)
	{
		if(conn_dev_list[i].conn_state)
		{
			int mac_match = 0;
			if( IS_RESOLVABLE_PRIVATE_ADDR(adr_type, addr) ){
				//TODO
			}
			else{
				if(adr_type == conn_dev_list[i].peer_adrType && (!memcmp (addr, conn_dev_list[i].peer_addr, 6)) ){
					mac_match = 1;
				}
			}

			//u16 connhandle = conn_dev_list[i].conn_handle;

			if(mac_match){
				if(conn_dev_list[i].conn_role == LL_ROLE_MASTER){   //master
					conn_master_num --;
				}
				else{  //slave
					conn_slave_num --;
				}

				memset(&conn_dev_list[i], 0, sizeof(dev_char_info_t));

				return 0;
			}

		}
	}

	return 1;  //no find the peer device address.
}







/**
 * @brief       Get device information by connection handle.
 * @param[in]   connhandle       - connection handle.
 * @return      0: no find
 *             !0: found
 */
dev_char_info_t* dev_char_info_search_by_connhandle (u16 connhandle)
{
	foreach (i, DEVICE_CHAR_INFO_MAX_NUM)
	{
		if(conn_dev_list[i].conn_handle == connhandle && conn_dev_list[i].conn_state)
		{
			return &conn_dev_list[i];   // find the peer device
		}
	}

	return 0;  // no find the peer device
}




/**
 * @brief       Get device information by  peer device address.
 * @param[in]   adr_type       - peer address type.
 * @param[in]   addr           - Pointer point to peer address.
 * @return      0: no find
 *             !0: found
 */
dev_char_info_t* dev_char_info_search_by_peer_mac_address (u8 adr_type, u8* addr)
{
	foreach (i, DEVICE_CHAR_INFO_MAX_NUM)
	{

		int mac_match = 0;
		if( IS_RESOLVABLE_PRIVATE_ADDR(adr_type, addr) ){
			//TODO
		}
		else{
			if(adr_type == conn_dev_list[i].peer_adrType && (!memcmp (addr, conn_dev_list[i].peer_addr, 6)) ){
				mac_match = 1;
			}
		}

		if(mac_match){
			return &conn_dev_list[i];   // find the peer device
		}
	}

	return 0;  // no find the peer device
}





/**
 * @brief       Get device information by connection handle.
 * @param[in]   connhandle       - connection handle.
 * @return      0: no find
 *             !0: found
 */
bool	dev_char_info_is_connection_state_by_conn_handle(u16 connhandle)
{
	foreach (i, DEVICE_CHAR_INFO_MAX_NUM)
	{
		if(conn_dev_list[i].conn_handle == connhandle && conn_dev_list[i].conn_state)
		{
			return TRUE;
		}
	}

	return FALSE;
}



/**
 * @brief       Get ACL connection role by connection handle.
 * @param[in]   connhandle       - connection handle.
 * @return      0: LL_ROLE_MASTER
 * 				1: LL_ROLE_SLAVE
 * 				2: connection handle invalid
 */
int dev_char_get_conn_role_by_connhandle (u16 connhandle)
{
	foreach (i, DEVICE_CHAR_INFO_MAX_NUM)
	{
		if(conn_dev_list[i].conn_handle == connhandle && conn_dev_list[i].conn_state)
		{
			return conn_dev_list[i].conn_role;
		}
	}

	return 2; //no connection match
}




/**
 * @brief       Get ACL connection index by connection handle.
 * @param[in]   connhandle       - connection handle.
 * @return      0xFF: 	  no connection index match
 * 				others:   connection index
 */
int dev_char_get_conn_index_by_connhandle (u16 connhandle)
{
	foreach (i, DEVICE_CHAR_INFO_MAX_NUM)
	{
		if(conn_dev_list[i].conn_handle == connhandle && conn_dev_list[i].conn_state)
		{
			return i;
		}
	}

	return INVALID_CONN_IDX; //no connection index match
}


