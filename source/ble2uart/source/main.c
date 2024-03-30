#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "app_config.h"

#if (MCU_CORE_TYPE == MCU_CORE_9518)
	/* default flash is 1M
	 * for 1M Flash, flash_sector_mac_address equals to 0xFF000
	 * for 2M Flash, flash_sector_mac_address equals to 0x1FF000 */
	_attribute_ble_data_retention_	u32 flash_sector_mac_address = CFG_ADR_MAC;
	_attribute_ble_data_retention_	u32 flash_sector_calibration = CFG_ADR_CALIBRATION;
#else
	/* default flash is 512K
	 * for 512K Flash, flash_sector_mac_address equals to 0x76000
	 * for 1M Flash, flash_sector_mac_address equals to 0xFF000
	 * for 2M Flash, flash_sector_mac_address equals to 0x1FF000 */
	_attribute_ble_data_retention_	u32 flash_sector_mac_address = CFG_ADR_MAC;
	_attribute_ble_data_retention_	u32 flash_sector_calibration = CFG_ADR_CALIBRATION;
#endif

/**
 * @brief   IRQ handler
 * @param   none.
 * @return  none.
 */
_attribute_ram_code_ void irq_handler(void)
{
    DBG_CHN15_HIGH;

	blc_sdk_irq_handler ();

	DBG_CHN15_LOW;
}

/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ int main (void) {    //must run in ramcode
	blc_pm_select_internal_32k_crystal();

	#if(MCU_CORE_TYPE == MCU_CORE_825x)
		cpu_wakeup_init();
	#elif(MCU_CORE_TYPE == MCU_CORE_827x)
		cpu_wakeup_init(DCDC_MODE, EXTERNAL_XTAL_24M);
	#endif

	/* detect if MCU is wake_up from deep retention mode */
	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	clock_init(SYS_CLK_TYPE);

	rf_drv_init(RF_MODE_BLE_1M);

	gpio_init(!deepRetWakeUp);  // this uses SDK/drivers/82x8/gpio_default_82x8.h

    // Initialize ports for the used LEDs
    int leds[] = { GPIO_LED_B, GPIO_LED_R, GPIO_LED_G, GPIO_LED_E, GPIO_LED_W };
    for (size_t i = 0; i < sizeof(leds)/sizeof(leds[0]); i++) {
        gpio_set_func(leds[i], AS_GPIO);
        gpio_set_output_en(leds[i], 1);
        gpio_set_data_strength(leds[i], 1);
    }

    // Initialize UART RX port
    gpio_set_func(GPIO_RX, AS_GPIO);
    gpio_set_input_en(GPIO_RX, 1);
    gpio_setup_up_down_resistor(GPIO_RX, PM_PIN_PULLUP_1M);

    // Initialize UART TX port
    gpio_set_func(GPIO_TX, AS_GPIO);
    gpio_set_output_en(GPIO_TX, 1);
    gpio_set_data_strength(GPIO_TX, 1);
    gpio_setup_up_down_resistor(GPIO_TX, PM_PIN_PULLUP_1M);

	adc_power_on_sar_adc(0); // - 0.4 mA
	lpc_power_down();

	user_init_normal();

	/* load customized freq_offset cap value.
	 */
	blc_app_loadCustomizedParameters();

    irq_enable();
	while(1) {
		main_loop();
	}
	return 0;
}
