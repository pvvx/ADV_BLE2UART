/********************************************************************************************************
 * @file    usb.h
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
#pragma once


#include "tl_common.h"
#include "drivers.h"
#include "usbdesc.h"

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

enum {
    //  3000 ms
	USB_TIME_BEFORE_ALLOW_SUSPEND = (3000*1000),
};

enum {
	USB_IRQ_SETUP_REQ = 0,
	USB_IRQ_DATA_REQ,
};


// telink usb report ctrl command. used mixed with USB_AUD_PLAY_PAUSE...
enum{
	USB_REPORT_NO_EVENT		= 0xf0,
	USB_REPORT_RELEASE 		= 0xff,
};

#if (USB_MIC_ENABLE)
extern u8 usb_alt_intf[USB_INTF_MAX];
static inline int usb_mic_is_enable(){
	return usb_alt_intf[USB_INTF_MIC];
}
#endif

extern u8 usb_just_wakeup_from_suspend;
extern u8 usb_has_suspend_irq;
extern u8 edp_toggle[8];

void usb_init(void);

void usb_handle_irq(void);

#ifndef		USB_SOFTWARE_CRC_CHECK
#define		USB_SOFTWARE_CRC_CHECK		0
#endif

#define MS_VENDORCODE            'T'    //This must match the char after the "MSFT100"
#define STRING_MSFT              L"MSFT100T"

#define MS_OS_DESCRIPTOR_ENABLE        0

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

