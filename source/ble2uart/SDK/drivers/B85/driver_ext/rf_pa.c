/********************************************************************************************************
 * @file    rf_pa.c
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
#include "rf_pa.h"
#include "gpio.h"
#include "compiler.h"

_attribute_data_retention_	rf_pa_callback_t  blc_rf_pa_cb = 0;

_attribute_ram_code_
void app_rf_pa_handler(int type)
{
#if(PA_ENABLE)
	if(type == PA_TYPE_TX_ON){
	    gpio_write(PA_RXEN_PIN, 0);
	    gpio_write(PA_TXEN_PIN, 1);
	}
	else if(type == PA_TYPE_RX_ON){
	    gpio_write(PA_TXEN_PIN, 0);
	    gpio_write(PA_RXEN_PIN, 1);
	}
	else{
	    gpio_write(PA_RXEN_PIN, 0);
	    gpio_write(PA_TXEN_PIN, 0);
	}
#endif
}


void rf_pa_init(void)
{
#if(PA_ENABLE)
    //rf_set_power_level_index (RF_POWER_0dBm);
    gpio_set_func(PA_TXEN_PIN, AS_GPIO);
    gpio_set_output_en(PA_TXEN_PIN, 1);
    gpio_write(PA_TXEN_PIN, 0);

    gpio_set_func(PA_RXEN_PIN, AS_GPIO);
    gpio_set_output_en(PA_RXEN_PIN, 1);
    gpio_write(PA_RXEN_PIN, 0);

    blc_rf_pa_cb = app_rf_pa_handler;
#endif
}

