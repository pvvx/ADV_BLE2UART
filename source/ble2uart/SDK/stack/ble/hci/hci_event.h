/********************************************************************************************************
 * @file	hci_event.h
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
#ifndef HCI_EVENT_H_
#define HCI_EVENT_H_


#include "stack/ble/ble_common.h"


/**
 *  @brief  Definition for general HCI event packet
 */
typedef struct {
	u8	 	   type;
	u8         eventCode;
	u8         paraLen;
	u8         parameters[1];
} hci_event_t;


typedef struct {
	u8         numHciCmds;
	u8         opCode_OCF;
	u8		   opCode_OGF;
	u8         returnParas[1];
} hci_cmdCompleteEvt_evtParam_t;


typedef struct {
	u8         status;
	u8         numHciCmds;
	u8         opCode_OCF;
	u8		   opCode_OGF;
} hci_cmdStatusEvt_evtParam_t;

typedef struct{
	u16 		connHandle;
	u16 		numOfCmpPkts;
}numCmpPktParamRet_t;

typedef struct {
	u8         numHandles;
	numCmpPktParamRet_t retParams[1];//TODO
} hci_numOfCmpPktEvt_t;

typedef struct{
	u8  status;
	u16 connHandle;
	u8  verNr;
	u16 compId;
	u16 subVerNr;
}hci_readRemVerInfoCmplEvt_t;

typedef struct {
	u8		   type;
	u8         eventCode;
	u8         paraLen;
	u8         subEventCode;
	u8         parameters[1];
} hci_le_metaEvt_t;



/**
 *  @brief  Event Parameters for "7.7.5 Disconnection Complete event"
 */
typedef struct {
	u8	status;
	u16	connHandle;
	u8	reason;
} event_disconnection_t;

typedef struct {
	u8         status;
	u16        connHandle;
	u8         reason;
} hci_disconnectionCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.8 Encryption Change event"
 */
typedef struct {
	u8	status;
	u16	handle;
	u8  enc_enable;
} event_enc_change_t;

typedef struct {
	u8         status;
	u16        connHandle;
	u8         encryption_enable;
} hci_le_encryptEnableEvt_t;

/**
 *  @brief  Event Parameters for "7.7.39 Encryption Key Refresh Complete event"
 */
typedef struct {
	u8	status;
	u16	handle;
} event_enc_refresh_t;

typedef struct {
	u8         status;
	u16        connHandle;
} hci_le_encryptKeyRefreshEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.1 LE Connection Complete event"
 */
typedef struct {
	u8         subEventCode;
	u8         status;
	u16        connHandle;
	u8         role;
	u8         peerAddrType;
	u8         peerAddr[6];
	u16        connInterval;
	u16        slaveLatency;
	u16        supervisionTimeout;
	u8         masterClkAccuracy;
} hci_le_connectionCompleteEvt_t;


/* ACL Connection Role */
typedef enum {
	LL_ROLE_MASTER 	= 0,
	LL_ROLE_SLAVE 	= 1,
} acl_conection_role_t;

/**
 *  @brief  Event Parameters for "7.7.65.2 LE Advertising Report event"
 */
typedef struct {
	u8	subcode;
	u8	nreport;
	u8	event_type;
	u8	adr_type;
	u8	mac[6];
	u8	len;
	u8	data[1];
} event_adv_report_t;

/* Advertise report event type */
typedef enum {
	ADV_REPORT_EVENT_TYPE_ADV_IND 		= 0x00,
	ADV_REPORT_EVENT_TYPE_DIRECT_IND 	= 0x01,
	ADV_REPORT_EVENT_TYPE_SCAN_IND 		= 0x02,
	ADV_REPORT_EVENT_TYPE_NONCONN_IND 	= 0x03,
	ADV_REPORT_EVENT_TYPE_SCAN_RSP 		= 0x04,
} advReportEventType_t;



/**
 *  @brief  Event Parameters for "7.7.65.3 LE Connection Update Complete event"
 */
typedef struct {
	u8         subEventCode;
	u8         status;
	u16        connHandle;
	u16        connInterval;
	u16        connLatency;
	u16        supervisionTimeout;
} hci_le_connectionUpdateCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.4 LE Read Remote Features Complete event"
 */
#define LL_FEATURE_SIZE                                      			8
typedef struct {
	u8         subEventCode;
	u8         status;
	u16        connHandle;
	u8		   feature[LL_FEATURE_SIZE];
} hci_le_readRemoteFeaturesCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.5 LE Long Term Key Request event"
 */
typedef struct {
	u8         subEventCode;
	u16        connHandle;
	u8         random[8];
	u16        ediv;
} hci_le_longTermKeyRequestEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.6 LE Remote Connection Parameter Request event"
 */
typedef struct {
	u8         subEventCode;
	u16        connHandle;
	u16        IntervalMin;
	u16        IntervalMax;
	u16		   latency;
	u16		   timeout;
} hci_le_remoteConnParamReqEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.7 LE Data Length Change event"
 */
typedef struct {
	u8         subEventCode;
	u16        connHandle;  //no aligned, can not be used as pointer
	u16  	   maxTxOct;
	u16		   maxTxtime;
	u16  	   maxRxOct;
	u16		   maxRxtime;
} hci_le_dataLengthChangeEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.8 LE Read Local P-256 Public Key Complete event"
 */
typedef struct {
	u8         subEventCode;
	u8         status;
	u8         localP256Key[64];
} hci_le_readLocalP256KeyCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.9 LE Generate DHKey Complete event"
 */
typedef struct {
	u8         subEventCode;
	u8         status;
	u8         DHKey[32];
} hci_le_generateDHKeyCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.10 LE Enhanced Connection Complete event"
 */
typedef struct {
	u8         subEventCode;
	u8         status;
	u16		   connHandle;
	u8		   role;
	u8		   PeerAddrType;
	u8         PeerAddr[6];
	u8         localRslvPrivAddr[6];
	u8         Peer_RslvPrivAddr[6];
	u16        connInterval;
	u16        connLatency;
	u16        superTimeout;
	u8         masterClkAccuracy;
} hci_le_enhancedConnCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.12 LE PHY Update Complete event"
 */
typedef struct {
	u8         subEventCode;
	u8         status;
	u16        connHandle;
	u8 		   tx_phy;
	u8		   rx_phy;
} hci_le_phyUpdateCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.13 LE Extended Advertising Report event"
 */
typedef struct{
	u16		event_type;			// 2
	u8		address_type;       // 1
	u8		address[6];			// 6
	u8		primary_phy;		// 1
	u8		secondary_phy;		// 1
	u8		advertising_sid;	// 1
	u8		tx_power;			// 1
	u8		rssi;				// 1
	u16		perd_adv_inter;		// 2   Periodic_Advertising_Interval
	u8		direct_address_type;// 1
	u8		direct_address[6];	// 6
	u8		data_length;		// 1
	u8		data[1];
} extAdvEvt_info_t;

typedef struct{
	u8	 sub_code;
	u16  sync_handle;
	u8   tx_power;

	u8   rssi;
	u8   unused;
	u8   data_status;
	u8   data_len;

	u8   data[1];
}le_periodAdvReportEvt_t;

#define EXTADV_INFO_LENGTH				24	 //byte number from "event_type" to "data_length"
#define EXTADV_RPT_DATA_LEN_MAX			229  //253 - 24 = 229

typedef struct{
	u8		subEventCode;
	u8		num_reports;
	u8		advEvtInfo[1];
} hci_le_extAdvReportEvt_t;


/* Extended Advertising Report Event Event_Type mask*/
typedef enum{
	EXTADV_RPT_EVT_MASK_CONNECTABLE					=	BIT(0),
	EXTADV_RPT_EVT_MASK_SCANNABLE     				=	BIT(1),
	EXTADV_RPT_EVT_MASK_DIRECTED  				    =   BIT(2),
	EXTADV_RPT_EVT_MASK_SCAN_RESPONSE     			=	BIT(3),
	EXTADV_RPT_EVT_MASK_LEGACY          			=	BIT(4),

	EXTADV_RPT_DATA_COMPLETE     					=	0x00,
	EXTADV_RPT_DATA_INCOMPLETE_MORE_TO_COME         =	0x20,
	EXTADV_RPT_DATA_INCOMPLETE_TRUNCATED          	=	0x40,
	EXTADV_RPT_DATA_RFU				 	          	=	0x60,

	EXTADV_RPT_EVTTYPE_MASK		         			=	0x1F,
	EXTADV_RPT_DATA_STATUS_MASK		         		=	0x60,
}extAdvRptEvtMask_t;


/* Extended Advertising Report Event_Type */
typedef enum{
	//Legacy
	EXTADV_RPT_EVTTYPE_LEGACY_ADV_IND 				       		= 0x0013,		//  0001 0011'b
	EXTADV_RPT_EVTTYPE_LEGACY_ADV_DIRECT_IND			       	= 0x0015,		//  0001 0101'b
	EXTADV_RPT_EVTTYPE_LEGACY_ADV_SCAN_IND	 					= 0x0012,		//  0001 0010'b
	EXTADV_RPT_EVTTYPE_LEGACY_ADV_NONCONN_IND					= 0x0010,		//  0001 0000'b
	EXTADV_RPT_EVTTYPE_LEGACY_SCAN_RSP_2_ADV_IND				= 0x001B,		//  0001 1011'b
	EXTADV_RPT_EVTTYPE_LEGACY_SCAN_RSP_2_ADV_SCAN_IND			= 0x001A,		//  0001 1010'b

	//Extended
	EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_UNDIRECTED    	    = 0x0000,	// Extended, Non_Connectable Non_Scannable Undirected
	EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_UNDIRECTED       		= 0x0001,	// Extended, Connectable Undirected
	EXTADV_RPT_EVTTYPE_EXT_SCANNABLE_UNDIRECTED					= 0x0002,	// Extended, Scannable Undirected
	EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_DIRECTED			= 0x0004,	// Extended, Non_Connectable Non_Scannable Directed
	EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_DIRECTED			       	= 0x0005,	// Extended, Connectable Directed
	EXTADV_RPT_EVTTYPE_EXT_SCANNABLE_DIRECTED					= 0x0006,	// Extended, Scannable Directed
	EXTADV_RPT_EVTTYPE_EXT_SCAN_RESPONSE						= 0x0008,	// Extended, AUX_SCAN_RESPONSE
}extAdvRptEvtType_t;  //extended advertising report event type




/* Address type */
typedef enum{
	EXTADV_RPT_PUBLIC_DEVICE_ADDRESS				=	0x00,
	EXTADV_RPT_RANDOM_DEVICE_ADDRESS     			=	0x01,
	EXTADV_RPT_PUBLIC_IDENTITY_ADDRESS  			=   0x02,
	EXTADV_RPT_RANDOM_IDENTITY_ADDRESS     			=	0x03,
	EXTADV_RPT_ANONYMOUS_ADV     					=	0xFF,
}ext_adv_adr_type_t;


#define	PERIODIC_ADV_INTER_NO_PERIODIC_ADV							0
#define	SECONDARY_PHY_NO_PACKET_ON_SECONDARY_ADV_CHN				0
#define	ADVERTISING_SID_NO_ADI_FIELD								0xFF
#define TX_POWER_INFO_NOT_AVAILABLE									127



/**
 *  @brief  Event Parameters for "7.7.65.14 LE Periodic Advertising Sync Established event"
 */
typedef struct {
	u8		subEventCode;
	u8		status;
	u16		syncHandle;
//	extadv_id_t	adverting_id;
	u8		advSID;
	u8		advAddrType;
	u8		advAddr[6];
	u8		advPHY;
	u16		perdAdvItvl;
	u8		advClkAccuracy;
}hci_le_periodicAdvSyncEstablishedEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.15 LE Periodic Advertising Report event"
 */
typedef struct {
	u8		subEventCode;
	u16		syncHandle;
	u8		txPower;
	u8		RSSI;
	u8		cteType;
	u8		dataStatus;
	u8		dataLength;  // 0 to 247 Length of the Data field
	u8		data[1];
} hci_le_periodicAdvReportEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.16 LE Periodic Advertising Sync Lost event"
 */
typedef struct {
	u8		subEventCode;
	u16		syncHandle;
} hci_le_periodicAdvSyncLostEvt_t;



typedef struct {
	//TODO
} hci_le_scanTimeoutEvt_t;




/**
 *  @brief  Event Parameters for "7.7.65.18 LE Advertising Set Terminated event"
 */
typedef struct {
	u8         subEventCode;
	u8         status;
	u8         advHandle;
	u16		   connHandle;
	u8		   num_compExtAdvEvt;
} hci_le_advSetTerminatedEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.19 LE Scan Request Received event"
 */
typedef struct {
	//TODO
} hci_le_scanReqRcvdEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.20 LE Channel Selection Algorithm event"
 */
typedef struct {
	u8         subEventCode;
	u16        connHandle;
	u8		   channel_selection_algotihm;
} hci_le_chnSelectAlgorithmEvt_t;






/**
 *  @brief  Event Parameters for "7.7.65.25 LE CIS Established event"
 */
typedef struct {
	u8        	subEventCode;
	u8        	status;
	u16			cisHandle;
	u8          cigSyncDly[3];
	u8          cisSyncDly[3];
	u8          transLaty_m2s[3];
	u8          transLaty_s2m[3];
	u8			phy_m2s;
	u8			phy_s2m;
	u8			nse;
	u8			bn_m2s;
	u8			bn_s2m;
	u8			ft_m2s;
	u8			ft_s2m;
	u16			maxPDU_m2s;
	u16			maxPDU_s2m;
	u16			isoIntvl;
} hci_le_cisEstablishedEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.26 LE CIS Request event"
 */
typedef struct {
	u8        	subEventCode;
	u16        	aclHandle;
	u16        	cisHandle;
	u8			cigId;
	u8			cisId;
} hci_le_cisReqEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.27 LE Create BIG Complete event"
 */
typedef struct {
	u8        	subEventCode;
	u8        	status;
	u8			bigHandle;
	u8        	bigSyncDly[3];
	u8          transLatyBig[3];
	u8			phy;
	u8			nse;
	u8			bn;
	u8			pto;
	u8			irc;
	u16			maxPDU;
	u16			isoIntvl;
	u8			numBis;
	u16		    bisHandles[1];//LL_BIS_IN_PER_BIG_BCST_NUM_MAX];
} hci_le_createBigCompleteEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.28 LE Terminate BIG Complete event"
 */
typedef struct {
	u8        	subEventCode;
	u8			bigHandle;
	u8			reason;
} hci_le_terminateBigCompleteEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.20 LE Channel Selection Algorithm event"
 */
typedef struct {
	u8        	subEventCode;
	u8        	status;
	u8			bigHandle;
	u8          transLatyBig[3];
	u8			nse;
	u8			bn;
	u8			pto;
	u8			irc;
	u16			maxPDU;
	u16			isoIntvl;
	u8			numBis;
	u16         bisHandles[1];//BIS_IN_PER_BIG_SYNC_NUM_MAX];
} hci_le_bigSyncEstablishedEvt_t;


/**
 *  @brief  Event Parameters for "7.7.65.29 LE BIG Sync Established event"
 */
typedef struct {
	u8        	subEventCode;
	u8			bigHandle;
	u8			reason;
} hci_le_bigSyncLostEvt_t;



/**
 *  @brief  Event Parameters for "7.7.65.30 LE BIG Sync Lost event"
 */


/**
 *  @brief  Event Parameters for "7.7.65.34 LE BIGInfo Advertising Report event"
 */
typedef struct {
	u8		subEventCode;
	u16		syncHandle;
	u8		numBis;
	u8		nse;
	u16		IsoItvl; //in units of 1.25 ms.
	u8		bn;
	u8		pto;
	u8		irc;
	u16		maxPdu;
	u8		sduItvl[3];
	u16		maxSdu;
	u8		phy;
	u8		framing;
	u8		enc;
} hci_le_bigInfoAdvReportEvt_t;


int		hci_le_periodicAdvSyncEstablished_evt (u8 status, u16 syncHandle,u8 advSID, u8 advAddrType, u8 advAddress[6], u8 advPHY,
										       u16 perdAdvItvl, u8 advClkAccuracy);
//int		hci_le_periodicAdvSyncEstablished_evt (u8 status, u16 syncHandle, extadv_id_t *pId, u8 advPHY, u16 perdAdvItvl, u8 advClkAccuracy);

int		hci_le_periodicAdvReport_evt (u8 subEventCode, u16 syncHandle, u8 txPower, u8 RSSI, u8 cteType,u8 dataStatus, u8 dataLength,
								      u8* data);
int		hci_le_periodicAdvSyncLost_evt (u16 syncHandle);
int		hci_le_cisEstablished_evt(u8 status, u16 cisHandle, u8 cigSyncDly[3], u8 cisSyncDly[3], u8 transLaty_m2s[3], u8 transLaty_s2m[3], u8 phy_m2s,
		                         u8 phy_s2m, u8 nse, u8 bn_m2s, u8 bn_s2m, u8 ft_m2s, u8 ft_s2m, u16 maxPDU_m2s, u16 maxPDU_s2m, u16 isoIntvl );
int		hci_le_cisReq_evt(u16 aclHandle, u16 cisHandle, u8 cigId, u8 cisId);
int		hci_le_createBigComplete_evt(u8 status, u8 bigHandle, u8 bigSyncDly[3], u8 transLatyBig[3], u8 phy, u8 nse,
								     u8 bn, u8 pto, u8 irc, u16 maxPDU, u16 isoIntvl, u8 numBis, u16* bisHandles);
int		hci_le_terminateBigComplete_evt(u8 bigHandle, u8 reason);
int		hci_le_bigSyncEstablished_evt(u8 staus, u8 bigHandle, u8 transLatyBig[3], u8 nse, u8 bn, u8 pto, u8 irc,
		                              u16 maxPDU, u16 isoIntvl,  u8 numBis, u16* bisHandles);
int		hci_le_bigSyncLost_evt(u8 bigHandle, u8 reason);
int		hci_le_BigInfoAdvReport_evt(u16 syncHandle, u8 numBis, u8 nse, u16 IsoItvl, u8 bn, u8 pto, u8 irc,
		                             u16 maxPdu, u8 sduItvl[3], u16 maxSdu, u8 phy, u8 framing, u8 enc);
int		hci_disconnectionComplete_evt(u8 status, u16 connHandle, u8 reason);
int		hci_cmdComplete_evt(u8 numHciCmds, u8 opCode_ocf, u8 opCode_ogf, u8 paraLen, u8 *para, u8 *result);
void	hci_cmdStatus_evt(u8 numHciCmds, u8 opCode_ocf, u8 opCode_ogf, u8 status, u8 *result);
int		hci_le_connectionComplete_evt(u8 status, u16 connHandle, u8 role, u8 peerAddrType, u8 *peerAddr,
                                      u16 connInterval, u16 slaveLatency, u16 supervisionTimeout, u8 masterClkAccuracy);
int		hci_le_enhancedConnectionComplete_evt(u8 status, u16 connHandle, u8 role, u8 peerAddrType, u8 *peerAddr, u8 *loaclRpa, u8 *peerRpa,
                                              u16 connInterval, u16 connLatency, u16 supervisionTimeout, u8 masterClkAccuracy);
int		hci_le_connectionUpdateComplete_evt(u8 status, u16 connHandle, u16 connInterval,
        									u16 connLatency, u16 supervisionTimeout);
int		hci_le_readRemoteFeaturesComplete_evt(u8 status, u16 connHandle, u8 * feature);
int		hci_le_chennel_selection_algorithm_evt(u16 connhandle, u8 channel_selection_alg);
int		hci_le_phyUpdateComplete_evt(u16 connhandle,u8 status, u8 new_phy);
int		hci_le_data_len_update_evt(u16 connhandle,u16 effTxOctets, u16 effRxOctets, u16 maxtxtime, u16 maxrxtime);
int		hci_le_longTermKeyRequest_evt(u16 connHandle, u8* random, u16 ediv, u8* result);
int		hci_le_readLocalP256KeyComplete_evt(u8* localP256Key, u8 status);
int		hci_le_generateDHKeyComplete_evt(u8* DHkey, u8 status);
int		hci_le_encryptChange_evt(u16 connhandle,  u8 encrypt_en);
int		hci_le_encryptKeyRefresh_evt(u16 connhandle);
int		hci_remoteNateReqComplete_evt (u8* bd_addr);


#endif /* HCI_EVENT_H_ */





