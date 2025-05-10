/********************************************************************************************************
 * @file    hci.h
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
#pragma  once


#include "stack/ble/ble_common.h"

typedef int (*blc_hci_rx_handler_t) (void);
typedef int (*blc_hci_tx_handler_t) (void);
typedef int (*blc_hci_user_handler_t)(u8 *p, u32 len);



#define			HCI_FLAG_EVENT_PHYTEST_2_WIRE_UART			(1<<23)
#define			HCI_FLAG_EVENT_TLK_MODULE					(1<<24)
#define			HCI_FLAG_EVENT_BT_STD						(1<<25)	//HCI event
#define			HCI_FLAG_EVENT_STACK						(1<<26)	//not used now
#define			HCI_FLAG_ACL_BT_STD							(1<<27)
#define			HCI_FLAG_BT_HCI_CMD							(1<<28)	//HCI command
#define			HCI_FLAG_ISO_DATE_STD						(1<<29)

#define			TLK_MODULE_EVENT_STATE_CHANGE				0x0730
#define			TLK_MODULE_EVENT_DATA_RECEIVED				0x0731
#define			TLK_MODULE_EVENT_DATA_SEND					0x0732
#define			TLK_MODULE_EVENT_BUFF_AVAILABLE				0x0733




#define			HCI_MAX_ACL_DATA_LEN              			27

#define 		HCI_MAX_DATA_BUFFERS_SALVE              	8
#define 		HCI_MAX_DATA_BUFFERS_MASTER              	8

#define			HCI_ADV_REPORT_EVT_RSVD_FIFO				3

extern	blc_hci_rx_handler_t	blc_hci_rx_handler;
extern	blc_hci_tx_handler_t	blc_hci_tx_handler;

#if BQB_TEST_EN	//LL/SEC/PER/BV-10-C	//Add by wenjing
extern u32		hci_eventMask_2;
#endif


extern  my_fifo_t	hci_tx_iso_fifo;

typedef	struct _attribute_aligned_(4) {
	u32		size;
	u8		num;
	u8		mask;
	u8		wptr;
	u8		rptr;
	u8*		p;
}hci_fifo_t;


/**
 *  @brief  Definition for HCI packet type & HCI packet indicator
 */
typedef enum{
	HCI_TYPE_CMD 		= 0x01,
	HCI_TYPE_ACL_DATA	= 0x02,
	HCI_TYPE_SCO_DATA	= 0x03,
	HCI_TYPE_EVENT  	= 0x04,
	HCI_TYPE_ISO_DATA 	= 0x05,  //core_5.2
} hci_type_t;


/**
 *  @brief  Definition for HCI ACL Data packets Packet_Boundary_Flag
 */
typedef enum{
	HCI_FIRST_NAF_PACKET          =		0x00,	//LE Host to Controller
	HCI_CONTINUING_PACKET         =		0x01,	//LE Host to Controller / Controller to Host
	HCI_FIRST_AF_PACKET           =    	0x02,	//LE 					  Controller to Host
} acl_pb_flag_t;




/**
 *  @brief  Definition for HCI ISO Data packets PB_Flag
 */
typedef enum{
	HCI_ISO_SDU_FIRST_FRAG		=	0x00,	//The ISO_Data_Load field contains the first fragment of a fragmented SDU
	HCI_ISO_SDU_CONTINUE_FRAG	=	0x01,	//The ISO_Data_Load field contains a continuation fragment of an SDU
	HCI_ISO_SDU_COMPLETE		=	0x02,	//The ISO_Data_Load field contains a complete SDU
	HCI_ISO_SDU_LAST_FRAG		=	0x03,	//The ISO_Data_Load field contains the last fragment of an SDU.
} iso_pb_flag_t;



/**
 *  @brief  Definition for HCI ISO Data packets Packet_Status_Flag
 */
typedef enum{
	HCI_ISO_VALID_DATA				=	0x00, //Valid data. The complete ISO_SDU was received correctly
	HCI_ISO_POSSIBLE_INVALID_DATA	=	0x01, //Possibly invalid data
	HCI_ISO_LOST_DATA				=	0x02, //Part(s) of the ISO_SDU were not received correctly. This is reported as "lost data"
} iso_ps_flag_t;

#if BQB_TEST_EN //HCI//CIN//BV-14-C Add by Jingying

typedef struct __attribute__((packed))
{
	u8 		status;
	u16 	connHandle;
	s8 		rssi;
}hci_readRssi_retParam_t;
/**
 * @brief Command & Return Parameters for "7.5.4 Read RSSI command"
 */
typedef struct
{
	u16 	connHandle;
}hci_readRssi_cmdParam_t;

#endif


// Controller event handler
typedef int (*hci_event_handler_t) (u32 h, u8 *para, int n);

// Controller data handler
typedef int (*hci_data_handler_t) (u16 conn, u8 * p);




// hci event
extern u32		hci_eventMask;
extern u32		hci_le_eventMask;
extern u32		hci_le_eventMask_2;
extern hci_event_handler_t		blc_hci_event_handler;
extern hci_data_handler_t		blc_hci_data_handler;
extern hci_fifo_t				bltHci_rxfifo;
extern hci_fifo_t			    bltHci_txfifo;

void hci_set_revision(u16 revision);
u16  hci_get_revision(void);
u8 hci_dtm_test(void);

/**
 * @brief      for user to initialize HCI TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_initHciTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number);

/**
 * @brief      for user to initialize HCI RX FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_initHciRxFifo(u8 *pRxbuf, int fifo_size, int fifo_number);

/**
 * @brief      for user to initialize HCI RX ACL Data FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address (Attention: buffer size = fifo_size*fifo_num*conn_max_num).
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_initHciAclDataFifo(u8 *pAclbuf, int fifo_size, int fifo_number);


/**
 * @brief      this function is used to get data by USB in RX mode for HCI Layer
 * @param[in]  none.
 * @return     0
 */
int			blc_hci_rx_from_usb (void);


/**
 * @brief      this function is used to send data by USB in TX mode for HCI Layer
 * @param[in]  none.
 * @return     0
 */
int			blc_hci_tx_to_usb (void);


/**
 * @brief      this function is used to process HCI data
 * @param[in]  *p - the pointer of HCI data
 * @param[in]  n - the length of HCI data
 * @return     0
 */
int			blc_hci_handler (u8 *p, int n);


/**
 * @brief      this function is used to report HCI events
 * @param[in]  h - HCI Event type
 * @param[in]  *para - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
int			blc_hci_send_event (u32 h, u8 *para, int n);


/**
 * @brief      this function is used to process HCI events
 * @param[in]  none.
 * @return     0
 */
int			blc_hci_proc (void);


/**
 * @brief      this function is used to set HCI EVENT mask
 * @param[in]  evtMask  -  HCI EVENT mask
 * @return     0
 */
ble_sts_t	blc_hci_setEventMask_cmd(u32 evtMask);      //eventMask: BT/EDR


/**
 * @brief      this function is used to set HCI LE EVENT mask
 * @param[in]  evtMask  -  HCI LE EVENT mask(BIT<0-31>)
 * @return     0
 */
ble_sts_t	blc_hci_le_setEventMask_cmd(u32 evtMask);   //eventMask: LE event  0~31


/**
 * @brief      this function is used to set HCI LE EVENT mask
 * @param[in]  evtMask  -  HCI LE EVENT mask(BIT<32-63>)
 * @return     0
 */
ble_sts_t 	blc_hci_le_setEventMask_2_cmd(u32 evtMask_2);   //eventMask: LE event 32~63


/**
 * @brief      This function is used to register the controller event processing callback
 * @param[in]  handler - hci_event_handler_t
 * @return     none.
 */
void 		blc_hci_registerControllerEventHandler (hci_event_handler_t  handler);


/**
 * @brief      This function is used to register ACL data transmission to Host for processing callback function.
 * @param[in]  handler - hci_data_handler_t
 * @return     none.
 */
void 		blc_hci_registerControllerDataHandler (hci_data_handler_t handle);


/**
 * @brief      this function is used to register HCI TX or RX handler callback function
 * @param[in]  *prx - blc_hci_rx_handler
 * @param[in]  *ptx - blc_hci_tx_handler
 * @return     none.
 */
void 		blc_register_hci_handler (void *prx, void *ptx);

/**
 * @brief      this function is used to register HCI user callback function
 * @param[in]  *usrHandler - blc_hci_rx_handler
 * @return     none.
 */
void 		blc_hci_register_user_handler(void *usrHandler);

/**
 * @brief      this function is used to send ACL data to HOST
 * @param[in]  handle - connect handle
 * @param[in]  *p - the pointer of l2cap data
 * @return     0
 */
int 		blc_hci_sendACLData2Host (u16 handle, u8 *p);

/**
 *
 */
int			blc_hci_getFreeTxFIFONum(void);
/**
 * @brief      this function is used to send data
 * @param[in]  h - HCI Event type
 * @param[in]  *para - data pointer of event
 * @param[in]  n - data length of event
 * @return     0,-1
 */
int 		blc_hci_send_data (u32 h, u8 *para, int n);


/**
 * @brief      This function is used to send ISO data from Controller to HOST.
 * @param[in]  h - HCI Event type
 * @param[in]  *iso_load - data pointer of the ISO load
 * @param[in]  data_load_len - data length of load
 * @return     0,-1
 */
int 		blc_hci_iso_send_data (u32 h, u8 *iso_load, int data_load_len);




