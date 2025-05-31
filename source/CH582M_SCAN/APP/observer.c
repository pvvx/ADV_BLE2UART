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
// Task ID for internal task/event processing
uint8_t ObserverTaskId;

scan_params_t scan_params = {
        .mode = DEVDISC_MODE_ALL,
        .phys = 0, // GAP_PHY_BIT_LE_1M | GAP_PHY_BIT_LE_CODED,
        .scan_type = DEFAULT_DISCOVERY_ACTIVE_SCAN,
        .own_addr_type = 0,
        .duration = DEFAULT_SCAN_DURATION
};

app_drv_fifo_t app_tx_fifo;

uint8_t app_cmd_buf[APP_RX_BUFFER_LENGTH];
uint8_t app_cmd_len;

mac_list_t mac_list;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8_t app_tx_buffer[APP_TX_BUFFER_LENGTH];

adv_msg_t adv_msg; // buffer adv.message

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void ObserverEventCB(gapRoleEvent_t *pEvent);
static void Observer_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);

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

    // Setup Observer Profile -> Default is 0 = unlimited.
    //uint8_t scanRes = DEFAULT_MAX_SCAN_RES;
    //GAPRole_SetParameter(GAPROLE_MAX_SCAN_RES, sizeof(uint8_t), &scanRes);

    // Setup a delayed profile startup
    tmos_set_event(ObserverTaskId, START_DEVICE_EVT);
}

/*********************************************************************
 * @fn      send_cmd_resp
 *
 * @brief
 *
 * @param   cmd, id, pmac, len
 *
 * @return  none
 */
void send_cmd_resp(uint8_t cmd, uint8_t id, uint8_t *pmac, uint8_t len) {
    uint8_t s[HEAD_MSG_LEN];
    uint16_t llen = len;
    memset(s, 0, HEAD_MSG_LEN);
    //s[0] = 0;
    s[1] = cmd; // rssi
    s[2] = id; // ev type
    s[3] = len; // addr type
    s[4] = 0xff; // phy = 0xff -> cmd response
    if(llen)
        memcpy(&s[5], pmac, (llen > 6)? 6 : llen);
    app_cmd_len = 0;
    llen = HEAD_MSG_LEN; // - 2;
/*
    uint16_t crc = crcFast(s, llen);
    s[llen++] = crc;
    s[llen++] = crc >> 8;
*/
//    app_drv_fifo_flush(&app_tx_fifo);
    app_drv_fifo_write(&app_tx_fifo, s, &llen);
    tmos_set_event(ObserverTaskId, NEW_BLEDATA_EVT);
}
/*********************************************************************
 * @fn      StopDiscovery
 *
 * @brief   Stop Discovery
 *
 * @param   none
 *
 * @return  none
 */
void StopDiscovery(void) {
    PRINT("Stop discovery\n");
    GAPRole_ObserverCancelDiscovery();
    app_drv_fifo_flush(&app_tx_fifo);
}
/*********************************************************************
 * @fn      parse_command
 *
 * @brief   Parse commands
 *
 * @param   none
 *
 * @return  none
 */
void parse_command(uint32_t len) {
    int cmd = app_cmd_buf[0];
    if(cmd == CMD_ID_SCAN && len >= 4) {
        uint8_t flg = app_cmd_buf[1];
        scan_params.phys = flg & 7;
        scan_params.scan_type = (flg >> 3) & 1; // =1 Passive scan, =0 Active scan
        scan_params.mode = (flg >> 4) & 3;
        mac_list.filtr = flg >> 6;

        scan_params.duration = app_cmd_buf[2] | (app_cmd_buf[3] << 8);

        if(scan_params.phys) {
            // Setup GAP
            GAP_SetParamValue(TGAP_DISC_SCAN, scan_params.duration);
            GAP_SetParamValue(TGAP_DISC_SCAN_PHY, scan_params.phys);
            GAP_SetParamValue(TGAP_FILTER_ADV_REPORTS, 0);

            GAPRole_ObserverStartDiscovery(scan_params.mode,
                                        scan_params.scan_type,
                                       DEFAULT_DISCOVERY_WHITE_LIST);
            PRINT("Start discovery\n");
        } else {
            StopDiscovery();
        }
        send_cmd_resp(cmd, mac_list.count, &app_cmd_buf[1], 3);
    } else if(len >= 6 + 1) {
        if(app_cmd_buf[0] == CMD_ID_WMAC) {
            mac_list.mode = WHITE_LIST;
            if(mac_list.count < MAC_MAX_SCAN_LIST) {
#if 1
                memcpy(&mac_list.mac[mac_list.count], &app_cmd_buf[1], 6);
#else
                mac_list.mac[mac_list.count][0] = app_cmd_buf[6];
                mac_list.mac[mac_list.count][1] = app_cmd_buf[5];
                mac_list.mac[mac_list.count][2] = app_cmd_buf[4];
                mac_list.mac[mac_list.count][3] = app_cmd_buf[3];
                mac_list.mac[mac_list.count][4] = app_cmd_buf[2];
                mac_list.mac[mac_list.count][5] = app_cmd_buf[1];
#endif
                mac_list.count++;
            }
            send_cmd_resp(cmd, mac_list.count, &app_cmd_buf[1], 6);
        } else if(app_cmd_buf[0] == CMD_ID_BMAC) {
            mac_list.mode = BALCK_LIST;
            if(mac_list.count < MAC_MAX_SCAN_LIST) {
#if 1
                memcpy(&mac_list.mac[mac_list.count], &app_cmd_buf[1], 6);
#else
                mac_list.mac[mac_list.count][0] = app_cmd_buf[6];
                mac_list.mac[mac_list.count][1] = app_cmd_buf[5];
                mac_list.mac[mac_list.count][2] = app_cmd_buf[4];
                mac_list.mac[mac_list.count][3] = app_cmd_buf[3];
                mac_list.mac[mac_list.count][4] = app_cmd_buf[2];
                mac_list.mac[mac_list.count][5] = app_cmd_buf[1];
#endif
                mac_list.count++;
            }
            send_cmd_resp(cmd, mac_list.count, &app_cmd_buf[1], 6);
        }
    } else if(len >= 1) {
        if(app_cmd_buf[0] == CMD_ID_CLRM)  {
            mac_list.count = 0;
            send_cmd_resp(cmd, MAC_MAX_SCAN_LIST, &app_cmd_buf[1], 0);
        }
        else if(app_cmd_buf[0] == CMD_ID_INFO) {
            GetMACAddress(app_cmd_buf);
            send_cmd_resp(cmd, SW_VERSION, app_cmd_buf, 6);
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
        if(app_cmd_len)
            parse_command(app_cmd_len);
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

    if(events & STOP_DISCOVERY_EVT)
    {
        StopDiscovery();
        return (events ^ STOP_DISCOVERY_EVT);
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

/*********************************************************************
 * @fn      chk_mac
 *
 * @brief   test mac black/white list
 *
 * @param   pmac - pointer to mac
 *
 * @return  none
 */
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

            PRINT("Init done...\n");
        }
        break;

        case GAP_DEVICE_INFO_EVENT:
        {
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
                len += HEAD_MSG_LEN;
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
        }
        break;


        case GAP_EXT_ADV_DEVICE_INFO_EVENT:
        {
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
                len += HEAD_MSG_LEN;
                app_drv_fifo_write(&app_tx_fifo, (uint8_t *)&adv_msg, &len);
                tmos_set_event(ObserverTaskId, NEW_BLEDATA_EVT);

            }
        }
        break;

        case GAP_DIRECT_DEVICE_INFO_EVENT:
        {
            if(chk_mac(pEvent->deviceDirectInfo.addr)
              && (pEvent->deviceInfo.addrType & mac_list.filtr) == 0) {
                adv_msg.len = 0;
                adv_msg.evTypes = pEvent->deviceInfo.eventType;
                adv_msg.adTypes = pEvent->deviceExtAdvInfo.addrType;
                adv_msg.phyTypes = GAP_PHY_BIT_LE_1M | (GAP_PHY_BIT_LE_1M << 4);
                adv_msg.rssi = pEvent->deviceDirectInfo.rssi;
                memcpy(adv_msg.addr, pEvent->deviceDirectInfo.addr, B_ADDR_LEN);
                len = HEAD_MSG_LEN;
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
