/********************************************************************************************************
 * @file	ll_pm.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	2020.06
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
#ifndef LL_PM_H_
#define LL_PM_H_



/**
 *  @brief
 */
typedef enum {
	PM_SLEEP_DISABLE		= 0,
	PM_SLEEP_LEG_ADV		= BIT(0),
	PM_SLEEP_LEG_SCAN 		= BIT(1),
	PM_SLEEP_ACL_SLAVE		= BIT(2),
	PM_SLEEP_ACL_MASTER 	= BIT(3),
}sleep_mask_t;



/**
 *  @brief
 */
/* DeepSleepRetention_Enable */
typedef enum {
	PM_DeepRetn_Disable = 0x00,
	PM_DeepRetn_Enable  = 0x01,
} deep_retn_en_t;


/**
 * @brief	for user to initialize low power mode
 * @param	none
 * @return	none
 */
void 		blc_ll_initPowerManagement_module(void);


/**
 * @brief   LinkLayer initialization after deepSleep retention wake_up
 * @param	none
 * @return	none
 */
void 		blc_ll_recoverDeepRetention(void);


/**
 * @brief	for user to set low power mode mask
 * @param	mask - low power mode mask
 * @return	none
 */
void 		blc_pm_setSleepMask (sleep_mask_t mask);

/**
 * @brief	for user to enable or disable deepSleep retention function
 * @param	en -  deepSleep retention enable, 1: enable; 0: disable
 * @return	none
 */
void 	blc_pm_setDeepsleepRetentionEnable (deep_retn_en_t en);

/**
 * @brief	for user to set low power mode wake up source
 * @param	wakeup_src - low power mode wake_up source
 * @return	none
 */
void		blc_pm_setWakeupSource (SleepWakeupSrc_TypeDef wakeup_src);


/**
 * @brief	for user to get low power mode wake up time
 * @param	none
 * @return	bltPm.current_wakeup_tick
 */
u32 		blc_pm_getWakeupSystemTick(void);

/**
 * @brief	for user to set the threshold of sleep tick for entering deep retention mode
 * @param	thres_ms - the threshold of time for suspend or deepsleep retention
 * @return  none.
 */
void 		blc_pm_setDeepsleepRetentionThreshold(u32 thres_ms);

/**
 * @brief	for user to set early wake up tick for deep retention mode
 * @param	earlyWakeup_us - early wake up tick for deep retention mode
 * @return  none.
 */
void 		blc_pm_setDeepsleepRetentionEarlyWakeupTiming(u32 earlyWakeup_us);

/**
 * @brief	for user to set the type of deep retention mode
 * @param	sleep_type - the type of deep retention mode
 * @return  none.
 */
void 		blc_pm_setDeepsleepRetentionType(SleepMode_TypeDef sleep_type);



/**
 * @brief	application wake up low power mode process callback function
 */
typedef 	void (*pm_appWakeupLowPower_callback_t)(int);

/**
 * @brief	for user to set application wake up low power mode
 * @param	wakeup_tick - low power mode wake up time
 * @param	enable - low power mode application wake up enable
 * @return	none
 */
void 		blc_pm_setAppWakeupLowPower(u32 wakeup_tick, u8 enable);

/**
 * @brief	for user to register the callback for application wake up low power mode process
 * @param	cb - the pointer of callback function
 * @return  none.
 */
void 		blc_pm_registerAppWakeupLowPowerCb(pm_appWakeupLowPower_callback_t cb);



#endif /* LL_PM_H_ */
