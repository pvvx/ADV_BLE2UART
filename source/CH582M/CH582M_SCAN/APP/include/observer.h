/********************************** (C) COPYRIGHT *******************************
 * File Name          : observer.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        : �۲�Ӧ��������������ϵͳ��ʼ��
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
#define SW_VERSION  0x12

// Simple BLE Observer Task Events
#define START_DEVICE_EVT          0x0001
#define START_SYNC_TIMEOUT_EVT    0x0002
#define STOP_DISCOVERY_EVT        0x0004
#define NEW_USBDATA_EVT           0x0008
#define NEW_BLEDATA_EVT           0x0010

#define FRAME_DATA_LEN           6
#define HEAD_CRC_ADD_LEN         13
#define MAX_CMD_BUFFER_LENGTH    64
#define MAX_ADV_PAYLOAD          229
#define TXADV_MAX_DATA_LEN       31
#define TXDATA_MAX_DATA_LEN      20

#define APP_TX_BUFFER_LENGTH      (1<<10) // fifo: 1024 bytes
#define APP_RX_BUFFER_LENGTH      MAX_CMD_BUFFER_LENGTH

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
    CMD_ID_PRNT     = 0x05, // print debug message
    CMD_ID_GPIO     = 0x06,
    CMD_ID_LED      = 0x07,
    CMD_ID_UART     = 0x08,
    CMD_ID_RFSDK    = 0x09,
    CMD_ID_VERSION  = 0x0a,
    CMD_ID_TXADV    = 0x0b,
    CMD_ID_CONN     = 0x0c,
    CMD_ID_TXDATA   = 0x0d,
    CMD_ID_RXDATA   = 0x0e,
    CMD_ID_VBAT     = 0x0f,
    CMD_ID_GPIOEVT  = 0x10
} CMD_ID_KEYS;

typedef enum {
    CMD_STATUS_OK = 0,
    CMD_STATUS_ARGS = 1,
    CMD_STATUS_PIN = 2,
    CMD_STATUS_DENIED = 3,
    CMD_STATUS_VALUE = 4
} cmd_status_t;

typedef enum {
    WHITE_LIST,
    BALCK_LIST
} mode_mac_list_e;

typedef struct {
    uint8_t enabled;
    uint8_t phys;              // bit0 = 1M, bit2 = Coded
    uint8_t active_scan;
    uint8_t duplicate_filter;
    uint8_t filter_random_addresses;
    uint8_t filter_private_addresses;
    uint8_t own_addr_type;
    uint16_t window_1m_units;     // in 0.625 ms units
    uint16_t window_coded_units;  // in 0.625 ms units
} scan_params_t;

typedef struct {
    u8  count;
    u8  mac[MAC_MAX_SCAN_LIST][6];
    u8  mac_len[MAC_MAX_SCAN_LIST];
} mac_filter_list_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern uint8_t ObserverTaskId;
extern scan_params_t scan_params;
extern app_drv_fifo_t app_tx_fifo;
extern uint8_t app_cmd_buf[APP_RX_BUFFER_LENGTH];
extern uint8_t app_cmd_len;
extern mac_filter_list_t white_list;
extern mac_filter_list_t black_list;

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
