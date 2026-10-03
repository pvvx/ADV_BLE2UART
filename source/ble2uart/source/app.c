#include "app_config.h"
#include "tl_common.h"
#include "stack/ble/ble.h"
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
	sched_timer_guard();    /* before the SDK runs: keep the System Timer alive */
	bb_checkpoint(BB_PHASE_SDK);
	blc_sdk_main_loop();
	bb_alive();
	bb_report_task();
	wd_clear();     /* feed the watchdog: 3 s of stall -> reset + report */
	while(clock_time() -  utc_time_sec_tick > utc_time_tick_step) {
		utc_time_sec_tick += utc_time_tick_step;
		utc_time_sec++; // + 1 sec
	}
	bb_checkpoint(BB_PHASE_SCAN);
	scan_task();
}
