/********************************************************************************************************
 * @file	bis_sync.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	2021.02
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
#ifndef BIS_SYNC_H_
#define BIS_SYNC_H_


#define 		BIG_SYNC_PARAM_LENGTH		(696) // Note: user can't modify this value,and this value must 4 byte aligned



/**
 * @brief      This function is used to initialize BIG Synchronize module.
 * @param	   none
 * @return     none
 */
void 		blc_ll_initBisSynchronize_module(void);

/**
 * @brief      This function is used to initialize BIG Synchronize parameters.
 * @param	   pointer to BIG Synchronize parameters buffer
 * @return     Number of BIG SYNC supported
 */
ble_sts_t	blc_ll_initBigSyncParameters(u8 *pBigSyncPara, u8 bigSyncNum);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_hci_le_bigCreateSync(hci_le_bigCreateSyncParams_t* pCmdParam);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_hci_le_bigTerminateSync(u8 bigHandle, u8* pRetParam);


/**
 * @brief      Used to enable private LegScan to get BIGINFO for BIG SYNC
 * @param	   none
 * @return     none
 */
void		blc_ll_enPrivLegScanForBigBync(void);


/**
 * @brief      Used to enable scan to get BIGINFO for BIG SYNC
 * @param	   none
 * @return     none
 */
void		blc_ll_enScanForBigBync(void);

/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_ll_bigCreateSync(u8 big_handle, u16 sync_handle, u8 enc, u8 broadcast_code[16],
								 u8 mse, u16 big_sync_timeout, u8  num_bis, u8 *bis);

/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_ll_bigTerminateSync(u8 bigHandle);


#endif /* BIS_SYNC_H_ */
