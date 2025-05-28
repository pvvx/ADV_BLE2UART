/********************************** (C) COPYRIGHT *******************************
 * File Name          : observer.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : 观察应用程序，初始化扫描参数，然后定时扫描，如果扫描结果不为空，则打印扫描到的广播地址
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "observer.h"
#include "scanning.h"
#include "app_usb.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
scan_params_t scan_params = {
        .mode = DEVDISC_MODE_ALL,
        .phys = 0, // GAP_PHY_BIT_LE_1M | GAP_PHY_BIT_LE_CODED,
        .scan_type = DEFAULT_DISCOVERY_ACTIVE_SCAN,
        .own_addr_type = 0,
        .duration = DEFAULT_SCAN_DURATION
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8_t gStatus;

// Task ID for internal task/event processing
uint8_t ObserverTaskId;

// Number of scan results and scan result index
//static uint8_t ObserverScanRes;

// Scan result list
//static gapDevRec_t ObserverDevList[DEFAULT_MAX_SCAN_RES];


// Peer device address
//static uint8_t PeerAddrDef[B_ADDR_LEN] = {0x02, 0x02, 0x03, 0xE4, 0xC2, 0x84};

app_drv_fifo_t app_tx_fifo;
app_drv_fifo_t app_rx_fifo;

adv_msg_t adv_msg;

uint8_t app_tx_buffer[APP_TX_BUFFER_LENGTH];
uint8_t app_rx_buffer[APP_RX_BUFFER_LENGTH];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void ObserverEventCB(gapRoleEvent_t *pEvent);
static void Observer_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void ObserverAddDeviceInfo(uint8_t *pAddr, uint8_t addrType);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static const gapRoleObserverCB_t ObserverRoleCB = {
    ObserverEventCB // Event callback
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Observer_Init
 *
 * @brief   Initialization function for the Simple BLE Observer App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notification).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Observer_Init()
{
    ObserverTaskId = TMOS_ProcessEventRegister(Observer_ProcessEvent);

    app_drv_fifo_init(&app_tx_fifo, app_tx_buffer, APP_TX_BUFFER_LENGTH);
    app_drv_fifo_init(&app_rx_fifo, app_rx_buffer, APP_RX_BUFFER_LENGTH);

    // Setup Observer Profile
    uint8_t scanRes = DEFAULT_MAX_SCAN_RES;
    GAPRole_SetParameter(GAPROLE_MAX_SCAN_RES, sizeof(uint8_t), &scanRes);

    // Setup a delayed profile startup
    tmos_set_event(ObserverTaskId, START_DEVICE_EVT);
}

void send_resp(uint8_t cmd, uint8_t id, uint8_t *pmac, uint8_t len) {
    uint8_t s[HEAD_CRC_ADD_LEN];
    uint16_t llen = len;
    memset(s, 0, HEAD_CRC_ADD_LEN);
    //s[0] = 0;
    s[1] = cmd; // rssi
    s[2] = id; // ev type
    s[3] = len; // addr type
    s[4] = 0xff; // phy = 0xff -> cmd response
    if(llen)
        memcpy(&s[5], pmac, (llen > 6)? 6 : llen);
    llen = HEAD_CRC_ADD_LEN; // - 2;
/*
    uint16_t crc = crcFast(s, llen);
    s[llen++] = crc;
    s[llen++] = crc >> 8;
*/
//    app_drv_fifo_flush(&app_tx_fifo);
    app_drv_fifo_write(&app_tx_fifo, s, &llen);
    tmos_set_event(ObserverTaskId, NEW_BLEDATA_EVT);
}


uint8_t read_buf[32];

void parse_command(void) {
    uint16_t len = sizeof(read_buf);
    if(app_drv_fifo_read(&app_rx_fifo, read_buf, &len) != APP_DRV_FIFO_RESULT_SUCCESS)
        return;
    app_drv_fifo_flush(&app_rx_fifo);
    int cmd = read_buf[0];
    if(cmd == CMD_ID_SCAN && len >= 4) {
        uint8_t flg = read_buf[1];
        scan_params.phys = flg & 7;
        scan_params.scan_type = (flg >> 3) & 1; // =1 Passive scan, =0 Active scan
        scan_params.mode = (flg >> 4) & 3;
        mac_list.filtr = flg >> 6;

        scan_params.duration = read_buf[2] | (read_buf[3] << 8);

        if(scan_params.phys) {
            // Setup GAP
            GAP_SetParamValue(TGAP_DISC_SCAN, scan_params.duration);
            GAP_SetParamValue(TGAP_DISC_SCAN_PHY, scan_params.phys);
            GAP_SetParamValue(TGAP_FILTER_ADV_REPORTS, 0);

            GAPRole_ObserverStartDiscovery(scan_params.mode,
                                        scan_params.scan_type,
                                       DEFAULT_DISCOVERY_WHITE_LIST);
            PRINT("Discovery...\n");
        } else {
            GAPRole_ObserverCancelDiscovery();
            app_drv_fifo_flush(&app_tx_fifo);
            PRINT("Stop discovery...\n");
        }
        send_resp(cmd, mac_list.count, &read_buf[1], 3);
    } else if(len >= 6 + 1) {
        if(read_buf[0] == CMD_ID_WMAC) {
            mac_list.mode = WHITE_LIST;
            if(mac_list.count < MAC_MAX_SCAN_LIST) {
#if 1
                memcpy(&mac_list.mac[mac_list.count], &read_buf[1], 6);
#else
                mac_list.mac[mac_list.count][0] = read_buf[6];
                mac_list.mac[mac_list.count][1] = read_buf[5];
                mac_list.mac[mac_list.count][2] = read_buf[4];
                mac_list.mac[mac_list.count][3] = read_buf[3];
                mac_list.mac[mac_list.count][4] = read_buf[2];
                mac_list.mac[mac_list.count][5] = read_buf[1];
#endif
                mac_list.count++;
            }
            send_resp(cmd, mac_list.count, &read_buf[1], 6);
        } else if(read_buf[0] == CMD_ID_BMAC) {
            mac_list.mode = BALCK_LIST;
            if(mac_list.count < MAC_MAX_SCAN_LIST) {
#if 1
                memcpy(&mac_list.mac[mac_list.count], &read_buf[1], 6);
#else
                mac_list.mac[mac_list.count][0] = read_buf[6];
                mac_list.mac[mac_list.count][1] = read_buf[5];
                mac_list.mac[mac_list.count][2] = read_buf[4];
                mac_list.mac[mac_list.count][3] = read_buf[3];
                mac_list.mac[mac_list.count][4] = read_buf[2];
                mac_list.mac[mac_list.count][5] = read_buf[1];
#endif
                mac_list.count++;
            }
            send_resp(cmd, mac_list.count, &read_buf[1], 6);
        }
    } else if(len >= 1) {
        if(read_buf[0] == CMD_ID_CLRM)  {
            mac_list.count = 0;
            send_resp(cmd, MAC_MAX_SCAN_LIST, &read_buf[1], 0);
        }
        else if(read_buf[0] == CMD_ID_INFO) {
            GetMACAddress(read_buf);
            send_resp(cmd, SW_VERSION, read_buf, 6);
        }
    }
}


/*********************************************************************
 * @fn      Observer_ProcessEvent
 *
 * @brief   Simple BLE Observer Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t Observer_ProcessEvent(uint8_t task_id, uint16_t events)
{
    //  VOID task_id; // TMOS required parameter that isn't used in this function

    if(events & NEW_BLEDATA_EVT) {
        if(USBSendData() != SUCCESS) {
            tmos_start_task(ObserverTaskId, NEW_BLEDATA_EVT, 2);
        } else if(app_drv_fifo_length(&app_tx_fifo)) {
            tmos_start_task(ObserverTaskId, NEW_BLEDATA_EVT, 1);
        }
        // return unprocessed events
        return (events ^ NEW_BLEDATA_EVT);
    }

    if(events & NEW_USBDATA_EVT) {
        uint16_t len = app_drv_fifo_length(&app_rx_fifo);
        if(len)
            parse_command();
        // return unprocessed events
        return (events ^ NEW_USBDATA_EVT);
    }

    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(ObserverTaskId)) != NULL)
        {
            Observer_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);

            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_ObserverStartDevice((gapRoleObserverCB_t *)&ObserverRoleCB);

        return (events ^ START_DEVICE_EVT);
    }
/*
    if(events & START_SYNC_TIMEOUT_EVT)
    {
        GAPRole_CancelSync();
        PRINT("Creat SYNC timeout, restart discovery..\n");
        GAPRole_ObserverStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                       DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                       DEFAULT_DISCOVERY_WHITE_LIST);
        return (events ^ START_SYNC_TIMEOUT_EVT);
    }
*/
    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      Observer_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Observer_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        case GATT_MSG_EVENT:
            break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      debug_print_mac
 *
 * @brief   debug print mac
 *
 * @param   pointer mac[6]
 *
 * @return  none
 */
#ifdef  DEBUG
void debug_print_mac(uint8_t * mac) {
    for(int i = B_ADDR_LEN-1; i >= 0; i--) {
        PRINT("%0x", mac[i]);
    }
    PRINT("\r\n");
}
#else
#define debug_print_mac()
#endif


mac_list_t mac_list;

static int chk_mac(uint8_t *pmac) {
    int ret = 0;
    if(mac_list.count) {
        for(int i = 0; i < mac_list.count; i++) {
            if(memcmp(&mac_list.mac[i], pmac, 6) == 0) {
                ret = 1;
                break;
            }
        }
        if(mac_list.mode == BALCK_LIST)
            ret = !ret;
    } else
        ret = 1;
    return ret;
}
/*********************************************************************
 * @fn      ObserverEventCB
 *
 * @brief   Observer event callback function.
 *
 * @param   pEvent - pointer to event structure
 *
 * @return  none
 */
static void ObserverEventCB(gapRoleEvent_t *pEvent)
{
    uint16_t len;
    switch(pEvent->gap.opcode)
    {
        case GAP_DEVICE_INIT_DONE_EVENT:
        {
            // Setup GAP
            GAP_SetParamValue(TGAP_DISC_SCAN, scan_params.duration);
            GAP_SetParamValue(TGAP_DISC_SCAN_PHY, scan_params.phys);
            GAP_SetParamValue(TGAP_FILTER_ADV_REPORTS, 0);

//            GAPRole_ObserverStartDiscovery(scan_params.mode,
//                                            scan_params.scan_type,
//                                           DEFAULT_DISCOVERY_WHITE_LIST);
            PRINT("Init done...\n");
        }
        break;

        case GAP_DEVICE_INFO_EVENT:
        {
//            PRINT("Recv legacy adv ");
//            debug_print_mac(pEvent->deviceDirectInfo.addr);
            if(chk_mac(pEvent->deviceInfo.addr)
              && (pEvent->deviceInfo.addrType & mac_list.filtr) == 0) {
                len = (uint8_t)pEvent->deviceInfo.dataLen;
                adv_msg.len = (uint8_t)len;
                adv_msg.evTypes = pEvent->deviceInfo.eventType;
                adv_msg.adTypes = pEvent->deviceInfo.addrType;
                adv_msg.phyTypes = GAP_PHY_BIT_LE_1M | (GAP_PHY_BIT_LE_1M << 4);
                adv_msg.rssi = pEvent->deviceInfo.rssi;
                memcpy(adv_msg.addr, pEvent->deviceInfo.addr, B_ADDR_LEN);
                if(len)
                    memcpy(adv_msg.data, pEvent->deviceInfo.pEvtData, len);
                len += HEAD_CRC_ADD_LEN;
                app_drv_fifo_write(&app_tx_fifo, (uint8_t *)&adv_msg, &len);
                tmos_set_event(ObserverTaskId, NEW_BLEDATA_EVT);
            }
        }
        break;

        case GAP_DEVICE_DISCOVERY_EVENT:
        {
            if(scan_params.phys) {

                GAPRole_ObserverStartDiscovery(scan_params.mode,
                                            scan_params.scan_type,
                                           DEFAULT_DISCOVERY_WHITE_LIST);
                PRINT("Discovery over...\n");
            }
//            ObserverScanRes = 0;
        }
        break;

        case GAP_EXT_ADV_DEVICE_INFO_EVENT:
        {
//            PRINT("Recv ext. adv ");
//            debug_print_mac(pEvent->deviceDirectInfo.addr);
//            if(socket_connected == 0)
//                  break;
            if(chk_mac(pEvent->deviceExtAdvInfo.addr)
              && (pEvent->deviceInfo.addrType & mac_list.filtr) == 0) {
                len = (uint8_t)pEvent->deviceExtAdvInfo.dataLen;
                adv_msg.len = (uint8_t)len;
                adv_msg.evTypes = pEvent->gap.opcode;
                adv_msg.adTypes = pEvent->deviceExtAdvInfo.addrType;
                adv_msg.phyTypes = pEvent->deviceExtAdvInfo.primaryPHY | (pEvent->deviceExtAdvInfo.secondaryPHY << 4);
                adv_msg.rssi = pEvent->deviceExtAdvInfo.rssi;
                memcpy(adv_msg.addr, pEvent->deviceExtAdvInfo.addr, B_ADDR_LEN);
                if(len)
                    memcpy(adv_msg.data, pEvent->deviceExtAdvInfo.pEvtData, len);
                len += HEAD_CRC_ADD_LEN;
                app_drv_fifo_write(&app_tx_fifo, (uint8_t *)&adv_msg, &len);
                tmos_set_event(ObserverTaskId, NEW_BLEDATA_EVT);

            }
        }
        break;

        case GAP_DIRECT_DEVICE_INFO_EVENT:
        {
//            PRINT("Recv dir. adv ");
//            debug_print_mac(pEvent->deviceDirectInfo.addr);
//            if(socket_connected == 0)
//                break;
            if(chk_mac(pEvent->deviceDirectInfo.addr)
              && (pEvent->deviceInfo.addrType & mac_list.filtr) == 0) {
                adv_msg.len = 0;
                adv_msg.evTypes = pEvent->deviceInfo.eventType;
                adv_msg.adTypes = pEvent->deviceExtAdvInfo.addrType;
                adv_msg.phyTypes = GAP_PHY_BIT_LE_1M | (GAP_PHY_BIT_LE_1M << 4);
                adv_msg.rssi = pEvent->deviceDirectInfo.rssi;
                memcpy(adv_msg.addr, pEvent->deviceDirectInfo.addr, B_ADDR_LEN);
                len = HEAD_CRC_ADD_LEN;
                app_drv_fifo_write(&app_tx_fifo, (uint8_t *)&adv_msg, &len);
                tmos_set_event(ObserverTaskId, NEW_BLEDATA_EVT);
            }
        }
        break;

/*
        case GAP_SYNC_ESTABLISHED_EVENT:
        {
            if(pEvent->syncEstEvt.status == SUCCESS)
            {
                GAPRole_ObserverCancelDiscovery();
                tmos_stop_task(ObserverTaskId, START_SYNC_TIMEOUT_EVT);
                PRINT("GAP_SYNC_ESTABLISHED...\n");
                PRINT("sync handle: %#x\n", pEvent->syncEstEvt.syncHandle);
                PRINT("sync interval: %d\n", pEvent->syncEstEvt.periodicInterval);
            }
        }
        break;
        case GAP_PERIODIC_ADV_DEVICE_INFO_EVENT:
        {
            PRINT("periodic adv - len %d (", pEvent->devicePeriodicInfo.dataLength);
            for (int i = 0 ; i < pEvent->devicePeriodicInfo.dataLength; i++) {
                PRINT(" %#x", pEvent->devicePeriodicInfo.pEvtData[i]);
            }
            PRINT(" )\n");
        }
        break;

        case GAP_SYNC_LOST_EVENT:
        {
            GAPRole_ObserverStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                           DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                           DEFAULT_DISCOVERY_WHITE_LIST);
            PRINT("GAP_SYNC_LOST ...\n");
        }
        break;
*/
        default:
            PRINT("opcode %x ...\n", pEvent->gap.opcode);
            break;
    }
}

/*********************************************************************
*********************************************************************/
