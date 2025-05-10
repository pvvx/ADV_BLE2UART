/********************************************************************************************************
 * @file    clock.c
 *
 * @brief   This is the source file for TC321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "register.h"
#include "clock.h"
#include "irq.h"
#include "analog.h"
#include "timer.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"
#include "tl_common.h"
#include "lib/include/sys.h"

extern _attribute_data_retention_ unsigned char tl_24mrc_cal;

_attribute_data_retention_	unsigned char system_clk_type;
_attribute_data_retention_  unsigned char sys_clock_print;
/*24m rc calibrate configuration, the default value is set to require calibration*/
volatile static unsigned char g_24m_rc_calib_flag = 1;

extern unsigned char g_chip_version;

/**
 * @brief       This function to select the system clock source.
 * @param[in]   SYS_CLK - the clock source of the system clock.
 * @note		1. Do not switch the clock during the DMA sending and receiving process because during the clock switching process, 
 * 					the system clock will be suspended for a period of time, which may cause data loss.
 * 				2. When this function called after power on or deep sleep wakeup, it will perform 24m rc calibration. 
 * 					The usage rules of 24m rc, please refer to the rc_24m_cal() for details.
 * 					If do not want this logic, you can check the usage and precautions of clock_init_calib_24m_rc_cfg().
 */
_attribute_ram_code_sec_noinline_
void clock_init(SYS_CLK_TypeDef SYS_CLK)
{
	reg_clk_sel = (unsigned char)SYS_CLK;
	system_clk_type = (unsigned char)SYS_CLK;
	/*
		In some customer application scenarios, they want code execution time to be short and power consumption to be low.
		Meanwhile, they do not concerned about the accuracy of 24m rc or they want to control the calibration cycle themselves. 
		Therefore, here we provide a way for users to configure the calibration logic without affecting compatibility.
	*/
	if(!pm_is_MCU_deepRetentionWakeup() && (g_24m_rc_calib_flag == 1)){
		rc_24m_cal();
	}

	#if(CLOCK_SYS_CLOCK_HZ == 16000000)  //16M
		sys_clock_print = 16;
	#elif(CLOCK_SYS_CLOCK_HZ == 24000000)  //24M
		sys_clock_print = 24;
	#endif

}

/**
 * @brief   This function serves to set 32k clock source.
 * @return  none.
 */
void clock_32k_init(CLK_32K_TypeDef src)
{
	analog_write(areg_05_power, analog_read(areg_05_power) & (~FLD_32K_RC_PD));//32k rc
}

/**
 * @brief      This function serves to 24m rc calibration wait.
 * @return     1:busy  0: not busy
 */
static bool clock_24m_rc_cal_busy(void)
{
    return ((analog_read(areg_0xcf) & FLD_CAL_24M_DONE) == 0x00);
}

/**
 * @brief     This function performs to select 24M as the system clock source.
 * @param[in] none.
 * @return    none.
 * @note	  During the first power-on, after the xtal is stable (cpu_wakeup_init()), it is necessary to calibrate the 24m rc as soon as possible 
 * 				to prevent some unknown problems caused by a large frequency deviation of the RC clock.
 *            1. If the sleep function is not used and the accuracy of 24m rc is not high, then there is no need for regular calibration.
 *            2. If the sleep wake-up function is required, it is necessary to calibrate the 24m rc before the first sleep, otherwise it may cause the 
 * 					oscillator to fail to start after waking up.The recommended interval for regular calibration is 10 seconds. 
 *            3. If the 24m rc is more accurate, the oscillator will start up faster after waking up. If it is not accurate, the oscillator may not start
 * 					up after waking up.Therefore, regular calibration is needed to prevent the impact of temperature changes.
 */
void rc_24m_cal (void)
{
	analog_write(areg_cal_24m_wait_len, 0x80);//wait 24m rc stable cycles

	analog_write(areg_aon_0x4f, analog_read(areg_aon_0x4f) | FLD_RC_24M_CAP_SEL);

	analog_write(areg_0xc7, FLD_CAL_24M_RC_DISABLE);
	analog_write(areg_0xc7, FLD_CAL_24M_RC_ENABLE);

    while(clock_24m_rc_cal_busy());

    analog_write(areg_rc_24m_cap, analog_read(areg_rc_24m_cal_cap));//write 24m cap into manual register

    analog_write(areg_aon_0x4f, analog_read(areg_aon_0x4f) & (~FLD_RC_24M_CAP_SEL));

    analog_write(areg_0xc7, FLD_CAL_24M_RC_DISABLE);
    tl_24mrc_cal = analog_read(areg_rc_24m_cap);
}

/**
 * @brief      This function serves to 32k rc calibration wait.
 * @return     1:busy  0: not busy
 */
static bool clock_32k_rc_cal_busy(void)
{
    return ((analog_read(areg_0xcf) & FLD_CAL_32K_DONE) == 0x00);
}

/**
 * @brief     This function performs to select 32K as the system clock source.
 * @param[in] none.
 * @return    none.
 * @note	  1. If a more accurate 32K RC timing is required, then to prevent temperature effects, calibration can be performed regularly.
 * 			  2. If it is to ensure accurate sleep time, then the 32K RC calibration is not necessary. Although sleep time is measured by 32K RC, 
 * 				    sleep time is obtained through tracking way and will not affected by 32K RC deviation. So in this scenario, it is necessary to 
 * 				    calibrate once when power-on (to prevent significant frequency deviation caused by 32K RC), and regular calibration is not necessary.
 */
void rc_32k_cal (void)
{
	analog_write(areg_aon_0x4f, analog_read(areg_aon_0x4f) | FLD_RC_32K_CAP_SEL);//rc_32k_cap_sel, sel cap from calibration module
	
	if (g_chip_version == CHIP_VERSION_A0) {
		analog_write(areg_aon_rc32k_cap_bits_0_to_2, analog_read(areg_aon_rc32k_cap_bits_0_to_2) | BIT(2));//TODO:A0 default value error, correct it(Issue:RAPT-1). A1 will fix

		analog_write(areg_aon_rc32k_res_bits_0_to_5, analog_read(areg_aon_rc32k_res_bits_0_to_5) & 0xf8);
	}

	analog_write(areg_0xc6, FLD_CAL_32K_RC_DISABLE);//cal 32k rc disable
    analog_write(areg_0xc6, FLD_CAL_32K_RC_ENABLE);//cal 32k rc enable
	while(clock_32k_rc_cal_busy());

	unsigned char res1 = analog_read(areg_0xc9);	//read 32k res[13:6]
	analog_write(areg_aon_rc32k_res_bits_6_to_13, res1);//write 32k res[13:6] into manual register
	unsigned char res2 = analog_read(areg_0xca);	//read 32k res[5:0]
	if (g_chip_version == CHIP_VERSION_A0) {
		analog_write(areg_aon_rc32k_res_bits_0_to_5, res2);//write 32k res[5:0] into manual register
	} else {
		//TODO:A1 need reconfirm.
	}
	analog_write(areg_0xc6, FLD_CAL_32K_RC_DISABLE);//cal 32k rc disable

	analog_write(areg_aon_0x4f, analog_read(areg_aon_0x4f) & (~FLD_RC_32K_CAP_SEL));//cal_32k_cap_sel: manual
}


/**
 * @brief     This function performs to probe clock to IO.
 * @param[in] src - the clock source which you want to probe.
 * @param[in] pin - the pin to probe clock, only support PB[0]/PD0/PE[1:0].
 * @return    none.
 */
void clock_prob(prob_clock_src_e src, GPIO_PinTypeDef pin)
{
	write_reg8(0x75, ((read_reg8(0x75) & 0xf8) | src));	//0:clk_7816,  1:clk32k,   2:clk_sys,      3:rc24m
												  	    //4:xtl24m,    5:clkpll,   6:clk_stimer,
	BM_CLR(reg_gpio_func(pin), (pin&0xff));

	if (pin == GPIO_PB0) {
		reg_gpio_func_mux(pin) = 19;// PB0 as CLK_7816
	} else if (pin == GPIO_PD0) {
		reg_gpio_func_mux(pin) = 0;// PD0 as CLK_7816
	} else if ((pin == GPIO_PE0) || (pin == GPIO_PE1)) {
		reg_gpio_func_mux(pin) = 2;// PE0/PE1 as CLK_7816
	}	
}

/**
 * @brief 	  This function performs to configure whether to calibrate the 24m rc in the clock_init() when power-on or wakeup from deep sleep mode.
 * 				If wakeup from deep retention sleep mode will not calibrate.
 * @param[in] calib_flag - Choose whether to calibrate the 24m rc or not.
 * 						1 - calibrate; 0 - not calibrate
 * @return	  none
 * @note	  This function will not take effect until it is called before clock_init(). 
 */
void clock_init_calib_24m_rc_cfg(char calib_flag)
{
	g_24m_rc_calib_flag = calib_flag;
}

/**
 * @brief  This function serves to set the 32k tick.
 * @param  none.
 * @return none.
 */
_attribute_ram_code_sec_noinline_ void cpu_set_32k_tick(unsigned int tick)
{

    reg_system_ctrl |= FLD_SYSTEM_32K_WR_EN;    // <0>: 1- write , 0- read
    while(reg_system_status & FLD_SYSTEM_32K_TIMER_BUSY);

    reg_system_32k_tick_wt = tick;

    reg_system_status = FLD_SYSTEM_32K_TIMER_SYNC_TRIG;

    asm("tnop");asm("tnop");asm("tnop");asm("tnop");
    asm("tnop");asm("tnop");asm("tnop");asm("tnop");
    asm("tnop");asm("tnop");asm("tnop");asm("tnop");
    asm("tnop");asm("tnop");asm("tnop");asm("tnop");
    while(reg_system_status & FLD_SYSTEM_32K_TIMER_SYNC_TRIG);
}

/**
 * Reading a 32k tick value using a digital register may cause a crash, so an analog register is used instead.
 * add by bingyu.li,confirmed by jianzhi.chen
 */
#if 1
/**
 * @brief   This function serves to get the 32k tick.
 * @param   none
 * @return  variable of 32k tick.
 */
_attribute_ram_code_sec_noinline_ unsigned int pm_get_32k_tick(void)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int n = 0;

    while (1) {

        t0 = t1;
        t1 = analog_read(0x63);
        t1 <<= 8;
        t1 += analog_read(0x62);
        t1 <<= 8;
        t1 += analog_read(0x61);
        t1 <<= 8;
        t1 += analog_read(0x60);
        if (n)
        {
            /**
             * The 32k tick value is read byte by byte. If the read occurs during a jump on the edge, the result will be that several bytes are the value of the previous tick and several bytes are the
             * value of the later tick, resulting in an incorrect tick value. Therefore, after reading the analog register, add a judgment: (t1-t0) is <2 (that is, t1-t0 is 0/1, 0/1 is normal).
             * add by bingyu.li,confirmed by sihui.wang
             */
            if ((unsigned int)(t1 - t0) < 2) {
                return t1;
            }
            /**
             * After checking with BLE team sihui and sunwei, the code for the "else if" branch is redundant, but it has been used for a long time, so it is still used today.
             * add by bingyu.li,confirmed by sihui.wang
             */
            else if ( (t0^t1) == 1 ) {
                return t0;
            }
        }
        n++;
    }
    return t1;
}
#else
/**
 * @brief   This function serves to get the 32k tick.
 * @param   none
 * @return  variable of 32k tick.
 */
_attribute_ram_code_sec_noinline_ _attribute_no_inline_ unsigned int cpu_get_32k_tick (void)
{
//  analog_write(0x44, analog_read(0x64) | BIT(1));             //clear timer wakeup flag(can not clear when read 32k)
//  reg_system_irq_mask &= (~BIT(2));                           //disable system timer irq
    //system_timer and 32kshould_cal should be disable before suspend(added by Yi Bao, confired by junwen and zhidong 20191015)
    reg_system_ctrl &= (~(FLD_SYSTEM_32K_WR_EN|FLD_SYSTEM_TIMER_AUTO_EN));             // <0>: 1- write , 0- read
    while((reg_system_status & FLD_SYSTEM_32K_TIMER_UPDATE_RD));
    while(!(reg_system_status & FLD_SYSTEM_32K_TIMER_UPDATE_RD));
    reg_system_ctrl |= FLD_SYSTEM_TIMER_AUTO_EN;
    return reg_system_32k_tick_rd;
}
#endif

