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

#ifndef OBSERVER_H
#define OBSERVER_H

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

// Simple BLE Observer Task Events
#define START_DEVICE_EVT          0x0001
//#define START_SYNC_TIMEOUT_EVT    0x0002
#define NEW_USBDATA_EVT 0x0004
#define NEW_BLEDATA_EVT 0x0008


#define APP_TX_BUFFER_LENGTH            (1<<10) // 1024
#define APP_RX_BUFFER_LENGTH            (1<<8) // 256

extern app_drv_fifo_t app_tx_fifo;
extern app_drv_fifo_t app_rx_fifo;
/*********************************************************************
 * MACROS
 */
/*********************************************************************
 * CONSTANTS
 */

#define SW_VERSION  0x10

// Maximum number of scan responses
#define DEFAULT_MAX_SCAN_RES             64 // 58

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
 * TYPEDEFS
 */
typedef struct {
    uint8_t mode;   // =0 No discoverable, =1 General Discoverable, =2 Limited Discoverable, =3 Not filtered
    uint8_t phys;   // bit0 = GAP_PHY_BIT_LE_1M, bit1 = GAP_PHY_BIT_LE_2M, bit2 = GAP_PHY_BIT_LE_CODED
    uint8_t scan_type; // =0 Passive scan, =1 Active scan
    uint8_t own_addr_type; // =0 Public, =1 Random, =2 or =3 Resolvable Private Address
    uint16_t duration; // Scan duration in (625us)

}scan_params_t;
/*********************************************************************
 * GLOBAL VARIABLES
 */

extern uint8_t ObserverTaskId;
extern scan_params_t scan_params;

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

#endif /* OBSERVER_H */
