/********************************************************************************************************
 * @file    ext_misc.h
 *
 * @brief   This is the header file for B91
 *
 * @author  BLE Group
 * @date    2020
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
#ifndef DRIVERS_8278_EXT_MISC_H_
#define DRIVERS_8278_EXT_MISC_H_
#include "../register.h"



/******************************* rf start **********************************************************************/
/**
 * @brief     This function serves to set BLE mode of RF.
 * @return	  none.
 */
void rf_drv_ble_init();

#define		RF_POWER_P3dBm					RF_POWER_P2p93dBm
#define		RF_POWER_P0dBm					RF_POWER_N0p28dBm
/******************************* rf end  **********************************************************************/

/******************************* stimer_start ******************************************************************/
#define	SYSTICK_NUM_PER_US			16

#define	SSLOT_TICK_NUM					625/2     //attention: not use "()" for purpose !!!    625uS*16/32=625/2=312.5
#define	SSLOT_TICK_REVERSE				2/625	  //attention: not use "()" for purpose !!!

#define reg_system_tick_irq 		REG_ADDR32(0x744)//reg_system_tick_irq_level
enum{
	SYSTEM_TIMER_TICK_1US 		= 16,
	SYSTEM_TIMER_TICK_1MS 		= 16000,
	SYSTEM_TIMER_TICK_1S 		= 16000000,

	SYSTEM_TIMER_TICK_625US  	= 10000,  //625*16
	SYSTEM_TIMER_TICK_1250US 	= 20000,  //1250*16
};

enum {
	FLD_SYSTEM_TICK_IRQ_EN  = 		BIT(2),
};


/**
 * @brief define system clock tick per us/ms/s.
 */
enum{
	SYSTEM_TIMER_TICK_78US125 	= 1250,   //78.125uS = 78.125*16=1250 //78.125*SYSTEM_TIMER_TICK_1US
	SYSTEM_TIMER_TICK_125US 	= 2000,   //125*16   = 2000 //126*SYSTEM_TIMER_TICK_1US
	SYSTEM_TIMER_TICK_156US25 	= 2500,   // 156.25uS = 156.25*16=2500 //156.25*SYSTEM_TIMER_TICK_1US
	SYSTEM_TIMER_TICK_312US5 	= 5000,   // 312.5uS = 312.5*16=5000 //312.5*SYSTEM_TIMER_TICK_1US


	SYSTEM_TIMER_TICK_40MS 		= 640000,  //16000*40=640000
};


/**
 * @brief    This function serves to set system timer capture tick.
 * @return  none
 */
void systimer_set_irq_capture(unsigned int sys_timer_tick);

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
	reg_system_irq_mask |= FLD_SYSTEM_TICK_IRQ_EN;
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

static inline int tick1_exceed_tick2(unsigned int tick1, unsigned int tick2)
{
	return (unsigned int)(tick1 - tick2) < BIT(30);
}

/*
 * @brief     This function performs to get system timer tick.
 * @param[in] none.
 * @return    system timer tick value.
**/
static inline unsigned int sys_get_stimer_tick(void)
{
	return reg_system_tick; //same as API: clock_time()
}

/******************************* stimer_end ********************************************************************/



/******************************* timer_start ******************************************************************/

/******************************* timer_end ********************************************************************/



/******************************* aes_start ********************************************************************/
static inline void reset_aes(void)
{
	reg_rst1 = FLD_RST1_AES;	//reset_baseband
	reg_rst1 = 0;				//release reset signal
}
/******************************* aes_end **********************************************************************/



/******************************* core_start *******************************************************************/

/******************************* core_end *********************************************************************/




/******************************* analog_start *****************************************************************/

/******************************* analog_end *******************************************************************/



/******************************* random_start *****************************************************************/
/**
 * @brief      This function performs to get a serial of random number.
 * @param[in]  len- the length of random number
 * @param[in]  data - the first address of buffer store random number in
 * @return     the result of a serial of random number..
 */
void generateRandomNum(int len, unsigned char *data);
/******************************* random_end *******************************************************************/



/******************************* dma_start ********************************************************************/
enum{
	FLD_DMA_RPTR_MASK =			0x0F, // max 15
};

/**
 * @brief	ACL RX Data buffer length = maxRxOct + 21, then 16 Byte align
 *			maxRxOct + 21 = 4(DMA_len) + 2(BLE header) + maxRxOct + 4(MIC) + 3(CRC) + 8(ExtraInfo)
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
/******************************* dma_end **********************************************************************/

/******************************* analog_start *****************************************************************/
#include "../analog.h"
#define  analog_write_reg8  analog_write
#define  analog_read_reg8   analog_read

/******************************* analog_end *******************************************************************/




/******************************* flash_start ******************************************************************/
void flash_set_capacity(Flash_CapacityDef flash_cap);

Flash_CapacityDef flash_get_capacity(void);
/******************************* flash_end ********************************************************************/


#endif
