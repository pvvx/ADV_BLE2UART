/********************************************************************************************************
 * @file	bis_bcst.h
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
#ifndef BIS_BCST_H_
#define BIS_BCST_H_

//Note: user can't modify this value,and this value must 4 byte aligned
#define 		BIG_BCST_PARAM_LENGTH		(672) //4B aligned


/**
 * @brief      This function is used to initialize BIG broadcast module.
 * @param	   none
 * @return     none
 */
void 		blc_ll_initBigBroadcast_module(void);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t	blc_ll_initBigBcstParameters(u8 *pBigBcstPara, u8 bigBcstNum);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_hci_le_createBigParams(hci_le_createBigParams_t* pCmdParam);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_hci_le_createBigParamsTest(hci_le_createBigParamsTest_t* pCmdParam);


/**
 * @brief
 * @param	   none
 * @return     none
 */
ble_sts_t 	blc_hci_le_terminateBig(hci_le_terminateBigParams_t* pCmdParam); //TODO: BIG BCST and BIG SYNC use common BIG CB


/**
 * @brief      Used to enable private ExtADV to send BIGINFO
 * @param	   ExtADV handle
 * @param	   If the BIS encrypted
 * @return     none
 */
void		blc_ll_enPrivExtAdvForBigBcst(u8 extAdvHdl);


#endif /* BIS_BCST_H_ */
