/********************************************************************************************************
 * @file    main.c
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
#include "app_config.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"


#if (FEATURE_TEST_MODE == TEST_PER_ADV)


/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 */
_attribute_ram_code_ void irq_handler(void)
{
    DBG_CHN15_HIGH;

	blc_sdk_irq_handler();

	DBG_CHN15_LOW;
}




/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ int main(void)
{

	#if(MCU_CORE_TYPE == MCU_CORE_825x)
		cpu_wakeup_init();
	#elif(MCU_CORE_TYPE == MCU_CORE_827x || MCU_CORE_TYPE == MCU_CORE_TC321X)
		cpu_wakeup_init(DCDC_LDO_MODE, INTERNAL_CAP_XTAL24M);
	#endif

	/* detect if MCU is wake_up from deep retention mode */
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	
	clock_init(SYS_CLK_TYPE);

	rf_drv_ble_init();

	gpio_init(!deepRetWakeUp);


	if(deepRetWakeUp){ //MCU wake_up from deepSleep retention mode
		user_init_deepRetn();
	}
	else{ //MCU power_on or wake_up from deepSleep mode
		user_init_normal();
	}

	irq_enable();

	while(1)
	{
		main_loop();
	}
	return 0;
}




#endif // of of (FEATURE_TEST_MODE == TEST_PERIODIC_ADVERTISING)
