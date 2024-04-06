## V4.0.1.3_Patch_0002

### Bug Fixes
* **driver**
  * xtal
    * Solved the B85 problem that when the crystal oscillator is abnormal, the program crashes and the software cannot reset by itself.

  * uart
    * Fixed a bug that caused the uart_send_byte() interface to send bytes incorrectly if the DMA data was not sent to completion due to the function exiting and the array memory defined in the stack being freed.  
    * uart_clear_parity_error(): Fixed the problem that when clearing uart clear rx status, other uart status bits may be operated by mistake.

  * irq
    * irq_clr_src2(): Fixed the problem that when clearing the specified IRQ source, other IRQ source bits may be operated by mistake.

  * USB
    * usb_handle_irq(): Fixed the problem that when clearing USB reset status, other status bits may be operated by mistake

  * gpio
    * Fixed an issue in the gpio_set_interrupt()/gpio_set_interrupt_risc0()/gpio_set_interrupt_risc1() interface that could mistakenly clear other interrupt status bits.

  * timer
    * timer0_set_mode/timer1_set_mode/timer2_set_mode/timer_start/timer_stop: in addition to the corresponding functions, the interrupt flags of time0/timer1/timer2 are also incorrectly cleared on the preceding interfaces. this problem occurs in the following application scenarios: when multiple timers work at the same time and the interrupt time is very close, the interrupt interface processes the timer that requests the interrupt first. if the above interface is called, the interrupt flag of other timers will be mistakenly cleared, resulting in no response to other timers' interruption. at present, it is modified to only set the corresponding function.
    * Modify the timer0_gpio_init and timer1_gpio_init functions to fix the problem that the gpio interrupt will be accidentally touched when the gpio trigger and gpio width modes are used.

  * watchdog
    * wd_set_interval_ms/wd_start/wd_stop: in addition to the corresponding functions, the interrupt flags of time0/timer1/timer2 are also mistakenly cleared on the above interfaces. At present, they are modified to set only corresponding functions.

  * pm
    * cpu_stall_wakeup_by_timer0/cpu_stall_wakeup_by_timer1/cpu_stall_wakeup_by_timer2:add setting timer0/timer1/timer2 mode0 to prevent these interfaces from working until timer0/timer1/timer2 is not mode0.
    * Solved the problem that the sleep time is incorrect due to a small probability that the total interruption time of the shutdown is not timely.
    * Eliminates the risk that STATUS_GPIO_ERR_NO_ENTER_PM may be set to 1 in the return value of cpu_sleep_wakeup.
    * Resolved the risk that cpu_sleep_wakeup_32k_rc()/cpu_sleep_wakeup_32k_xtal() could crash due to misreading the 32K tick. (The digital reading 32K tick inside these two functions has been replaced with reading using analog registers).
    * Solved the B85 problem of disconnection caused by the 32k calibration value not being updated in time in high and low-temperature environments.

  * clock
    * rc_24m_cal() is added to the clock_init() function to solve the B85 problem of possible probabilistic crash.

* **Extended Scan**
  * Fixed the bug that creating connection could fail when using extended scan and times of creating connection have been exceeding 4.

* **SMP**
  * Fixed the bug that blc_smp_loadBondingInfoByAddr may load fail when the peer address is RPA.



### BREAKING CHANGES
* **pm** 
  * Delete the cpu_get_32k_tick interface. If you want to read a 32K tick value, use pm_get_32k_tick().



### Bug Fixes
* **driver**
  * xtal
    * 解决了B85晶振异常时，程序死机，不能自行软件复位的问题。

  * uart
    * 修复了由于 uart_send_byte() 函数退出后，DMA 还未将数据搬运完成前，函数内部定义的数据内存已经被释放，导致数据发送出错问题。
    * uart_clear_parity_error(): 修复了清除uart clear rx状态时，可能误操作其他uart状态位的问题。

  * irq
    * irq_clr_src2(): 修复了清除指定的中断源时，可能误操作其他中断源的状态位的问题。

  * usb
    * usb_handle_irq(): 修复了清除usb reset状态位时，可能误操作其他状态位的问题。

  * gpio
    * 修复了gpio_set_interrupt()/gpio_set_interrupt_risc0()/gpio_set_interrupt_risc1()接口中可能误清其他中断状态位的问题。
  * timer
    * timer0_set_mode/timer1_set_mode/timer2_set_mode/timer_start/timer_stop::以上这些接口除了设置了对应的功能以外，还误清了timer0、timer1、timer2的中断标志位。在如下应用场景下会有问题：当使用多个timer同时工作，并且产生的中断时间很接近时，中断接口中先处理第一个请求中断的timer时，调用了以上接口的话，将其他timer的中断标志位误清，导致其他timer中断没有响应，目前将其进行修改为只设置对应功能。
    * 修改timer0_gpio_init和timer1_gpio_init函数，修复了使用gpio trigger和gpio width模式会误触gpio中断的问题。

  * watchdog
    * wd_set_interval_ms/wd_start/wd_stop:以上这些接口除了设置了对应的功能以外，还误清了timer0、timer1、timer2的中断标志位，目前将其进行修改为只设置对应功能。

  * pm
    * cpu_stall_wakeup_by_timer0/cpu_stall_wakeup_by_timer1/cpu_stall_wakeup_by_timer2:添加设置timer0/timer1/timer2 mode0功能，避免在调用这些接口之前，timer0/timer1/timer2不是mode0，这些接口不能工作。

    * 解决了因关总中断时间不及时，小概率地导致的睡眠时间不对的问题。
    * 规避了cpu_sleep_wakeup的返回值中STATUS_GPIO_ERR_NO_ENTER_PM 可能会被异常置为1的风险。
    * 解决了cpu_sleep_wakeup_32k_rc()/cpu_sleep_wakeup_32k_xtal()可能因为读错32K tick引发死机的风险（已将这两个函数内部，数字读32K tick的方式替换成模拟方式读32Ktick）。
    * 解决了B85在高温和低温环境下，因32k校准值没有及时更新，导致的断联的问题。

  * clock
    * 在B85 clock_init函数中加入rc_24m_cal()解决可能概率性死机的问题.

* **Extended Scan**
  * 修复了当使用扩展扫描创建连接的次数超过4次时，创建连接会失败的问题。

* **SMP**
  * 修复了当对端地址为RPA时，blc_smp_loadBondingInfoByAddr可能会读取失败。



### BREAKING CHANGES
* **driver**
  * pm 
    * 删除cpu_get_32k_tick接口，如果想要读32K的tick值，可以使用pm_get_32k_tick()。


## V4.0.1.3_Patch_0001

### Bug Fixes
* Fixed: A very small number of chips rarely experience insufficient oscillation of the 24M crystal after waking up from sleep, leading to MCU abnormality.   
* Fixed: Pairing failure occurs when using Secure Connection with low power deepsleep retention mode.  
* Fixed: The primary Channel advertising packet can not be received when using Coded PHY in Extended Scan.  
* Fixed: The RSSI value is incorrect in the Extended advertising report event.  
* Fixed: The AUX_CONNECT_RSP PDU can not be received when initiating a connection request under Extended Scan Coded PHY S8 mode, resulting in creating connection failure.  

### BREAKING CHANGES
  - N/A


### Bug Fixes
* 修复: 极少数芯片，低概率出现睡眠唤醒后24M晶体起振不充分，导致MCU异常
* 修复: Secure Connection，低功耗使用deepsleep retention mode，出现配对失败。
* 修复: Extended Scan，无法收到Primary Channel使用Coded PHY的广播包。
* 修复: Extended Scan，Extended ADV report event中RSSI值异常。
* 修复: Extended Scan，在Coded PHY S8模式发起连接，会出现无法接收AUX_CONNECT_RSP数据包，导致创建连接失败。

### BREAKING CHANGES
  - N/A