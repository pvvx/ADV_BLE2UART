/********************************************************************************************************
 * @file    hci_dfu_def.h
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
#ifndef HCI_DFU_DEF_H_
#define HCI_DFU_DEF_H_

#define HCI_OPCODE(ogf, ocf)      (((ogf)<<10) + (ocf))
#define HCI_OGF(opcode)           ((opcode)>>10)
#define HCI_OCF(opcode)           ((opcode)& 0x03ff)
/**
 * @Name: OGF define.
 */
#define HCI_OGF_VS                      0x3F    /*!< HCI VS Command OGF define. */
#define HCI_OGF_CONTROLLER              0x03    /*!< Controller and baseband */
#define HCI_OGF_INFORMATIONAL           0x04    /*!< Informational parameters */


/**
 * @Name: OCF define.
 */


/*! HCI VS Command OCF define. */
#define HCI_OCF_VS_START_DFU            0x0300
#define HCI_OCF_VS_END_DFU              0x0301
#define HCI_OCF_VS_FW_DATA              0x0302


/**
 * @Name: HCI Opcode define.
 */
/*! HCI VS Command opcode define. */
#define HCI_OPCODE_VS_START_DFU         HCI_OPCODE(HCI_OGF_VS, HCI_OCF_VS_START_DFU)
#define HCI_OPCODE_VS_END_DFU           HCI_OPCODE(HCI_OGF_VS, HCI_OCF_VS_END_DFU)
#define HCI_OPCODE_VS_FW_DATA           HCI_OPCODE(HCI_OGF_VS, HCI_OCF_VS_FW_DATA)



//#define DFU_DEBUG
#ifdef DFU_DEBUG
	#define DFU_TRACK_DATA(pData, len) \
	do{ \
		printf("{ "); \
		for(int i=0; i<(len); i++){ \
			printf("0x%02X ", (pData)[i]);\
		}\
		printf("}\n");\
	}while(0);


	#define DFU_TRACK(format, ...)          printf(format, ##__VA_ARGS__)
	#define DFU_TRACK_ERR(format, ...)      printf("[ERR -DFU ] "format, ##__VA_ARGS__)
	#define DFU_TRACK_INFO(format, ...)     printf("[INFO-DFU ] "format, ##__VA_ARGS__)
	#define DFU_TRACK_WRN(format, ...)      printf("[WRAN-DFU ] "format, ##__VA_ARGS__)
#else
	#define DFU_TRACK_DATA(pData, len)

	#define DFU_TRACK(format, ...)
	#define DFU_TRACK_ERR(format, ...)
	#define DFU_TRACK_INFO(format, ...)
	#define DFU_TRACK_WRN(format, ...)
#endif

#endif /* HCI_DFU_DEF_H_ */
