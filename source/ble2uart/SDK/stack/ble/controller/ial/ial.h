/********************************************************************************************************
 * @file	ial.h
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
#ifndef IAL_H_
#define IAL_H_




#include "stack/ble/controller/ll/iso/iso.h"
#include "stack/ble/hci/hci_cmd.h"




/*
 * First fragment of sdu, data field
 */
#define HCI_ISO_PKT_HDR_HANDLE_LEN					(2)
#define HCI_ISO_PKT_HDR_DATA_LEN					(2)
#define	HCI_ISO_LOAD_HDR_TIMESTAMP_LEN				(4)
#define	HCI_ISO_LOAD_HDR_PACKET_SN_LEN				(2)
#define	HCI_ISO_LOAD_SDU_LEN						(2)

#define HCI_ISO_PKT_HDR_LEN							(HCI_ISO_PKT_HDR_HANDLE_LEN + HCI_ISO_PKT_HDR_DATA_LEN)

#define HCI_ISO_LOAD_HDR_LEN_MAX					(HCI_ISO_LOAD_HDR_TIMESTAMP_LEN + HCI_ISO_LOAD_HDR_PACKET_SN_LEN + HCI_ISO_LOAD_SDU_LEN)
#define HCI_ISO_LOAD_HDR_LEN_MIN					(HCI_ISO_LOAD_HDR_PACKET_SN_LEN + HCI_ISO_LOAD_SDU_LEN)


#define ISO_FRAMED_SEGM_HEADER_LEN					(2)
#define	ISO_FRAMED_TIMEOFFSET_LEN					(3)


/*
 * HCI ISO data packet
 */
typedef struct{

//0
	u32 timestamp;
	u16 offset;
	u16 sn_offset;


//8
	u16 connHandle		 :12;
	u16 pb         	 	 :2;
	u16 ts				 :1;
	u16 RFU2			 :1;
//10
	u16 iso_dl_len		 :14;  //iso_data_load_length
	u16 RFU3			 :2;


//12
	u8 data[1];

}iso_data_packet_t;






/******************************* Macro & Enumeration & Structure Definition for Stack End ******************************/






/******************************* Macro & Enumeration variables for User Begin ******************************************/



/******************************* Macro & Enumeration variables for User End ********************************************/














/******************************* User Interface  Begin *****************************************************************/

/**
 * @brief      This function is used to initialize the ISOAL module.
 */
void 		blc_ial_initSdu_module(void);

/**
 * @brief      This function is used to initialize sdu buff.
 * @param[in]  rx_fifo
 * @param[in]  rx_fifo_size
 * @param[in]  rx_fifo_num
 * @param[in]  tx_fifo
 * @param[in]  tx_fifo_size
 * @param[in]  tx_fifo_num
 */
void 		blc_ial_initCisSduBuff(u8 *rx_fifo,u16 rx_fifo_size, u8 rx_fifo_num, u8 *tx_fifo,u16 tx_fifo_size, u8 tx_fifo_num);

/**
 * @brief      This function is used to initialize cis sdu buff.
 * @param[in]  rx_fifo
 * @param[in]  rx_fifo_size
 * @param[in]  rx_fifo_num
 * @param[in]  tx_fifo
 * @param[in]  tx_fifo_size
 * @param[in]  tx_fifo_num
 */
void		blc_ial_initBisSduBuff(u8 *rx_fifo,u16 rx_fifo_size, u8 rx_fifo_num, u8 *tx_fifo,u16 tx_fifo_size, u8 tx_fifo_num);

/**
 * @brief      This function is used to pack HCI ISO data packet to SDU packet.
 * @param[in]  cis_connHandle - point to handle of cis.
 * @param[in]  pIsoData - point to hci ISO Data packet buff.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_hci_packIsoData(u16 cis_connHandle, u8 *pIsoData);

/**
 * @brief      This function is used to setup ISO Data Path.
 * @param[in]  refer to the structure 'hci_le_setupIsoDataPathCmdParams_t'
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_isoal_le_setupISODataPath_cmd(hci_le_setupIsoDataPathCmdParams_t *para);

/**
 * @brief      This function is used to segmentation SDU to one Framed PDUs.
 * @param[in]  cis_connHandle
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 * 						 LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t	blc_ial_splitCisSdu2FramedPdu(u16 cis_connHandle);


/**
 * @brief      This function is used to fragmentation SDU to one or more Unframed PDUs.
 * @param[in]  cis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 * 						 LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t 	blc_ial_cis_splitSdu2UnframedPdu(u16 cisHandle, iso_data_packet_t *sdu);


/**
 * @brief      This function is used to fragmentation SDU to one or more Unframed PDUs.
 * @param[in]  bis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 * 						 LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t 	blc_ial_bis_splitSdu2UnframedPdu(u16 bis_connHandle, iso_data_packet_t *sdu);


/**
 * @brief      This function is used to send data to controller by CIS.
 * @param[in]  cisHandle
 * @param[in]  pData  point to data to send
 * @return      Status - 0x00:  succeeded;
 * 						 other:  failed
 */
ble_sts_t blc_cis_sendData(u16 cisHandle, u8 *pData, u16 len);


/**
 * @brief      This function is used to send data to controller by BIS.
 * @param[in]  cisHandle
 * @param[in]  pData  point to data to send
 * @return      Status - 0x00:  succeeded;
 * 						 other:  failed
 */
ble_sts_t blc_bis_sendData(u16 bisHandle, u8 *pData, u16 len);


/******************************* User Interface  End  ******************************************************************/



#endif


