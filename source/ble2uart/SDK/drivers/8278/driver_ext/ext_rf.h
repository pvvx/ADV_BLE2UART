/********************************************************************************************************
 * @file	ext_rf.h
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

#ifndef DRIVERS_8278_EXT_RF_H_
#define DRIVERS_8278_EXT_RF_H_

extern signed char ble_txPowerLevel;

#define	RF_BLE_PACKET_VALIDITY_CHECK(p)		(RF_BLE_PACKET_LENGTH_OK(p) && RF_BLE_PACKET_CRC_OK(p))
#define rf_tx_packet_dma_len(len)           (len)

#define reg_rf_ll_ctrl3				REG_ADDR8(0xf16)
enum{
	FLD_RF_R_TX_EN_DLY_EN         =	BIT(0),
	FLD_RF_R_PLL_RESET_EN         =	BIT(1),
	FLD_RF_R_CMD_SCHDULE_EN       =	BIT(2),
	FLD_RF_R_PLL_EN_MAN           =	BIT(3),
	FLD_RF_R_T_TX_EN_DLY          =	BIT_RNG(4,7),
};
#define reg_rf_ll_cmd_2B			REG_ADDR16(0xf00)
#define reg_rf_ll_cmd				REG_ADDR8(0xf00)
enum{
	FLD_RF_R_CMD                 =	BIT_RNG(0,3),
	FLD_RF_R_STOP                =	0,
	FLD_RF_R_BTX                 =	1,
	FLD_RF_R_BRX                 =	2,
	FLD_RF_R_PTX                 =	3,
	FLD_RF_R_PRX                 =	4,
	FLD_RF_R_STX                 =	5,
	FLD_RF_R_SRX                 =	6,
	FLD_RF_R_STR                 =	7,
	FLD_RF_R_SRT                 =	8,
	FLD_RF_R_CMD_TRIG            =  BIT(7),
};


#define	STOP_RF_STATE_MACHINE		(reg_rf_ll_cmd = 0x80 )
//t_cmd_schedule[0:31]
#define reg_rf_ll_cmd_sch 			REG_ADDR32(0xf18)

//#define reg_rf_ll_pid_l			REG_ADDR8(0xf22)
enum{
	FLD_RF_SN = 		BIT(0),
};

#define reg_rf_ll_pid_l				REG_ADDR8(0xf22)
enum{
	FLD_RF_NESN	=       BIT(4),
};

#define reg_rf_ll_pid_h				REG_ADDR8(0xf23)
#define reg_rf_rx_1st_timeout		REG_ADDR32(0xf28) ///vulture single SDK not defined. compare with regiter table. vulture is same as kite.
#define reg_rf_ll_cmd_schedule   	reg_rf_ll_cmd_sch //REG_ADDR32(0xf18)
#define reg_rf_ll_rx_fst_timeout	reg_rf_rx_1st_timeout //REG_ADDR32(0xf28)


/**
 * @brief   This function serves to set RX first timeout value.
 * @param[in]   tm - timeout, unit: uS.
 * @return  none.
 */
static inline void rf_set_1st_rx_timeout(unsigned int tm)
{
	reg_rf_ll_rx_fst_timeout = tm;
}

/**
 * @brief   This function serves to set RX timeout value.
 * @param[in]   tm - timeout, unit: uS, range: 0 ~ 0xfff
 * @return  none.
 */
static inline void rf_ble_set_rx_timeout(unsigned short tm)
{
	reg_rf_rx_timeout = tm;
}
/**
 * @brief    This function serves to enable zb_rt interrupt source.
 * @return  none
 */
static inline void zb_rt_irq_enable(void)
{
	reg_irq_mask |= FLD_IRQ_ZB_RT_EN;
}


/**
 * @brief    This function serves to disable zb_rt [telink's ble irq entry] interrupt source.
 * @return  none
 */
static inline void zb_rt_irq_disable(void)
{
	reg_irq_mask &= ~FLD_IRQ_ZB_RT_EN;
}

static inline void ble_rf_set_rx_dma(unsigned char *buff, unsigned char fifo_size_div_16)
{
	reg_dma_rf_rx_addr = (unsigned short)(unsigned int)buff;
	reg_dma_rf_rx_size = fifo_size_div_16;
}

static inline void ble_rf_set_tx_dma(unsigned char fifo_dep, unsigned char fifo_byte_size)
{

}

/**
 * @brief   	This function serves to judge the statue of  RF receive.
 * @return  	none.
 */
static inline unsigned char rf_receiving_flag(void)
{
	//if the value of [5:4] of the reg_0x448 is 0b10 or 0b11, it means that the RF is in the receiving packet phase.(confirmed by junwen)
	return (((read_reg8(0x448)>>5)& 1) == 1);
}

/**
 * @brief   This function serves to settle adjust for RF Tx.
 * @param   txstl_us - adjust TX settle time.
 * @return  none
 */
static inline void 	rf_tx_settle_adjust(unsigned short txstl_us)
{
	REG_ADDR16(0xf04) = txstl_us;
}

/**
 * @brief  	   This function serves to set the max length of rx packet.Use byte_len to limit what DMA
 * 			   moves out will not exceed the buffer size we define.And old chip do this through dma size.
 * @param[in]  byte_len  the longest of rx packet.
 * @return     none.
 */

static inline void rf_set_rx_maxlen(unsigned int byte_len)
{
	REG_ADDR8(0x431) = byte_len; //#define reg_rf_rxtmaxlen	REG_ADDR8(0x431)
}

/**
 * @brief   This function serves to triggle accesscode in coded Phy mode.
 * @param   none.
 * @return  none.
 */
static inline void rf_trigle_codedPhy_accesscode(void)
{
	WRITE_REG8(0x405, REG_ADDR8(0x405) | BIT(7));
}








/******************************* rf_pa_start ******************************************************************/

#include "../gpio.h"


#ifndef PA_ENABLE
#define PA_ENABLE                           0
#endif



#ifndef PA_TXEN_PIN
#define PA_TXEN_PIN                         GPIO_PB2
#endif

#ifndef PA_RXEN_PIN
#define PA_RXEN_PIN                         GPIO_PB3
#endif



#define PA_TYPE_OFF							0
#define PA_TYPE_TX_ON						1
#define PA_TYPE_RX_ON						2


typedef void (*rf_pa_callback_t)(int type);
extern rf_pa_callback_t  blc_rf_pa_cb;



void rf_pa_init(void);
/******************************* rf_pa_end ******************************************************************/

typedef enum{
	FSM_BTX 	= 0x81,
	FSM_BRX 	= 0x82,
	FSM_STX 	= 0x85,
	FSM_SRX 	= 0x86,
	FSM_TX2RX	= 0x87,
	FSM_RX2TX	= 0x88,
}fsm_mode_e;


/**
 * @brief     	This function serves to RF trigger RF state machine.
 * @param[in] 	mode  - FSM mode.
 * @param[in] 	tx_addr  - DMA TX buffer, if not TX, must be "NULL"
 * @param[in] 	tick  - FAM Trigger tick.
 * @return	   	none.
 */
void rf_start_fsm(fsm_mode_e mode, void* tx_addr, unsigned int tick);




/**
 * @brief     This function performs to switch PHY test mode.
 * @param[in] mode - PHY mode
 * @return    none.
 */
void rf_switchPhyTestMode(RF_ModeTypeDef mode);

//TODO: need debug
#define 	LL_TX_STL_TIFS_1M								62
#define 	LL_TX_STL_TIFS_2M								(LL_TX_STL_TIFS_1M)
#define 	LL_TX_STL_TIFS_CODED							(LL_TX_STL_TIFS_1M + 40)

//TX settle time
#define		LL_TX_STL_ADV_1M								81
#define 	LL_SCAN_TX_SETTLE								63
#define 	LL_INIT_TX_SETTLE								56
#define     LL_SCANRSP_TX_SETTLE                            78
#define 	LL_SLAVE_TX_SETTLE								83
#define 	LL_MASTER_TX_SETTLE								83

#define		LL_TX_STL_ADV_2M								87
#define		LL_TX_STL_ADV_CODED								121

#define 	LL_SLAVE_TX_STL_2M								90
#define 	LL_SLAVE_TX_STL_CODED							122

#define		LL_MASTER_TX_STL_2M								90
#define		LL_MASTER_TX_STL_CODED							122

/* AD convert delay : timing cost on RF analog to digital convert signal process:
 * 					Eagle	1M: 20uS	   2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
 *					Jaguar	1M: 20uS	   2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
 */
#define		AD_CONVERT_DLY_1M								20
#define 	AD_CONVERT_DLY_2M								32
#define 	AD_CONVERT_DLY_CODED							14



static inline void rf_ble_set_1m_phy(void)
{
	write_reg32(0x1220, 0x23200a16);

	write_reg8(0x1273,0x01);  		//default: 01
	write_reg16(0x1236,0x8eb7);	//default: 71c48eb7
	write_reg16(0x1238,0x71c4);

//		write_reg8(0x401, 0x08);		//core_401 all 0x01 set in "rf_ble_1m_param_init", so no set here
	write_reg8(0x402, 0x46);
	write_reg8(0x404, 0xf5);
	write_reg8(0x405, 0x04);
	write_reg8(0x420, 0x1e);

	write_reg8(0x430, 0x36);		//tx timestamp capture disable
	write_reg32(0x460, 0x5f4f4434);  //grx_3~0
	write_reg16(0x464, 0x766b);      //grx_5~4
}

static inline void rf_ble_set_2m_phy(void)
{
	write_reg32(0x1220, 0x06432a04);
	write_reg8(0x1273,0x01);  		//default: 01
	write_reg16(0x1236,0x8eb7);	//default: 71c48eb7
	write_reg16(0x1238,0x71c4);
	//write_reg8(0x401, 0x08);		//core_401 all 0x01 set in "rf_ble_1m_param_init", so no set here
	write_reg8(0x402, 0x4c);   		//0x4a -> 0x4c: 12 - 2 = 10,  10*4=40uS, be same as 1M PHY 6 preamble timing
	write_reg8(0x404, 0xe5);
	write_reg8(0x405, 0x04);
	write_reg8(0x420, 0x1e);
	write_reg8(0x430, 0x34);		//tx timestamp capture disable
	write_reg32(0x460, 0x61514636);  //grx_3~0
	write_reg16(0x464, 0x786d);      //grx_5~4
}

static inline void rf_ble_set_coded_phy_common(void)
{
	write_reg32(0x1220, 0x23200a17);

	write_reg8(0x1273,0x21);  		//default: 01
	//write_reg8(0x1236,0xee);		//default: b7       //different value for S2/S8
	write_reg8(0x1237,0x8c);		//default: 8e
	write_reg16(0x1238,0x7dc8);		//default: 0x71c4

	//write_reg8(0x401, 0x08);		//core_401 all 0x01 set in "rf_ble_1m_param_init", so no set here
	write_reg8(0x402, 0x4a);
	write_reg8(0x404, 0xf5);
	//write_reg8(0x405, 0xa4);		//different value for S2/S8
	write_reg8(0x420, 0xf0);
	write_reg8(0x430, 0x34);		//tx timestamp capture disable

	write_reg32(0x460, 0x5f4f4434);  //grx_3~0
	write_reg16(0x464, 0x766b);      //grx_5~4
}
static inline void rf_ble_set_coded_phy_s2(void)
{
	write_reg8(0x1236,0xee);
	write_reg8(0x405, 0xa4);	//continuous writing core_405 will be ERROR
}

static inline void rf_ble_set_coded_phy_s8(void)
{
	write_reg8(0x1236,0xf6);
	write_reg8(0x405, 0xb4);
}


#endif

