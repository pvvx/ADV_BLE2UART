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