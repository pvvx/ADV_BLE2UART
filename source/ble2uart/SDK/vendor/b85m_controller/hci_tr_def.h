/********************************************************************************************************
 * @file	hci_tr_def.h
 *
 * @brief	This is the source file for BLE SDK
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
#ifndef HCI_TR_DEF_H_
#define HCI_TR_DEF_H_


/*!  HCI Transmit Type. */
#define HCI_TR_TYPE_NONE      0x00
#define HCI_TR_TYPE_CMD       0x01
#define HCI_TR_TYPE_ACL       0x02
#define HCI_TR_TYPE_SCO       0x03
#define HCI_TR_TYPE_EVENT     0x04

/*!  HCI Head length define. */
#define HCI_CMD_HEAD_LEN  3
#define HCI_ACL_HEAD_LEN  4
#define HCI_EVT_HEAD_LEN  2
#define HCI_SCO_HEAD_LEN  3


/*! H5 packet length macro. */
#define HCI_H5_HEAD_LEN        4
#define HCI_H5_CRC_LEN         2


/*! Common macro. */
#define BYTES_TO_UINT8(n, p)      {n = (u8)(p)[0];}

#define BYTES_TO_UINT16(n, p)     {n = ((u16)(p)[0] + ((u16)(p)[1]<<8));}

#define BYTES_TO_UINT32(n, p)     {n = ((u32)(p)[0] + ((u32)(p)[1] << 8) + \
                                       ((u32)(p)[2] << 16) + ((u32)(p)[3] << 24));}


#define BSTREAM_TO_UINT8(n, p)    {BYTES_TO_UINT8(n, p);  p += 1;}
#define BSTREAM_TO_UINT16(n, p)   {BYTES_TO_UINT16(n, p); p += 2;}
#define BSTREAM_TO_UINT32(n, p)   {BYTES_TO_UINT32(n, p); p += 4;}

#define UINT8_TO_BSTREAM(p, n)    {*(p)++ = (u8)(n);}
#define UINT16_TO_BSTREAM(p, n)   {*(p)++ = (u8)(n); *(p)++ = (u8)((n) >> 8);}
#define UINT32_TO_BSTREAM(p, n)   {*(p)++ = (u8)(n); *(p)++ = (u8)((n) >> 8); \
                                   *(p)++ = (u8)((n) >> 16); *(p)++ = (u8)((n) >> 24);}

#define COUNTOF(x)    ( sizeof((x))/sizeof((x)[0]) )

#define MEM_SIZE_ALIGN4(x)    (((x) + 3) & ~3)
#define MEM_SIZE_ALIGN16(x)   (((x) + 15) & ~15)



















/*! Debug system -- Author: GaoQiu. */

//#define HCI_TR_DEBUG

#ifdef HCI_TR_DEBUG
#define ASSERT(exp, errCode) \
do{ \
	if(!(exp)) {REG_ADDR32(0x40000) = errCode; while(1);}\
}while(0)

#else
#define ASSERT(exp, errCode)
#endif

/*! Common error */
#define HCI_TR_ERR_INVALID_PARAM        0x00010000

/*! HCI H5 Protocol error */
#define HCI_TR_ERR_H5_HEAD_LEN          0x01010000
#define HCI_TR_ERR_H5_TX_BUF_OVFL       0x01020000
#define HCI_TR_ERR_H5_HCI_RX_BUF_OVFL   0x01030000

/*! HCI Slip Protocol error */
#define HCI_TR_ERR_SLIP_ESCAPE          0x02010000
#define HCI_TR_ERR_SLIP_ENCODE_BUF      0x02020000
#define HCI_TR_ERR_SLIP_DECODE_BUF      0x02030000

/*! HCI Transport error */
#define HCI_TR_ERR_TR_BACKUP_BUF        0x03010000
#define HCI_TR_ERR_TR_TX_BUF            0x03020000


#ifdef HCI_TR_DEBUG
#define HCI_TRACK_DATA(pData, len) \
do{ \
	printf("{ "); \
	for(int i=0; i<(len); i++){ \
		printf("%02X ", (pData)[i]);\
	}\
	printf("}\n");\
}while(0);

#define H5_TRACK(format, ...)          printf(format, ##__VA_ARGS__)
#define H5_TRACK_ERR(format, ...)      printf("[ERR -H5  ] "format, ##__VA_ARGS__)
#define H5_TRACK_INFO(format, ...)     printf("[INFO-H5  ] "format, ##__VA_ARGS__)
#define H5_TRACK_WRN(format, ...)      printf("[WRAN-H5  ] "format, ##__VA_ARGS__)

#define SLIP_TRACK(format, ...)        printf(format, ##__VA_ARGS__)
#define SLIP_TRACK_ERR(format, ...)    printf("[ERR -SLIP] "format, ##__VA_ARGS__)
#define SLIP_TRACK_INFO(format, ...)   printf("[INFO-SLIP] "format, ##__VA_ARGS__)
#define SLIP_TRACK_WRN(format, ...)    printf("[WRAN-SLIP] "format, ##__VA_ARGS__)

#define TR_TRACK(format, ...)          printf(format, ##__VA_ARGS__)
#define TR_TRACK_ERR(format, ...)      printf("[ERR -TR  ] "format, ##__VA_ARGS__)
#define TR_TRACK_INFO(format, ...)     printf("[INFO-TR  ] "format, ##__VA_ARGS__)
#define TR_TRACK_WRN(format, ...)      printf("[WRAN-TR  ] "format, ##__VA_ARGS__)

#else
#define HCI_TRACK_DATA(pData, len)

#define H5_TRACK(format, ...)
#define H5_TRACK_ERR(format, ...)
#define H5_TRACK_INFO(format, ...)
#define H5_TRACK_WRN(format, ...)

#define SLIP_TRACK(format, ...)
#define SLIP_TRACK_ERR(format, ...)
#define SLIP_TRACK_INFO(format, ...)
#define SLIP_TRACK_WRN(format, ...)

#define TR_TRACK(format, ...)
#define TR_TRACK_ERR(format, ...)
#define TR_TRACK_INFO(format, ...)
#define TR_TRACK_WRN(format, ...)
#endif

#endif /* HCI_TR_DEF_H_ */




