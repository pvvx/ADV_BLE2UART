/********************************************************************************************************
 * @file	gatt.h
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
#ifndef GATT_H_
#define GATT_H_

/* GATT data handle call-back. */
typedef int (*gatt_handler_t) (u16 conn, u8 * p);

/**
 * @brief       Register GATT data handle call-back.
 * @param[in]   handler  -
 * @return      none.
 */
void 		blc_gatt_register_data_handler(gatt_handler_t handler);

/**
 * @brief       Send ATT Value Notify.
 * @param[in]   connHandle   - connection handle.
 * @param[in]   attHandle    - ATT handle.
 * @param[in]   p            - Pointer point to application data buffer.
 * @param[in]   len          - the length of data.
 * @return      none.
 */
ble_sts_t	blc_gatt_pushHandleValueNotify  (u16 connHandle, u16 attHandle, u8 *p, int len);

/**
 * @brief       Send ATT Indicate.
 * @param[in]   connHandle   - connection handle.
 * @param[in]   attHandle    - ATT handle.
 * @param[in]   p            - Pointer point to application data buffer.
 * @param[in]   len          - the length of data.
 * @return      none.
 */
ble_sts_t	blc_gatt_pushHandleValueIndicate(u16 connHandle, u16 attHandle, u8 *p, int len);

/**
 * @brief       Send ATT Write Command.
 * @param[in]   connHandle   - connection handle.
 * @param[in]   attHandle    - ATT handle.
 * @param[in]   p            - Pointer point to application data buffer.
 * @param[in]   len          - the length of data.
 * @return      none.
 */
ble_sts_t 	blc_gatt_pushWriteCommand (u16 connHandle, u16 attHandle, u8 *p, int len);

/**
 * @brief       Send ATT Write Request.
 * @param[in]   connHandle   - connection handle.
 * @param[in]   attHandle    - ATT handle.
 * @param[in]   p            - Pointer point to application data buffer.
 * @param[in]   len          - the length of data.
 * @return      none.
 */
ble_sts_t 	blc_gatt_pushWriteRequest (u16 connHandle, u16 attHandle, u8 *p, int len);

/**
 * @brief       Send ATT Find Info Request.
 * @param[in]   connHandle         - connection handle.
 * @param[in]   start_attHandle    - the start of ATT handle.
 * @param[in]   end_attHandle      - the end of ATT handle.
 * @return      none.
 */
ble_sts_t 	blc_gatt_pushFindInformationRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle);

/**
 * @brief       Send ATT Find By Type Value Request.
 * @param[in]   connHandle         - connection handle.
 * @param[in]   start_attHandle    - the start of ATT handle.
 * @param[in]   end_attHandle      - the end of ATT handle.
 * @param[in]   uuid               - attribute type.
 * @param[in]   attr_value         - Pointer point to value buffer.
 * @param[in]   len                - the length of value.
 * @return      none.
 */
ble_sts_t   blc_gatt_pushFindByTypeValueRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle,
		                                        u16 uuid, u8 *attr_value, int len);
/**
 * @brief       Send ATT Read By Type Request.
 * @param[in]   connHandle         - connection handle.
 * @param[in]   start_attHandle    - the start of ATT handle.
 * @param[in]   end_attHandle      - the end of ATT handle.
 * @param[in]   uuid               - attribute type.
 * @param[in]   uuid_len           - the length of attribute type.
 * @return      none.
 */
ble_sts_t   blc_gatt_pushReadByTypeRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle, u8 *uuid, int uuid_len);

/**
 * @brief       Send ATT Read Request.
 * @param[in]   connHandle         - connection handle.
 * @param[in]   attHandle          - ATT handle.
 * @return      none.
 */
ble_sts_t   blc_gatt_pushReadRequest(u16 connHandle, u16 attHandle);

/**
 * @brief       Send ATT Read Blob Request.
 * @param[in]   connHandle         - connection handle.
 * @param[in]   attHandle          - ATT handle.
 * @param[in]   offset             - ATT value offset.
 * @return      none.
 */
ble_sts_t   blc_gatt_pushReadBlobRequest(u16 connHandle, u16 attHandle, u16 offset);

/**
 * @brief       Send ATT Read By Group Type Request.
 * @param[in]   connHandle         - connection handle.
 * @param[in]   start_attHandle    - the start of ATT handle.
 * @param[in]   end_attHandle      - the end of ATT handle.
 * @param[in]   uuid               - Pointer point to attribute type.
 * @param[in]   uuid_len           - the length of attribute type.
 * @return      none.
 */
ble_sts_t 	blc_gatt_pushReadByGroupTypeRequest(u16 connHandle, u16 start_attHandle, u16 end_attHandle,
		                                        u8 *uuid, int uuid_len);

/**
 * @brief       Send ATT Err Response.
 * @param[in]   connHandle         - connection handle.
 * @param[in]   reqOpcode          - reqOpcode.
 * @param[in]   attHdlInErr        - attHdlInErr.
 * @param[in]   ErrorCode
 * @return      ble_sts_t.
 */
ble_sts_t	blc_gatt_pushErrResponse(u16 connHandle, u8 reqOpcode, u16 attHdlInErr, u8 ErrorCode);

/**
 * @brief       Send ATT handle value confirm.
 * @param[in]   connHandle         - connection handle.
 * @return      ble_sts_t.
 */
ble_sts_t blc_gatt_pushAttHdlValueCfm(u16 connHandle);


#endif /* GATT_H_ */
