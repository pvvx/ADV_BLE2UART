/********************************************************************************************************
 * @file    hci_tr.h
 *
 * @brief   This is the source file for BLE SDK
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
#ifndef HCI_TR_H_
#define HCI_TR_H_

#include "drivers.h"
#include "common/types.h"
#include "stack/ble/hci/hci.h"
#include "user_config.h"

#ifndef HCI_TR_EN
#define HCI_TR_EN            0
#endif


#if (__PROJECT_CONTROLLER__)
#include "vendor/ble_controller/app_buffer.h"
#else
#endif

/*! HCI transport layer protocol selection. */
#define HCI_TR_H4            0
#define HCI_TR_H5            1
#define HCI_TR_USB           2  /*!< Not currently supported */
#ifndef HCI_TR_MODE
#define HCI_TR_MODE          HCI_TR_H4
#endif

/*! HCI RX/TX path max buffer size define. */
#define HCI_TR_RX_BUF_SIZE   (HCI_RX_FIFO_SIZE)
#define HCI_TR_TX_BUF_SIZE   (HCI_TX_FIFO_SIZE)

/*! HCI UART transport pin define */
#ifndef HCI_TR_RX_PIN
#define HCI_TR_RX_PIN        UART_RX_PB0
#endif
#ifndef HCI_TR_TX_PIN
#define HCI_TR_TX_PIN        UART_TX_PA2
#endif
#ifndef HCI_TR_BAUDRATE
#define HCI_TR_BAUDRATE      (1000000)
#endif
/*! HCI UART transport Flow Control. */
#ifndef HCI_TR_FLOW_CTRL_EN
#define HCI_TR_FLOW_CTRL_EN  0
#endif
#if HCI_TR_FLOW_CTRL_EN
/*** RTS/CTS Pin ***/
#ifndef HCI_TR_RTS_PIN
#define HCI_TR_RTS_PIN       UART_RTS_PB3
#endif
#ifndef HCI_TR_CTS_PIN
#define HCI_TR_CTS_PIN       UART_CTS_PA3
#endif
#if 0
/*** RTS ***/
#define HCI_TR_RTS_INVERT    1 /*!< 0: RTS PIN will change from low to high. !! use for RTS auto mode(default auto mode)*/
#define HCI_TR_RTS_THRESH    5 /*!< RTS trigger threshold. range: 1-16. */
/*** CTS ***/
#define HCI_TR_CTS_POLARITY  1 /*!< 0: low level stop TX; 1: high level stop TX.*/
#endif

#endif

typedef void (*HciH5PacketHandler_t)(u8 *pPacket, u32 len);
 extern volatile u8 isUartTxDone;

/*! HCI transport layer API. */

/**
 * @brief : HCI transport initialization.
 * @param : none.
 * @param : none.
 */
void HCI_TransportInit(void);

/**
 * @brief : HCI transport main loop.
 * @param : none.
 * @param : none.
 */
void HCI_TransportPoll(void);

/**
 * @brief : HCI IRQ handler.
 * @param : none.
 * @param : none.
 */
void HCI_TransportIRQHandler(void);

#endif /* HCI_TR_H_ */
