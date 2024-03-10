/********************************************************************************************************
 * @file	ext_misc.h
 *
 * @brief	This is the header file for B91
 *
 * @author	BLE Group
 * @date	2020
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

#ifndef DRIVERS_8258_EXT_MISC_H_
#define DRIVERS_8258_EXT_MISC_H_

#include <tl_common.h>

//#define GET_WORD_LEN(bitLen)     ((bitLen+31)/32)
//#define GET_BYTE_LEN(bitLen)     ((bitLen+7)/8)

/***************************************************************************/
//for debug
#define	DBG_SRAM_ADDR					0x40000
#define write_dbg32(addr, value)   		write_reg32(addr, value)

/******************************* stimer_start ******************************************************************/
#define	SYSTICK_NUM_PER_US			16
/**
 * @brief    This function serves to set system timer capture tick.
 * @return  none
 */
static inline void systimer_set_irq_capture(unsigned int tick)
{
	reg_system_tick_irq = tick;
}

/**
 * @brief    This function serves to get system timer capture tick.
 * @return  none
 */
static inline unsigned int systimer_get_irq_capture(void)
{
	return reg_system_tick_irq;
}

/**
 * @brief    This function serves to set system timer interrupt mask.
 * @return  none
 */
static inline void systimer_set_irq_mask(void)
{
	reg_system_tick_mode |= FLD_SYSTEM_TICK_IRQ_EN;
}

/**
 * @brief    This function serves to enable system timer interrupt.
 * @return  none
 */
static inline void systimer_irq_enable(void)
{
	reg_irq_mask |= FLD_IRQ_SYSTEM_TIMER;
}

/**
 * @brief    This function serves to disable system timer interrupt.
 * @return  none
 */
static inline void systimer_irq_disable(void)
{
	reg_irq_mask &= ~FLD_IRQ_SYSTEM_TIMER;
}

/**
 * @brief    This function serves to clear system timer interrupt status.
 * @return  none
 */
static inline void systimer_clr_irq_status(void)
{
	reg_irq_src = FLD_IRQ_SYSTEM_TIMER;
}

/**
 * @brief    This function serves to get system timer interrupt status.
 * @return  none
 */
static inline unsigned int systimer_get_irq_status(void)
{
	return reg_irq_src & FLD_IRQ_SYSTEM_TIMER;
}

static inline int tick1_exceed_tick2(u32 tick1, u32 tick2)
{
	return (u32)(tick1 - tick2) < BIT(30);
}

/*
 * @brief     This function performs to get system timer tick.
 * @param[in] none.
 * @return    system timer tick value.
**/
static inline unsigned int  sys_get_stimer_tick(void)
{
	return reg_system_tick; //same as API: clock_time()
}

/******************************* stimer_end ********************************************************************/


/******************************* dma_start ******************************************************************/

/**
 * @brief	ACL RX Data buffer length = maxRxOct + 21, then 16 Byte align
 *			maxRxOct + 21 = 4(DMA_len) + 2(BLE header) + maxRxOct + 4(MIC) + 3(CRC) + 8(ExtraInfor)
			RX buffer size must be be 16*n, due to MCU design
 */
//actually +21.The purpose of +22 is to deal with extreme situations. Due to DMA design,at least one byte buffer can not be unusable.
#define 	CAL_LL_ACL_RX_FIFO_SIZE(maxRxOct)	(((maxRxOct+22) + 15) / 16 *16)


/**
 * @brief	ACL TX Data buffer length = maxTxOct + 10, then 4 Byte align
 *			maxTxOct + 10 = 4(DMA_len) + 2(BLE header) + maxTxOct + 4(MIC)
			TX buffer size must be be 4*n, due to MCU design
 */
#define 	CAL_LL_ACL_TX_FIFO_SIZE(maxTxOct)	(((maxTxOct+10) + 3) / 4 *4)



/*HCI TX RX buffer len = uart_fifo+ dma 4byte */
#define 	HCI_FIFO_SIZE(n)					(((n+7) + 15) / 16 *16)

/*
 * DMA_LEN(4B)+Hdr(2B)+PLD(251B)+MIC(4B)+CRC(3B)+TLK_PKT_INFO(8B)
 *             **use 2B enough**
 */
#define		ISO_PDU_SIZE_ALLIGN16(n)			(((n + 21) + 15) / 16 * 16) //4+2+4+2+4+3+8


/*
* DMA_LEN(4B)+Hdr(2B)+PLD(251B)+MIC(4B)+CRC(3B)+TLK_PKT_INFO(12B)
*             **use 2B enough**
*/
#define		ISO_BIS_RX_PDU_SIZE_ALLIGN16(n)			(((n + 25) + 15) / 16 * 16) //4+2+4+2+4+3+12
/******************************* dma_end ******************************************************************/

/******************************* core_start ******************************************************************/

/******************************* core_end ********************************************************************/




/******************************* analog_start ******************************************************************/
#include "../analog.h"
#define  analog_write_reg8  analog_write
#define  analog_read_reg8   analog_read

/******************************* analog_end ********************************************************************/



/******************************* flash_start *****************************************************************/
void flash_set_capacity(Flash_CapacityDef flash_cap);

Flash_CapacityDef flash_get_capacity(void);
/******************************* flash_end *******************************************************************/



#endif
