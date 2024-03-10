/********************************************************************************************************
 * @file	iso_test.h
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

#ifndef STACK_BLE_CONTROLLER_LL_ISO_ISO_TEST_H_
#define STACK_BLE_CONTROLLER_LL_ISO_ISO_TEST_H_




/**
 * @brief      This function is used to initialize LE ISO test mode.
 * @param	   none
 * @return     none
 */
void  blc_initIsoTestMode(void);


/**
 * @brief      This function is used to enter ISO test mode, only for testing purposes.
 * @param	   *pCmdParam
 * @return     ble_sts_t
 */
ble_sts_t blc_hci_le_iso_transmit_test_cmd(hci_le_isoTransmitTestCmdParams_t *pCmdParam);


ble_sts_t blc_hci_le_iso_receive_test_cmd(hci_le_isoReceiveTestCmdParams_t *pCmdParam);

ble_sts_t blc_hci_le_iso_read_test_count_cmd(u16 handle, hci_le_isoReadTestCountersCmdRetParam_t *ret);
ble_sts_t blc_hci_le_iso_test_end_count_cmd(u16 handle, hci_le_isoTestEndCmdRetParam_t *ret);

#endif /* STACK_BLE_CONTROLLER_LL_ISO_ISO_TEST_H_ */
