/********************************************************************************************************
 * @file	cis_master.h
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
#ifndef CIS_MASTER_H_
#define CIS_MASTER_H_




#define 		CIG_MST_PARAM_LEN				(436)   //Note: user can't modify this value,and this value must 4 byte aligned
#define 		CIG_ID_0						0
#define 		CIG_ID_1		      			1
#define 		CIG_ID_2		              	2
#define 		CIG_ID_3		              	3
#define 		CIG_ID_INVALID		           	0xFF




/**
 * @brief      for user to initialize CIS master module.
 * @param	   none
 * @return     none
 */
void 		blc_ll_initCisMaster_module(void);


/**
 * @brief
 * @param	   none
 * @return     none
 */
void 		blc_ll_initCisMasterParameters( u8 *pCigPara, int cig_mst_num);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_setCigParams	   (hci_le_setCigParam_cmdParam_t* pCmdParam, 		hci_le_setCigParam_retParam_t* pRetParam);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_setCigParamsTest(hci_le_setCigParamTest_cmdParam_t* pCmdParam, 	hci_le_setCigParam_retParam_t* pRetParam);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_removeCig(u8 cigId, u8* pRetParamm);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t 	blc_hci_le_createCis(hci_le_CreateCisParams_t* pCisPara);



#endif
