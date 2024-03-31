/********************************************************************************************************
 * @file	ext_rf.c
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
#include "drivers/8278/driver_8278.h"
#include "ext_rf.h"


 /**
 * This define for ble debug the effect of rx_dly.
 * when this function turn on the time of rx_dly will shorten 6.3us
 */
#define 	RF_RX_SHORT_MODE_EN				1


_attribute_data_retention_ signed char ble_txPowerLevel = 0; /* <<TX Power Level>>: -127 to +127 dBm */




/*********************************************************** RF_PA START********************************************/
_attribute_data_retention_	rf_pa_callback_t  blc_rf_pa_cb = 0;

_attribute_ram_code_ void app_rf_pa_handler(int type)
{
#if(PA_ENABLE)
	if(type == PA_TYPE_TX_ON){
	    gpio_set_output_en(PA_RXEN_PIN, 0);
	    gpio_write(PA_RXEN_PIN, 0);
	    gpio_set_output_en(PA_TXEN_PIN, 1);
	    gpio_write(PA_TXEN_PIN, 1);
	}
	else if(type == PA_TYPE_RX_ON){
	    gpio_set_output_en(PA_TXEN_PIN, 0);
	    gpio_write(PA_TXEN_PIN, 0);
	    gpio_set_output_en(PA_RXEN_PIN, 1);
	    gpio_write(PA_RXEN_PIN, 1);
	}
	else{
	    gpio_set_output_en(PA_RXEN_PIN, 0);
	    gpio_write(PA_RXEN_PIN, 0);
	    gpio_set_output_en(PA_TXEN_PIN, 0);
	    gpio_write(PA_TXEN_PIN, 0);
	}
#endif
}


void rf_pa_init(void)
{
#if(PA_ENABLE)
    //rf_set_power_level_index (RF_POWER_0dBm);
    gpio_set_func(PA_TXEN_PIN, AS_GPIO);
    gpio_set_output_en(PA_TXEN_PIN, 0);
    gpio_write(PA_TXEN_PIN, 0);

    gpio_set_func(PA_RXEN_PIN, AS_GPIO);
    gpio_set_output_en(PA_RXEN_PIN, 0);
    gpio_write(PA_RXEN_PIN, 0);

    blc_rf_pa_cb = app_rf_pa_handler;
#endif
}

/*********************************************************** RF_PA END********************************************/
//vulture
_attribute_ram_code_
void rf_start_fsm (fsm_mode_e mode, void* tx_addr, unsigned int tick)
{
	reg_rf_ll_cmd_schedule = tick;
	reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHDULE_EN;
	reg_rf_ll_cmd = mode;

	if(tx_addr){
		reg_dma_rf_tx_addr = (unsigned short)((unsigned int)tx_addr);
	}
}
