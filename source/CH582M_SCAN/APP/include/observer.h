/********************************** (C) COPYRIGHT *******************************
 * File Name          : observer.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        : 观察应用主函数及任务系统初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef _OBSERVER_H
#define _OBSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "app_drv_fifo.h"

/*********************************************************************
 * CONSTANTS
 */
#define SW_VERSION  0x11

// Simple BLE Observer Task Events
#define START_DEVICE_EVT          0x0001
#define START_SYNC_TIMEOUT_EVT    0x0002
#define STOP_DISCOVERY_EVT        0x0004
#define NEW_USBDATA_EVT           0x0008
#define NEW_BLEDATA_EVT           0x0010

#define HEAD_MSG_LEN    11 // if CRC = 13

#define APP_TX_BUFFER_LENGTH      (1<<10) // fifo: 1024 bytes
#define APP_RX_BUFFER_LENGTH      HEAD_MSG_LEN

#define MAC_MAX_SCAN_LIST   64

// Maximum number of scan responses
#define DEFAULT_MAX_SCAN_RES             0

// Scan duration in (625us)
#define DEFAULT_SCAN_DURATION            4800

// Creat sync timeout in (625us)
#define DEFAULT_CREAT_SYNC_TIMEOUT       4800

// Discovey mode (limited, general, all)
#define DEFAULT_DISCOVERY_MODE           DEVDISC_MODE_ALL

// TRUE to use active scan
#define DEFAULT_DISCOVERY_ACTIVE_SCAN    FALSE

// TRUE to use white list during discovery
#define DEFAULT_DISCOVERY_WHITE_LIST     FALSE

/*********************************************************************
 * MACROS
 */
/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    CMD_ID_INFO     = 0x00,
    CMD_ID_SCAN     = 0x01, //  Scan on/off, parameters
    CMD_ID_WMAC     = 0x02, // add white mac
    CMD_ID_BMAC     = 0x03, // add black mac
    CMD_ID_CLRM     = 0x04, // clear mac list
    CMD_ID_PRNT     = 0x05  // print debug message
} CMD_ID_KEYS;

typedef enum {
    WHITE_LIST,
    BALCK_LIST
} mode_mac_list_e;

typedef struct {
    uint8_t mode;   // =0 No discoverable, =1 General Discoverable, =2 Limited Discoverable, =3 Not filtered
    uint8_t phys;   // bit0 = GAP_PHY_BIT_LE_1M, bit1 = GAP_PHY_BIT_LE_2M, bit2 = GAP_PHY_BIT_LE_CODED
    uint8_t scan_type; // =0 Passive scan, =1 Active scan
    uint8_t own_addr_type; // =0 Public, =1 Random, =2 or =3 Resolvable Private Address
    uint16_t duration; // Scan duration in (625us)
} scan_params_t;


typedef struct _mac_list_t {
    u8  mode;   // mode_mac_list_e
    u8  count;
    u8  filtr;
    u8  res;
    u8  mac[MAC_MAX_SCAN_LIST][6];
} mac_list_t;

typedef struct _adv_msg_t {
    uint8_t     len;
    int8_t      rssi;                     //!< Advertisement or SCAN_RSP RSSI
    uint8_t     evTypes;                  //event type
    uint8_t     adTypes;                  //address type: @ref GAP_ADDR_TYPE_DEFINES | adv type
    uint8_t     phyTypes;                 // PHY primary | secondary
    uint8_t     addr[B_ADDR_LEN];         //!< Address of the advertisement or SCAN_RSP
    uint8_t     data[255];
} adv_msg_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern uint8_t ObserverTaskId;
extern scan_params_t scan_params;
extern app_drv_fifo_t app_tx_fifo;
extern uint8_t app_cmd_buf[APP_RX_BUFFER_LENGTH];
extern uint8_t app_cmd_len;
extern mac_list_t mac_list;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void Observer_Init(void);

/*
 * Task Event Processor for the BLE Application
 */
extern uint16_t Observer_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _OBSERVER_H */
