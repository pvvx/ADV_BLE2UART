/********************************************************************************************************
 * @file	device_manage.h
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
#ifndef APP_DEVICE_H_
#define APP_DEVICE_H_


#include "vendor/common/user_config.h"
#include "stack/ble/ble_common.h"
#include "stack/ble/hci/hci_event.h"

#ifndef MASTER_MAX_NUM
#define MASTER_MAX_NUM         						4
#endif

#ifndef SLAVE_MAX_NUM
#define SLAVE_MAX_NUM     							4
#endif



#define INVALID_CONN_IDX     						0xFF



#define DEVICE_CHAR_INFO_MAX_NUM		(MASTER_MAX_NUM + SLAVE_MAX_NUM)    //4 master, 3 slave most



#define CHAR_HANDLE_MAX					8
/***************** connection character device information ****************************
 *
 * Demo master device char_handle(ATT handle) define as follows, assuming that peer device(BLE slave) is TELINK HID device
 *   char_handle[0] :  MIC
 *   char_handle[1] :  Speaker
 *   char_handle[2] :  OTA
 *   char_handle[3] :  Consume Report In
 *   char_handle[4] :  Key Report In
 *   char_handle[5] :
 *   char_handle[6] :  BLE Module, SPP Server to Client
 *   char_handle[7] :  BLE Module, SPP Client to Server
 *************************************************************************************/

typedef struct {
	u8 id_adrType;					//identity address type
	u8 id_addr[6];					//identity address
	u8 reserved;
	u8 irk[16];
} rpa_addr_t;



//Attention: manual set 4 byte aligned
typedef struct
{
	u16 	conn_handle;
	u8		conn_role;				// 0: master; 1: slave
	u8 		conn_state;				// 1: connect;  0: disconnect

	u8		char_handle_valid;      // 1: peer device's attHandle is available;   0: peer device's attHandle not available
	u8		rsvd[3];  				// for 4 Byte align

	u8		peer_adrType;
	u8		peer_addr[6];
	u8		peer_RPA;         //RPA: resolvable private address

	//rpa_addr_t *pPeer_RPA;    //only when peer  mac_address is RPA, this pointer is useful

	u16		char_handle[CHAR_HANDLE_MAX];

}dev_char_info_t;


extern dev_char_info_t	conn_dev_list[];


extern int	conn_master_num;
extern int	conn_slave_num;

/**
 * @brief       Used for add device information to conn_dev_list.
 * @param[in]   dev_char_info       - Pointer point to data buffer.
 * @return      0 ~ DEVICE_CHAR_INFO_MAX_NUM - 1: new connection index, insert success
 *              0xFF: insert failed
 */
int 	dev_char_info_insert (dev_char_info_t* dev_char_info);



/**
 * @brief       Used for add device information to conn_dev_list.
 * @param[in]   pConnEvt - LE connection complete event data buffer address.
 * @return      0 ~ DEVICE_CHAR_INFO_MAX_NUM - 1: new connection index, insert success
 *              0xFF: insert failed
 */
int 	dev_char_info_insert_by_conn_event(hci_le_connectionCompleteEvt_t* pConnEvt);


/**
 * @brief       Used for add device information to conn_dev_list.
 * @param[in]   pConnEvt - LE enhanced connection complete event data buffer address.
 * @return      0 ~ DEVICE_CHAR_INFO_MAX_NUM - 1: new connection index, insert success
 *              0xFF: insert failed
 */
int dev_char_info_insert_by_enhanced_conn_event(hci_le_enhancedConnCompleteEvt_t* pConnEvt);


/**
 * @brief       Used for delete device information from conn_dev_list by connHandle
 * @param[in]   connhandle       - connection handle.
 * @return      0: success
 *              1: no find
 */
int 	dev_char_info_delete_by_connhandle (u16 connhandle);

/**
 * @brief       Used for delete device information from conn_dev_list by peer mac_address
 * @param[in]   adr_type       - peer address type.
 * @param[in]   addr           - Pointer point to peer address.
 * @return      0: success
 *              1: no find
 */
int 	dev_char_info_delete_by_peer_mac_address (u8 adr_type, u8* addr);


/**
 * @brief       Get device information by  peer device address.
 * @param[in]   adr_type       - peer address type.
 * @param[in]   addr           - Pointer point to peer address.
 * @return      0: no find
 *             !0: found
 */
dev_char_info_t* 	dev_char_info_search_by_peer_mac_address (u8 adr_type, u8* addr);

/**
 * @brief       Get device information by connection handle.
 * @param[in]   connhandle       - connection handle.
 * @return      0: no find
 *             !0: found
 */
dev_char_info_t* 	dev_char_info_search_by_connhandle (u16 connhandle);



/**
 * @brief       Used for judge if current device conn_handle
 * @param[in]   connhandle       - connection handle.
 * @return
 */
bool	dev_char_info_is_connection_state_by_conn_handle(u16 connhandle);



/**
 * @brief       Get ACL connection role by connection handle.
 * @param[in]   connhandle       - connection handle.
 * @return      0: LL_ROLE_MASTER
 * 				1: LL_ROLE_SLAVE
 * 				2: connection handle invalid
 */
int dev_char_get_conn_role_by_connhandle (u16 connhandle);





/**
 * @brief       Get ACL connection index by connection handle.
 * @param[in]   connhandle       - connection handle.
 * @return      0xFF: 	  no connection index match
 * 				others:   connection index
 */
int dev_char_get_conn_index_by_connhandle (u16 connhandle);





#endif /* APP_DEVICE_H_ */
