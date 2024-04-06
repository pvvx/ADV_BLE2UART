#include "app_config.h"
#include "tl_common.h"
//#include "drivers.h"
#include "stack/ble/ble.h"
//#include "vendor/common/blt_common.h"
//#include "cmd_parser.h"
#include "ble.h"
#include "app.h"
#include "scanning.h"


RAM u32 utc_time_sec;	// clock in sec (= 0 -> 1970-01-01 00:00:00)
#define utc_time_tick_step CLOCK_16M_SYS_TIMER_CLK_1S
RAM u32 utc_time_sec_tick;

//------------------ user_init_normal -------------------
void user_init_normal(void) {
//this will get executed one time after power up
	random_generator_init(); //must
	init_ble();
}

//----------------------- main_loop()
void main_loop(void) {
	blc_sdk_main_loop();
	while(clock_time() -  utc_time_sec_tick > utc_time_tick_step) {
		utc_time_sec_tick += utc_time_tick_step;
		utc_time_sec++; // + 1 sec
	}
	scan_task();
}
