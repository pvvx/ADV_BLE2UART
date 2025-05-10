# V4.0.2.1(PR)

### Version
* SDK version: tc_ble_sdk V4.0.2.1
* Chip Version
  - B85: TLSR825X
  - B87: TLSR827X
  - TC321X (A0)
* Hardware Version
  - B85: C1T139A30_V1_2, C1T139A3_V2_0
  - B87: C1T197A30_V1_1, C1T201A3_V1_0
  - TC321X: C1T357A20_V1_1, C1T362A5_V1_0
* Platform Version
  - tc_platform_sdk V3.1.0
* Toolchain Version
  - TC32 ELF GCC4.3 ( IDE: [Telink IDE](https://wiki.telink-semi.cn/wiki/IDE-and-Tools/IDE-for-TLSR8-Chips/)/ [TelinkIoTStudio_V2025.2](https://doc.telink-semi.cn/tools/telink_iot_studio/TelinkIoTStudio_V2025.2.zip) )

### Features
* **Application**
  - Add demo "ble_remote" into TC321X, which supports "TL_AUDIO_RCU_ADPCM_GATT_TELINK" audio mode.
  - Add ble_controller demo.

### Bug Fixes
* **Application**
  - Fix(TC321X): The flash information is not initialized normally when enabling macro "APP_FLASH_PROTECTION_ENABLE" and using Flash "P25Q80SU".
  - Fix: Fix compile error when enabling macro "APP_BATT_CHECK_ENABLE".

* **PM**
  - Fix(TC321X): The MCU can not be waken up normally when using long suspend sleep mode.

### Refactoring
* **Application**
  - Modify "USED_DEEP_ANA_REG" setting in "acl_c1p1_demo", "acl_peripheral_demo" and "acl_connection_demo".

### BREAKING CHANGES
* N/A.

### Flash
* **B85**
  - ZB25WD40B
  - ZB25WD80B
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E

* **B87**
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E
  - ZB25WD40B
  - ZB25WD80B

* **TC321X**
  - P25D40SU
  - GD25LD40EGEG (Not Test)
  - P25Q80SU
  - P25Q16SU

### CodeSize
* **B85**
  * acl_peripheral_demo
    - Firmware size: 81.4 kBytes
    - SRAM size: 31.9 kBytes
    - deepsleep retention SRAM size: 28.3 kBytes
  * acl_central_demo
    - Firmware size: 72.8 kBytes
    - SRAM size: 37.0 kBytes
  * acl_connection_demo
    - Firmware size: 87.3 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 93.4 kBytes
    - SRAM size: 36.0 kBytes
    - deepsleep retention SRAM size: 31.9 kBytes

* **B87**
  * acl_peripheral_demo
    - Firmware size: 79.4 kBytes
    - SRAM size: 31.7 kBytes
    - deepsleep retention SRAM size: 28.1 kBytes
  * acl_central_demo
    - Firmware size: 71.8 kBytes
    - SRAM size: 37.0 kBytes
  * acl_connection_demo
    - Firmware size: 91.2 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 91.3 kBytes
    - SRAM size: 35.8 kBytes
    - deepsleep retention SRAM size: 31.7 kBytes

* **TC321X**
  * acl_peripheral_demo
    - Firmware size: 79.3 kBytes
    - SRAM size: 31.8 kBytes
    - deepsleep retention SRAM size: 28.3 kBytes
  * acl_central_demo
    - Firmware size: 71.6 kBytes
    - SRAM size: 37.3 kBytes
  * acl_connection_demo
    - Firmware size: 86.7 kBytes
    - SRAM size: 43.3 kBytes
  * acl_c1p1_demo
    - Firmware size: 91.5 kBytes
    - SRAM size: 35.9 kBytes
    - deepsleep retention SRAM size: 32.0 kBytes
  * ble_remote
    - Firmware size: 95.5 kBytes
    - SRAM size: 40.4 kBytes
    - deepsleep retention SRAM size: 35.5 kBytes

### Version
* SDK 版本: tc_ble_sdk V4.0.2.1
* Chip 版本
  - B85: TLSR825X
  - B87: TLSR827X
  - TC321X (A0)
* Hardware 版本
  - B85: C1T139A30_V1_2, C1T139A3_V2_0
  - B87: C1T197A30_V1_1, C1T201A3_V1_0
  - TC321X: C1T357A20_V1_1, C1T362A5_V1_0
* Platform 版本
  - tc_platform_sdk V3.1.0
* Toolchain 版本
  - TC32 ELF GCC4.3 ( IDE: [Telink IDE](https://wiki.telink-semi.cn/wiki/IDE-and-Tools/IDE-for-TLSR8-Chips/)/ [TelinkIoTStudio_V2025.2](https://doc.telink-semi.cn/tools/telink_iot_studio/TelinkIoTStudio_V2025.2.zip) )

### Features
* **Application**
  - 在TC321X中添加例程“ble_remote”，支持“TL_AUDIO_RCU_ADPCM_GATT_TELINK” audio模式。
  - 添加ble_controller demo。

### Bug Fixes
* **Application**
  - 修复（TC321X）：使能宏“APP_FLASH_PROTECTION_ENABLE”，且使用flash“P25Q80SU”时，flash信息不能正常初始化。
  - Fix: 使能宏“APP_BATT_CHECK_ENABLE”时的编译错误。

* **PM**
  - 修复（TC321X）：使用long suspend休眠模式时，MCU无法正常唤醒。

### Refactoring
* **Application**
  - 修改“acl_c1p1_demo”、“acl_peripheral_demo”和“acl_connection_demo”中的“USED_DEEP_ANA_REG”设置。

### BREAKING CHANGES
* N/A.

### Flash
* **B85**
  - ZB25WD40B
  - ZB25WD80B
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E

* **B87**
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E
  - ZB25WD40B
  - ZB25WD80B

* **TC321X**
  - P25D40SU
  - GD25LD40EGEG （未测试）
  - P25Q80SU
  - P25Q16SU

### CodeSize
* **B85**
  * acl_peripheral_demo
    - Firmware size: 81.4 kBytes
    - SRAM size: 31.9 kBytes
    - deepsleep retention SRAM size: 28.3 kBytes
  * acl_central_demo
    - Firmware size: 72.8 kBytes
    - SRAM size: 37.0 kBytes
  * acl_connection_demo
    - Firmware size: 87.3 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 93.4 kBytes
    - SRAM size: 36.0 kBytes
    - deepsleep retention SRAM size: 31.9 kBytes

* **B87**
  * acl_peripheral_demo
    - Firmware size: 79.4 kBytes
    - SRAM size: 31.7 kBytes
    - deepsleep retention SRAM size: 28.1 kBytes
  * acl_central_demo
    - Firmware size: 71.8 kBytes
    - SRAM size: 37.0 kBytes
  * acl_connection_demo
    - Firmware size: 91.2 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 91.3 kBytes
    - SRAM size: 35.8 kBytes
    - deepsleep retention SRAM size: 31.7 kBytes

* **TC321X**
  * acl_peripheral_demo
    - Firmware size: 79.3 kBytes
    - SRAM size: 31.8 kBytes
    - deepsleep retention SRAM size: 28.3 kBytes
  * acl_central_demo
    - Firmware size: 71.6 kBytes
    - SRAM size: 37.3 kBytes
  * acl_connection_demo
    - Firmware size: 86.7 kBytes
    - SRAM size: 43.3 kBytes
  * acl_c1p1_demo
    - Firmware size: 91.5 kBytes
    - SRAM size: 35.9 kBytes
    - deepsleep retention SRAM size: 32.0 kBytes
  * ble_remote
    - Firmware size: 95.5 kBytes
    - SRAM size: 40.4 kBytes
    - deepsleep retention SRAM size: 35.5 kBytes




# V4.0.2.0(PR)

### Version
* SDK version: tc_ble_sdk V4.0.2.0
* Chip Version
  - B85: TLSR825X
  - B87: TLSR827X
  - TC321X (A0)
* Hardware Version
  - B85: C1T139A30_V1_2, C1T139A3_V2_0
  - B87: C1T197A30_V1_1, C1T201A3_V1_0
  - TC321X: C1T357A20_V1_1
* Platform Version
  - tc_platform_sdk V3.1.0
* Toolchain Version
  - TC32 ELF GCC4.3 ( IDE: [Telink IDE](https://wiki.telink-semi.cn/wiki/IDE-and-Tools/IDE-for-TLSR8-Chips/)/ [TelinkIoTStudio_V2024.8](https://wiki.telink-semi.cn/tools_and_sdk/Tools/IoTStudio/TelinkIoTStudio_V2024.8.zip) )

### Note
* All changes up to telink_b85m_ble_sdk V4.0.1.3_Patch0003 are included.

* TC321x has not been fully tested, subsequent iterations will be updated.

* B85m_controller has been removed in this version, and will be added back in the next version.

* Battery Voltage Check is a very important function for mass production. The user must check battery voltage to prevent abnormal writing or erasing Flash at a low voltage.
	
* Flash protection is a critical function for mass production. 
	- Flash protection is enabled by default in SDK. User must enable this function on their final mass-production application.
	- Users should use "Unlock" command in the Telink BDT tool for Flash access during the development and debugging phase.
	- Flash protection demonstration in SDK is a reference design based on sample code. Considering that the user's final application may be different from the sample code, for example, the user's final firmware size is bigger, or the user has a different OTA design, or the user needs to store more data in some other area of Flash, all these differences imply that Flash protection reference design in SDK can not be directly used on user's mass production application without any change. User should refer to sample code, understand the principles and methods, and then change and implement a more appropriate mechanism according to their application if needed.

* Correct some spelling mistakes
	- For example, change OTA_DATA_UNCOMPLETE to OTA_DATA_INCOMPLETE.

### Features
* **Chip**
  - Support TC321X A0
      - Support 1M phy and 2M phy.
      - Support PM, including suspend mode, deepsleep retention mode and deepsleep mode.
      - Support Flash Protection.
      - Support Battery Voltage Check.
      - Support Demo:
          - acl_connection_demo
          - feature_test
          - acl_c1p1_demo
          - acl_peripheral_demo
          - acl_central_demo

* **Application**
	- Add Flash Protection module and sample 
		- Add flash_prot.h/flash_prot.c in vendor/common folder as a common module for all applications.
		- Add Flash protection sample code in project acl_connection_demo/acl_c1p1_demo/acl_central_demo/acl_peripheral_demo. User can refer to the sample code and make sure the Flash protection function	must be enabled on their own application.
		- "app_flash_protection_operation" is a common entrance for all Flash operations (Flash write and erase), Flash locking and unlocking are processed in this function.
		- "blc_appRegisterStackFlashOperationCallback" and "flash_prot_op_cb" are used to process some stack internal Flash operation callback which may need Flash locking and unlocking.
		- The Telink OTA and SMP module already add callbacks related to blc_flashProt to inform the application layer that they need to write/erase Flash. Then, the application layer processes Flash lock and unlock.
		- If users do not use Telink OTAs, they need to refer to this design to add similar callback functions in their own OTA code for upper-layer use.
		- Now only firmware is protected in the sample code. System data and user data are not protected now.
		- IRQ disabling time too long in driver API "flash_write_status", BLE system interrupt is delayed, causing RX data error. 
		- Driver API "flash_write_status" be replaced by "flash_write_status" in flash_prot.c to solve this issue by calling LinkLayer API "blc_ll_write_flash_status".

	- Add tlkapi_debug module to print logs
		- Add tlkapi_debug.h/tlkapi_debug.c in vendor/common folder as a common module for all applications.
		- Add debug.h in stack/ble/debug. Add APIs "blc_debug_enableStackLog", "blc_debug_addStackLog", "blc_debug_removeStackLog" to control the print stack log.
		- Use GPIO simulating UART TX sequence to output data information to PC UART tool, same function with "printf" function in old SDK.
		- API "tlkapi_printf" and "tlkapi_send_string_data" are recommended for usage, with some macro in app_config.h to control log enabling such as "APP_LOG_EN".
		- Only 1M baud rate can be used. When the system clock is 16M/24M, the assembly is used to control the UART TX timing. When the system clock is 32M/48M, the system_timer is used to control the UART TX timing. IRQ disabling and restoring are used in function "uart_putc" to guarantee UART TX byte data integrity is not broken by IRQ. 
		- If the baud rate is too low, IRQ disabling time too long, BLE system interrupt and user interrupt will be affected which may lead to critical error.

* **feature_test**
	- Add feature_privacy_peripheral: Sample code for testing peripheral RPA. This sample code to get peer Central(Master) IDA(identity address) if RPA(resolved private address) is used in packet "CONNECT_IND".
	- Add feature_privacy_central: Sample code for testing Central RPA.
  - Add feature_l2cap_coc: Sample code for testing peripheral L2CAP COC.
  - Add Sample code for SC OOB in feature_smp_security.
  - Add feature_phy_test: Sample code for testing PHY functions.
  - Remove feature_misc.

* **Controller**
  - Add APIs:
    - "blc_ll_setAdvCustomizedChannel" is used to set some other channel to replace primary advertising chn37/38/39 in ADV state.
    - "blc_ll_configLegacyAdvEnableStrategy", which is used to configure leg_adv enabling by API only.
    - "blc_ll_setScanCustomizedChannel" is used to set some other channel to replace primary advertising chn37/38/39 in SCAN state.
    - "blc_ll_configScanEnableStrategy" is used to configure scan enable by API.
    - "blc_ll_isInitiationBusy" is used to check if initiation is busy.
    - "blc_ll_setCreateConnectionRetryNumber" is used to set create connection retry number.
    - "blc_ll_disableConnUpdHighAuthority" is used to disable connection update high authority for central.
    - "blc_ll_getAclLatestRSSI" is used to obtain the latest RSSI of ACL connections.
    - "blc_ll_getAclConnectionInterval" is used to read current ACL connection interval.
    - "blc_ll_getAclConnectionLatency" is used to read current ACL connection latency.
    - "blc_ll_getAclConnectionTimeout" is used to read current ACL connection supervision timeout.
    - "blc_ll_dataLenAutoExgDisable" is used to disable DLE exchange automatically.
    - "blc_ll_customizeConnectionEstablishEvent" is used to customize Connection Establish Event.
    - "blc_ll_removeDeviceFromResolvingList" is used to remove a device from resolving list.
    - "blc_ll_readResolvingListSize" is used to read resolving list size.
    - "blc_ll_setResolvablePrivateAddressTimeout" is used to set the length of time the controller uses a Resolvable Private Address before a new resolvable private address is generated and starts being used.
    - "blc_ll_setPrivacyMode" is used to allow the Host to specify the privacy mode to be used for a given entry on the resolving list.
    - "blc_contr_setBluetoothVersion" is used to set Bluetooth core specification version in controller.

* **L2CAP**	
	- Add Peripheral L2CAP COC feature.
		- Add stack/ble/host/signaling/folder, and add signaling.h in this folder.
		- Add some APIs for this feature.
			- blc_l2cap_registerCocModule is used to register the COC Module.
			- blc_l2cap_disconnectCocChannel is used to disconnect with the COC Channel.
			- blc_l2cap_createLeCreditBasedConnect is used to create LE Credit Based connection.
			- blc_l2cap_createCreditBasedConnect is used to create Credit Based connection.
			- blc_l2cap_sendCocData is used to send COC Data.
		- Add some host events in gap_event.h 
			- GAP_EVT_L2CAP_COC_CONNECT is triggered by the COC connection.
			- GAP_EVT_L2CAP_COC_DISCONNECT is triggered by the COC disconnection.
			- GAP_EVT_L2CAP_COC_RECONFIGURE is triggered by the COC reconfigure.
			- GAP_EVT_L2CAP_COC_RECV_DATA is triggered when receiving COC Data.
			- GAP_EVT_L2CAP_COC_SEND_DATA_FINISH is triggered after COC Data is sent.
			- GAP_EVT_L2CAP_COC_CREATE_CONNECT_FINISH is triggered after the create COC channel command is sent.

  - Add API "blc_l2cap_reg_att_sig_handler", which is used to register the function to process L2CAP SIG CHANNEL packet.

  - Add API "blc_l2cap_SendCommandRejectResponse", which is used to send command reject response in central.

* **SMP**
	- Add Peripheral SC OOB.
		- Add some APIs for this feature.
			- blc_smp_generateScOobData is used to generate SC OOB data.
			- blc_smp_setScOobData is used to set SC OOB data.
		- Add some host events in gap_event.h.
			- GAP_EVT_MASK_SMP_TK_SEND_SC_OOB_DATA is triggered when the peer requests SC OOB.
			- GAP_EVT_MASK_SMP_TK_REQUEST_SC_OOB is triggered when SMP requests SC OOB data.
  
  - Add some APIs:
    - "blc_smp_setLocalIrkGenerateStrategy" is used to set local IRK generating strategy.
    - "blc_smp_triggerMasterManualSmp" is used to trigger central sending Pairing Request.
    - "blc_smp_sendKeypressNotify" is used to send keypress notify during TK input phrase.
    - "blc_smp_cancel_auth" is used to cancel SMP process.
    - "blc_smp_setDevExceedMaxStrategy" is used to configure the bonding strategy.

* **OTA**
  - Add some APIs:
    - "blc_ota_setOtaScheduleIndication_by_pduNum" is used to set resolution of OTA schedule indication by PDU number.
    - "blc_ota_setAttHandleOffset" is used to calculate OTA notify data ATT handle by OTA write data ATT handle.

* **GATT**
  - Add some APIs:
    - "blc_gap_mtuAutoExgDisable" is used to disable MTU exchange automatically.
    - "blc_host_checkHostInitialization" is used to check if any host initialization incorrect.

* **HCI**
  - Add API:
    - "blc_hci_register_user_handler" is used to register HCI user callback function.

* **Others**
	- Add mcu_config.h, including features supported by the MCU.

	- Add ext_calibration.c and ext_calibration.h to support ADC calibration, including flash two-point GPIO calibration, flash single-point GPIO calibration, flash single-point VBAT calibration (supported only by B87) and efuse single-point calibration; supports flash voltage calibration.

	- Add version information function.
		- Add API "blc_get_sdk_version" to get the SDK version.
		- Add the SDK version at the end of the bin file.
		- Add SDK version at the post-build.

### Bug Fixes
* N/A.

### Refactoring
* **Application**
	- add app_common.h/app_common.c to process some common initialization or settings for all application projects.
		- Move SYS_CLK_TYPE/CLOCK_SYS_CLOCK_1S/CLOCK_SYS_CLOCK_1MS/CLOCK_SYS_CLOCK_1US definition from app_config.h to app_common.h. Users now only define CLOCK_SYS_CLOCK_HZ in app_config.h. 
		- add API "blc_app_setDeepsleepRetentionSramSize" in app_common to automatically set deepsleep retention SRAM size for all application projects.
		- add API "blc_app_checkControllerHostInitialization" in app_common to check stack initialization error for the application project.
		- add API "blc_app_isIrkValid" in app_common to check if local or peer IRK is valid, which can be used in the whitelist and resolving list of relevant applications.

  - Battery Check is moved to common, and the application only handles the relevant UI parts.

	- Development board configuration
		- Delete redundant configuration information for LED and Keyboard in app_config.h of each project. Users can select the development board by BOARD_SELECT.
		- Add the folder vendor/common/boards and the following files that define each development board configuration:
			- boards_config.h, C1T139A3.h, C1T139A30.h, C1T197A30.h, C1T201A3.h, C1T357A20.h
  
* **Controller**
  - Add stack/ble/ble_controller/controller.h. Put some controller-relevant enumeration and API declaration in this file.

### BREAKING CHANGES
* **Boot & Link**	
	- Optimize and simplify the implementation of cstartup.S and boot.link.
		- Add cstartup_825x.S. This file is enough for all B85 projects. The SRAM size is automatically set by judging MCU_STARTUP. Previous 825x cstartup files are deleted as below:
			- cstartup_8253.S/cstartup_8253_RET_32K.S/cstartup_8258.S/cstartup_8258_RET_32K.S

		- Add cstartup_827x.S. This file is enough for all B87 projects. The SRAM size is automatically set by judging MCU_STARTUP. Previous 827x cstartup files are deleted as below:
			- cstartup_8238K.S/cstartup_8238_RET_32K.S/cstartup_8278.S/cstartup_8278_RET_32K.S

		- Modify boot.link. This file is enough for all B85/B87 projects. Previous link files are deleted as below:
			- boot_32k_retn.link
			
    - Configure "SRAM_SIZE" for different ICs in the project setting.
		- B85
			- Use new macro MCU_STARTUP_8253/MCU_STARTUP_8258 to configure IC, and MCU_STARTUP_8258 is used by default.
			    - Old macro below is not used anymore: 
				    - MCU_STARTUP_8258_RET_32K, 
				    - MCU_STARTUP_8253_RET_32K, 		
		- B87
			- Use new macro MCU_STARTUP_8238/MCU_STARTUP_8278 to configure IC, and MCU_STARTUP_8278 is used by default.
			    - Old macro below is not used anymore: 
				    - MCU_STARTUP_8278_RET_32K, 
				    - MCU_STARTUP_8238_RET_32K, 

		- Users do not need to be concerned about how to change configurations of different deepsleep retention SRAM size(16K or 32K). It's now processed automatically.
			- "blc_app_setDeepsleepRetentionSramSize" is used to configure deepsleep retention SRAM size automatically.
			- "\_retention_size_" is defined in cstarup.S file, and it is used to automatically calculate the total size of retention SRAM.
			- If "_retention_size" exceeds the maximum value(32K, TC321x support 64K bytes), the compiler will report the error "Error: Retention RAM size overflow." for users to notice this.
			- Add user_config.c in vendor/common, and add "__PM_DEEPSLEEP_RETENTION_ENABLE" in this file for retention SRAM size auto calculating and alarming in boot.link.
			- "__PM_DEEPSLEEP_RETENTION_ENABLE" equals to "PM_DEEPSLEEP_RETENTION_ENABLE" which is configurated by user in app_config.h.

* **Application**
	- BLE Flash information and Calibration
		- Rename blt_common.h/blt_common.c to ble_flash.h/ble_flash.c.
		- Split function "blc_app_loadCustomizedParameters" into two functions "blc_app_loadCustomizedParameters_normal"	and "blc_app_loadCustomizedParameters_deepRetn".
		- The first one is used when the MCU is powered on or wakes up from deepsleep mode. The second one is used when MCU wakes up from deepsleep retention mode.
		- Add API "blc_flash_read_mid_get_vendor_set_capacity" in "blc_readFlashSize_autoConfigCustomFlashSector" to get Flash mid, vendor and capacity information.

* **Driver**
  - Rename drivers/8258 to drivers/B85.
  - Rename drivers/8278 to drivers/B87.

* **BLE General**
	- Add stack/ble/host/host_comp.h, stack/ble/controller/contr_comp.h and stack/ble/ble_comp.h for some API/macro/data structure names in the old SDK compatible with the new SDK.
  - Remove l2cap_signal.h, and use signaling.h instead.

* **Controller**
  - Rename some files.
    - Rename acl_master.h to acl_central.h.
    - Rename acl_slave.h to acl_peripheral.h.
    - Rename resolvlist.h to resolvelist.h.

  - Remove some APIs:
    - blc_ll_resolvListInit
    - ll_resolvingList_delete
    - ll_resolvingList_getSize
    - ll_resolvingList_getPeerResolvableAddr
    - ll_resolvingList_getLocalResolvableAddr
    - ll_resolvingList_setResolvablePrivateAddrTimer
    - ll_resolvingList_setPrivcyMode

  - Move stack/ble/debug.h into stack/ble/debug/debug.h.

* **SMP**
  - Remove API "blc_smp_setBondingInfoIndexUpdateMethod".

* **GATT**
  - Change API "blc_gatt_pushAttHdlValueCfm" to "blc_gatt_pushConfirm"

* **Others**
  - Change the structure of the project. Split it into three independent projects.

### Flash
* **B85**
  - ZB25WD40B
  - ZB25WD80B
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E

* **B87**
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E
  - ZB25WD40B
  - ZB25WD80B

### CodeSize
* **B85**
  * acl_peripheral_demo
    - Firmware size: 81.4 kBytes
    - SRAM size: 31.4 kBytes
    - deepsleep retention SRAM size: 28.3 kBytes
  * acl_central_demo
    - Firmware size: 72.8 kBytes
    - SRAM size: 37.0 kBytes
  * acl_connection_demo
    - Firmware size: 87.3 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 93.4 kBytes
    - SRAM size: 35.0 kBytes
    - deepsleep retention SRAM size: 31.9 kBytes

* **B87**
  * acl_peripheral_demo
    - Firmware size: 79.4 kBytes
    - SRAM size: 31.7 kBytes
    - deepsleep retention SRAM size: 28.1 kBytes
  * acl_central_demo
    - Firmware size: 71.5 kBytes
    - SRAM size: 37.2 kBytes
  * acl_connection_demo
    - Firmware size: 86.9 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 91.3 kBytes
    - SRAM size: 35.8 kBytes
    - deepsleep retention SRAM size: 31.7 kBytes

* **TC321X**
  * acl_peripheral_demo
    - Firmware size: 79.3 kBytes
    - SRAM size: 31.5 kBytes
    - deepsleep retention SRAM size: 28.2 kBytes
  * acl_central_demo
    - Firmware size: 71.5 kBytes
    - SRAM size: 37.2 kBytes
  * acl_connection_demo
    - Firmware size: 88.4 kBytes
    - SRAM size: 43.1 kBytes
  * acl_c1p1_demo
    - Firmware size: 93.6 kBytes
    - SRAM size: 35.9 kBytes
    - deepsleep retention SRAM size: 31.9 kBytes


### Version
* SDK 版本: tc_ble_sdk V4.0.2.0
* Chip 版本
  - B85: TLSR825X
  - B87: TLSR827X
  - TC321X (A0)
* Hardware 版本
  - B85: C1T139A30_V1_2, C1T139A3_V2_0
  - B87: C1T197A30_V1_1, C1T201A3_V1_0
  - TC321X: C1T357A20_V1_1
* Platform 版本
  - tc_platform_sdk V3.1.0
* Toolchain 版本
  - TC32 ELF GCC4.3 ( IDE: [Telink IDE](https://wiki.telink-semi.cn/wiki/IDE-and-Tools/IDE-for-TLSR8-Chips/)/ [TelinkIoTStudio_V2024.8](https://wiki.telink-semi.cn/tools_and_sdk/Tools/IoTStudio/TelinkIoTStudio_V2024.8.zip) )

### Note
* telink_b85m_ble_sdk V4.0.1.3_Patch0003之前的所有修改均已包含。

* TC321x尚未进行完整测试，后续将更新迭代版本。

* B85m_controller在当前版本中移除，将在下个版本中重新添加。

* 电池电压检测是量产的重要功能。
	- 用户必须检查电池电压，以防止在低电压下Flash写入或擦除异常。
	
* Flash保护是量产的必要功能。
	- SDK中默认开启Flash保护。用户必须在最终的量产应用中启用此功能。
	- 在开发和调试阶段，用户可以使用Telink BDT工具中的“Unlock”命令进行Flash解锁。
	- SDK中的Flash保护是基于示例代码的参考设计。考虑到用户的最终应用可能与示例代码不同，如用户的最终固件尺寸更大，或者用户有不同的OTA设计，或者用户需要在Flash的其他区域存储更多的数据，这些差异都意味着SDK中的Flash保护参考设计不可能在不做任何更改的情况下直接用于用户的量产应用。用户应参考示例代码，了解原理和方法，然后根据自己的应用更改和实现更适合的机制。

* 纠正一些拼写错误
	- 如修改OTA_DATA_UNCOMPLETE为OTA_DATA_INCOMPLETE。

### Features
* **Chip**
  - 支持TC321X A0；
      - 支持1M phy和2M phy。
      - 支持PM，包括suspend mode，deepsleep retention mode和deepsleep mode.
      - 支持Flash保护。
      - 支持低电检测。
      - 支持以下Demo：
          - acl_connection_demo
          - feature_test
          - acl_c1p1_demo
          - acl_peripheral_demo
          - acl_central_demo

* **Application**
	- 新增Flash Protection模块和示例
		- 在vendor/common文件夹中添加flash_prot.h/flash_prot.c作为所有应用程序的通用模块。
		- 在acl_connection_demo/acl_c1p1_demo/acl_central_demo/acl_peripheral_demo中增加Flash保护示例代码，用户可以参考示例代码，并必须确保Flash保护功能在应用程序上启用。
		- “app_flash_protection_operation”是所有Flash操作(Flash写、擦除)的通用入口，Flash加锁和解锁都在这个函数中进行。
		- “blc_appRegisterStackFlashOperationCallback”和“flash_prot_op_cb”用于处理一些协议栈内部Flash操作的回调，这些回调可能需要Flash加锁和解锁。
			- 协议栈OTA模块已经正确增加blc_flashProt相关回调，告知应用层SMP需要write/erase Flash的操作。应用层需要对应处理Flash lock/unlock。
			- 如果客户不使用Telink OTA，需参考此设计在OTA代码中添加类似的回调函数供上层使用。
		- 现在只有固件在示例代码中受到保护，系统数据和用户数据现在不受保护。
		- 驱动API “flash_write_status”中中断禁用时间过长，BLE系统中断会延迟，导致接收数据错误。将驱动API “flash_write_status”替换为flash_prot.c中的“flash_write_status”，通过调用LinkLayer API “blc_ll_write_flash_status”来解决此问题。

	- 新增tlkapi_debug模块打印日志
		- 在vendor/common文件夹中新增tlkapi_debug.h与tlkapi_debug.c，作为所有应用程序的通用模块。
		- 在stack/ble/debug文件夹中新增debug.h，新增API“blc_debug_enableStackLog”，“blc_debug_addStackLog”，blc_debug_removeStackLog”控制协议栈log打印。
		- 使用GPIO模拟UART TX时序将打印信息输出，功能与旧版SDK中的“printf”功能相同。
		- 建议使用API "tlkapi_printf"和"tlkapi_send_string_data"，并在app_config.h中使用一些宏来控制日志启用，例如"APP_LOG_EN"。
		- 只支持1M波特率，系统时钟为16M/24M时通过汇编指令控制UART TX时序，系统时钟为32M/48M时通过system_timer控制UART TX时序。IRQ禁用和恢复在函数“uart_put”中使用，以保证UART TX字节数据的完整性不被IRQ破坏。如果波特率过低，IRQ禁用时间过长，会影响BLE系统中断或者用户中断，可能导致严重错误。

* **Feature Test**
	- 新增feature_privacy_peripheral：测试peripheral RPA解析的示例代码。如果在CONNECT_IND包中使用RPA(Resolved Private Address)，该示例代码可通过对端设备的IDA解析。
	- 新增feature_privacy_central：测试Central RPA解析的示例代码。
  - 新增feature_l2cap_coc：Peripheral端L2CAP COC的示例代码。
	- 新增feature_smp_security：SC OOB的示例代码。
	- 新增feature_phy_test：PHY功能的测试代码。
	- 移除feature_misc。

* **Controller**
  - 新增API：
    - “blc_ll_setAdvCustomizedChannel”用于在ADV状态下设置其他channel来代替主channel 37/38/39。
    - “blc_ll_configLegacyAdvEnableStrategy”用于配置legacy ADV仅通过API启用。
    - “blc_ll_setScanCustomizedChannel”用于在SCAN状态下设置其他channel来代替主channel 37/38/39。
    - “blc_ll_configScanEnableStrategy”用于配置scan通过API启用。
    - “blc_ll_isInitiationBusy”用于检查是否初始化处于busy状态。
    - “blc_ll_setCreateConnectionRetryNumber”用于设置创建连接重试次数。
    - “blc_ll_disableConnUpdHighAuthority”用于禁用连接参数更新central的高级权限。
    - “blc_ll_getAclLatestRSSI”用于获取ACL连接的最新RSSI。
    - “blc_ll_getAclConnectionInterval”用于读取当前ACL连接的时间间隔。
    - “blc_ll_getAclConnectionLatency”用于读取当前ACL连接的latency。
    - “blc_ll_getAclConnectionTimeout”用于读取当前ACL连接的超时时间。
    - “blc_ll_dataLenAutoExgDisable”用于禁用DLE的自动交换。
    - “blc_ll_customizeConnectionEstablishEvent”用于自定义连接建立事件。
    - “blc_ll_removeDeviceFromResolvingList”用于从解析列表中删除设备。
    - “blc_ll_readResolvingListSize”用于读取解析列表大小。
    - “blc_ll_setResolvablePrivateAddressTimeout”用于设置controller使用“可解析私有地址”的时间，系统会生成新的可解析私有地址并开始使用。
    - “blc_ll_setPrivacyMode”用于允许Host指定要用于解析列表中给定条目的隐私模式。
    - “blc_contr_setBluetoothVersion”用于在controller中设置蓝牙版本。

* **L2CAP**	
	- 支持Peripheral L2CAP COC
	- 新增文件夹stack/ble/host/signaling，并在该文件夹中新增signaling.h.
		- 新增如下API：
			- blc_l2cap_registerCocModule用于注册COC模块。
			- blc_l2cap_disconnectCocChannel用于对COC通道断开连接。
			- blc_l2cap_createLeCreditBasedConnect用于建立基于LE credit的连接。
			- blc_l2cap_createCreditBasedConnect用于建立基于credit的连接。
			- blc_l2cap_sendCocData用于发送COC数据。
		- gap_event.h中新增以下host event：
			- GAP_EVT_L2CAP_COC_CONNECT 在COC连接时触发。
			- GAP_EVT_L2CAP_COC_DISCONNECT 在COC断连时触发。
			- GAP_EVT_L2CAP_COC_RECONFIGURE 在COC连接通道参数更新时触发。
			- GAP_EVT_L2CAP_COC_RECV_DATA 在COC通道接收到新数据时触发。
			- GAP_EVT_L2CAP_COC_SEND_DATA_FINISH 在COC通道数据发送完成时触发。
			- GAP_EVT_L2CAP_COC_CREATE_CONNECT_FINISH 在创建COC通道指令发送成功时触发。
  
  - 添加API “blc_l2cap_reg_att_sig_handler”用于注册处理L2CAP SIG CHANNEL报文。

  - 添加API “blc_l2cap_SendCommandRejectResponse”用于在central中发送命令拒绝响应。

* **SMP**
	- 支持Peripheral SC OOB
		- 新增如下API：
			- blc_smp_generateScOobData用于生成SC OOB数据
			- blc_smp_setScOobData用于设置SC OOB数据
		- 新增如下Host event及对应参数结构体：
			- GAP_EVT_MASK_SMP_TK_SEND_SC_OOB_DATA 在对端请求SC OOB时触发。
			- GAP_EVT_MASK_SMP_TK_REQUEST_SC_OOB 在SMP请求SC OOB数据时触发。

  - 新增如下API：
    - “blc_smp_setLocalIrkGenerateStrategy”用于设置本地IRK生成策略。
    - “blc_smp_triggerMasterManualSmp”用于触发central发送配对请求。
    - “blc_smp_sendKeypressNotify”用于在TK输入期间发送按键通知。
    - “blc_smp_cancel_auth”用于取消SMP进程。
    - “blc_smp_setDevExceedMaxStrategy”用于配置bonding策略。

* **OTA**
  - 新增如下API：
    - “blc_ota_setOtaScheduleIndication_by_pduNum”用于按PDU编号设置OTA调度指示的分辨率。
    - “blc_ota_setAttHandleOffset”通过OTA写入数据ATT句柄计算OTA通知数据ATT句柄。

* **GATT**
  - 新增如下API：
    - “blc_gap_mtuAutoExgDisable”用于自动关闭MTU交换功能。
    - “blc_host_checkHostInitialization”用于检查是否有host初始化错误。

* **HCI**
  - 新增如下API：
    - “blc_hci_register_user_handler”用于注册HCI用户回调功能。

* **Others**
	- 新增mcu_config.h，包含MCU对应的功能支持。

	- 新增ext_calibration.c和ext_calibration.h，支持ADC校准，包括flash两点GPIO校准，flash单点GPIO校准，flash单点VBAT校准（只有B87支持）和efuse单点校准；支持flash电压值校准。

	- 新增版本信息功能
		- 增加API “blc_get_sdk_version”获取SDK版本。
		- 在bin文件的末尾添加SDK版本，在common文件夹下新增sdk_version.c和sdk_version.h。
		- 在编译后显示SDK版本。

### Bug Fixes
* N/A.

### Refactoring
* **Application**
	- 添加app_common.h/app_common.c，用于处理所有应用项目的一些通用功能。
		- 将SYS_CLK_TYPE/CLOCK_SYS_CLOCK_1S/CLOCK_SYS_CLOCK_1MS/CLOCK_SYS_CLOCK_1US定义从app_config.h移至app_common.h。用户只需在app_config.h中定义CLOCK_SYS_CLOCK_HZ。
		- 在app_common中增加API “blc_app_setDeepsleepRetentionSramSize”，自动设置所有应用项目的Deepsleep Retention保留SRAM大小。
		- 在app_common中增加API "blc_app_checkControllerHostInitialization"，用于检查应用项目的协议栈初始化错误。
		- 在app_common中增加API "blc_app_isIrkValid"来检查本地或对端IRK是否有效，可用于白名单和地址解析相关的应用。

  - Battery Check移动到common下，应用只处理UI相关的部分。 

	- 开发板选择
		- 删除每个工程app_config.h里的LED、Keyboard的冗余配置信息，用户可以通过定义BOARD_SELECT来选择开发板
		- 新增vendor/common/boards文件夹和以下定义各个硬件配置的文件：
			- boards_config.h、C1T139A3.h、C1T139A30.h、C1T197A30.h、C1T201A3.h、C1T357A20.h

* **Controller**
  - 新增stack/ble/ble_controller/controller.h。在这个文件中放入一些Controller相关的枚举和API声明。

### BREAKING CHANGES
* **Boot & Link**	
	- 优化cstartup. S和boot.link的实现。
	- 添加cstartup_825x.S，这个文件适用于所有B85工程，SRAM大小通过判断MCU_STARTUP自动设置。以下旧版cstartup文件移除：
		- cstartup_8253.S/cstartup_8253_RET_32K.S/cstartup_8258.S/cstartup_8258_RET_32K.S
	- 添加cstartup_827x.S，这个文件适用于所有B87工程，SRAM大小通过判断MCU_STARTUP自动设置。以下旧版cstartup文件移除：
		- cstartup_8238K.S/cstartup_8238_RET_32K.S/cstartup_8278.S/cstartup_8278_RET_32K.S
	- 添加boot.link，这个文件适用于所有B85/B87工程。以下旧版link文件移除：
		- boot_32k_retn.link

	- 根据不同芯片，在project setting中配置"SRAM_SIZE"
		- B85
			- 使用新的宏 MCU_STARTUP_8251/MCU_STARTUP_8253/MCU_STARTUP_8258 来配置芯片, 默认配置为 MCU_STARTUP_8258
			- 以下这些旧的宏不再使用: 
				- MCU_STARTUP_8258_RET_32K, 
				- MCU_STARTUP_8253_RET_32K
		- B87
			- 使用新的宏 MCU_STARTUP_8271/MCU_STARTUP_8238/MCU_STARTUP_8278 来配置芯片, 默认配置为 MCU_STARTUP_8278
			- 以下这些旧的宏不再使用: 
				- MCU_STARTUP_8278_RET_32K, 
				- MCU_STARTUP_8238_RET_32K, 
	- 自动配置deepsleep retention SRAM大小(16K或32K)，用户无需关注。
		- "blc_app_setDeepsleepRetentionSramSize"用来自动配置deepsleep retention SRAM大小。
		- "\_retention_size_"在cstarup.S中定义，用于自动计算retention SRAM大小。
		- 如果“_retention_size_”超过最大值32K字节（TC321x为64K字节），编译器将报告错误"Error: Retention RAM size overflow."，以便用户注意到这一点。
		- 在vendor/common中添加user_config.c，在此文件中添加宏"__PM_DEEPSLEEP_RETENTION_ENABLE"，用于在boot.link中自动计算和报警retention SRAM size。
		- "__PM_DEEPSLEEP_RETENTION_ENABLE"等于用户在app_config.h中配置的"PM_DEEPSLEEP_RETENTTION_ENABLE"。

* **Application**
	- 调整BLE Flash配置和校准
		- 将blt_common.h/blt_common.c重命名为ble_flash.h/ble_flash.c。
		- 将函数"blc_app_loadCustomizedParameters"拆分为两个函数"blc_app_loadCustomizedParameters_normal"和"blc_app_loadCustomizedParameters_deepRetn"。第一个是在MCU上电或从deepsleep模式唤醒时使用，第二个是当MCU从deepsleep retention模式唤醒时使用的。
		- 添加API “blc_flash_read_mid_get_vendor_set_capacity”以获取Flash mid、供应商和容量信息。

* **Driver**
  - 将drivers/8258重命名为drivers/B85。
  - 将drivers/8278重命名为drivers/B87。

* **BLE General**
	- 新增stack/ble/host/host_comp.h、stack/ble/controller/contr_comp.h和stack/ble/ble_comp.h，用于新版本SDK兼容旧版本SDK中一些API/宏/结构体。
  - 移除l2cap_signal.h，使用signaling.h。

* **Controller**
  - 重命名文件夹
    - 将acl_master.h重命名为acl_central.h。
    - 将acl_slave.h重命名为acl_peripheral.h。
    - 将resolvlist.h重命名为resolvelist.h。

  - 移除以下API：
    - blc_ll_resolvListInit
    - ll_resolvingList_delete
    - ll_resolvingList_getSize
    - ll_resolvingList_getPeerResolvableAddr
    - ll_resolvingList_getLocalResolvableAddr
    - ll_resolvingList_setResolvablePrivateAddrTimer
    - ll_resolvingList_setPrivcyMode

* **SMP**
  - 移除API “blc_smp_setBondingInfoIndexUpdateMethod”。

* **GATT**
  - 修改API “blc_gatt_pushAttHdlValueCfm”为“blc_gatt_pushConfirm”。

* **Others** 
  - 修改工程结构，将其分成三个独立的工程。

### Flash
* **B85**
  - ZB25WD40B
  - ZB25WD80B
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E

* **B87**
  - GD25LD40C/GD25LD40E
  - GD25LD80C/GD25LD80E
  - ZB25WD40B
  - ZB25WD80B

### CodeSize
* **B85**
  * acl_peripheral_demo
    - Firmware size: 81.4 kBytes
    - SRAM size: 31.4 kBytes
    - deepsleep retention SRAM size: 28.3 kBytes
  * acl_central_demo
    - Firmware size: 72.8 kBytes
    - SRAM size: 37.0 kBytes
  * acl_connection_demo
    - Firmware size: 87.3 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 93.4 kBytes
    - SRAM size: 35.0 kBytes
    - deepsleep retention SRAM size: 31.9 kBytes

* **B87**
  * acl_peripheral_demo
    - Firmware size: 79.4 kBytes
    - SRAM size: 31.7 kBytes
    - deepsleep retention SRAM size: 28.1 kBytes
  * acl_central_demo
    - Firmware size: 71.5 kBytes
    - SRAM size: 37.2 kBytes
  * acl_connection_demo
    - Firmware size: 86.9 kBytes
    - SRAM size: 43.0 kBytes
  * acl_c1p1_demo
    - Firmware size: 91.3 kBytes
    - SRAM size: 35.8 kBytes
    - deepsleep retention SRAM size: 31.7 kBytes

* **TC321X**
  * acl_peripheral_demo
    - Firmware size: 79.3 kBytes
    - SRAM size: 31.5 kBytes
    - deepsleep retention SRAM size: 28.2 kBytes
  * acl_central_demo
    - Firmware size: 71.5 kBytes
    - SRAM size: 37.2 kBytes
  * acl_connection_demo
    - Firmware size: 88.4 kBytes
    - SRAM size: 43.1 kBytes
  * acl_c1p1_demo
    - Firmware size: 93.6 kBytes
    - SRAM size: 35.9 kBytes
    - deepsleep retention SRAM size: 31.9 kBytes



## V4.0.1.3


### Features
* SDK version：telink_b85m_ble_sdk_V4.0.1.3
* Normalizes the naming of content related to private schemas.


### BREAKING CHANGES
* N/A.


### Features
* SDK 版本：telink_b85m_ble_sdk_V4.0.1.3
* 规范化与私有模式相关内容的命名

### BREAKING CHANGES
* N/A.



## V4.0.1.2_Patch_0003

### Features
* Support 8238.

### Bug Fixes
* Fixed disconnection when there is long user task in main loop.
* Fixed connection failure when jumping over first packet after master's Connect Ind.
* Fixed "MIC Fail" disconnection due to AES reentrancy.
* Release blc_ll_register_user_irq_handler_cb() API to solve the data loss problem of hardware interfaces (eg. UART) in Adv status.
* Fixed that Slave will still send the Security Request, thought SMP is disabled. 

### BREAKING CHANGES
  - N/A

 ### Features
* 支持8238。


### Bug Fixes
* 修复mainloop用户任务长时间阻塞，导致连接断开。
* 修复Slave在收到Connect Ind之后没有收到第一个包的情况下产生断连。
* 修复AES重入导致的 MIC Fail 断连。
* 释放blc_ll_register_user_irq_handler_cb api接口，解决在广播态下存在的硬件接口（如UART等）数据丢失问题。
* 修复SMP关闭的情况下，Slave仍然会发送Security Request。

### BREAKING CHANGES
  - N/A



## V4.0.1.2_Patch_0002

### Features
* Add API blc_ll_checkBleTaskIsIdle() to get BLE task idle status.
* Optimize Adv validation timing after disabled and enabled Adv.
* Add API blc_ll_continue_adv_after_scan_req() to set continuing advertising on other channels after receiving scan request.
* Add API blc_ll_register_user_irq_handler_cb() to solve the problem of hardware interface (eg. UART) data miss under Adv status.


### Bug Fixes
* Fix abnormal phenomenon when there is no stack task in some cases.
* Fix the problem of disconnection after receiving Connection Indication/Connection Update in some cases.


### BREAKING CHANGES
  - N/A


 ### Features
* 增加blc_ll_checkBleTaskIsIdle api接口，获取底层蓝牙任务是否处于Idle状态。
* 优化enable或disable广播生效时间。
* 增加blc_ll_continue_adv_after_scan_req api接口，用于设置在收到Scan Req之后，继续在其他通道进行广播。
* 增加blc_ll_register_user_irq_handler_cb api接口，解决在广播态下存在的硬件接口（如UART等）数据丢失问题。


### Bug Fixes
* 修复修复协议栈没有任务时程序可能运行异常的Bug。
* 修复在create Connection /connection parameter update时候可能出现收不到包的现象，导致断连问题。


### BREAKING CHANGES
  - N/A



## V4.0.1.2_Patch_0001

### Bug Fixes
* When MTU>23, data crosstalk happens occassionally if several connections' L2CAP packets consist of start fragment and continuation fragments. A connection's continuation fragment may reassemble to another connection's start fragment.

### BREAKING CHANGES
  - N/A


### Bug Fixes
* 当MTU>23时，l2cap packet如果由start fragment和continuation fragment组成，底层拼包有概率出现continuation fragment和不属于自己的start fragment组成一个l2cap包的情况，即产生串扰。

### BREAKING CHANGES
  - N/A



## V4.0.1.2

### Notes
* Customers must confirm flash size in app_config.h

### Features
* SDK version：telink_b85m_ble_sdk_V4.0.1.2
* This version sdk support 8253/8258/8273/8278 chip.
* Optimize the process of channel map/phy/connection parameter update.
* Clean application code to make user feel better to use, such as using MACRO to calulate related buffer size automatically.
* Add some APIs according to users' feedback.
  +  Long sleep API.
  +  RSSI get API.
  +  OTA max timeout prolonged to 1000s from 250s.

### Bug Fixes
  +  Connection parameter update abnormally under PM.
  +  Inaccurate T_IFS in CONNECT_IND, SCAN_RSP, AUX_CONNECT_REQ.
  +  Long run time of software timer callback affects timing logic.

### BREAKING CHANGES
  - N/A



### Notes
* 用户必须要在app_config.h中确认flash size与当前所用芯片相符。

### Features
* SDK 版本：telink_b85m_ble_sdk_V4.0.1.2
* 支持8253/8258/8273/8278。
* 优化Channel map/PHY/Connection parameter更新流程。
* 清理优化应用层代码，提高用户友好性，比如用宏自动计算相关的buffer大小。
* 根据用户反馈，增加了如下API。
  +  长休眠。
  +  获取RSSI。
  +  OTA超时设置上限从250s调整到1000s。

### Bug Fixes
  +  连接参数在PM下更新异常。
  +  CONNECT_IND, SCAN_RSP, AUX_CONNECT_REQ的T_IFS时间不准。
  +  软件定时器回调运行太长影响时序逻辑错误。

### BREAKING CHANGES
  - N/A






## V4.0.1.1-Beta

### Features
* SDK version：telink_b85m_ble_sdk_V4.0.1.1-Beta
* This version sdk support 8253/8258/8273/8278 chip.
* Release relevant m4s4.a/m1s1.a/m0s4.a that cost different ram size.
* Support maximum 4 master and 4 slave connections simultaneous.
* Support low power management(b85m_slave and b85m_m1s1 support deepsleep retention mode).
* Support Extended Advertising/Extended Scan/Periodic Advertising/Periodic Scan.
* Support core version 5.0.
* Demo illustration.
  +  4 master and 4 slave demo: b85m_demo
  +  1 master and 1 slave demo: b85m_m1s1
  +  master demo: b85m_master_dongle
  +  slave demo: b85m_slave
  +  controller demo: b85m_controller
  +  feature demo: b85m_feature

### BREAKING CHANGES
  - N/A



### Features
* SDK 版本：telink_b85m_ble_sdk_V4.0.1.1-Beta
* 支持8253/8258/8273/8278。
* 提供m4s4.a/m1s1.a/m0s4.a，不同的库消耗的ram大小不同。
* 支持最多4个master和4个slave同时连接。
* 支持低功耗管理（b85m_slave和b85m_m1s1支持deepsleep retention mode）。
* 支持扩展广播、扩展扫描、周期性广播及扫描。
* 支持core 5.0版本。
* Demo 描述.
  +  4 master and 4 slave demo: b85m_demo
  +  1 master and 1 slave demo: b85m_m1s1
  +  master demo: b85m_master_dongle
  +  slave demo: b85m_slave
  +  controller demo: b85m_controller
  +  feature demo: b85m_feature

### BREAKING CHANGES
  - N/A






## V4.0.0

### Features
* SDK version: telink_b85m_ble_sdk_alpha_v4.0.0.
* This version sdk support 825x/827x chip.
* Support connect slave(maximum 4) and master(maximum 4) simultaneously without low power manage.
* Support 1M/2M/Coded PHY.
* Support Extended Advertising.
* Support CSA#2.
* HCI(controller) Support H4 and H5(3-wire) two transmission protocols. 
* Demo illustration.
  +  master and slave demo:b85m_demo 
  +  controller demo:b85m_controller
  +  feature demo：b85m_feature
  +  only master demo：b85m_master_dongle 

### BREAKING CHANGES
* N/A



### Features
* SDK 版本：telink_b85m_ble_sdk_alpha_v4.0.0。
* 该版本支持825x、827x两种芯片。
* 同时支持4个master和4个slave连接。
* 支持1M/2M/Coded PHY。
* 支持扩展广播。
* 支持跳频算法2。
* controller支持H4/H5（3-wire）两种传输协议。
* Demo illustration.
  +  master and slave demo: b85m_demo 
  +  controller demo: b85m_controller
  +  feature demo：b85m_feature
  +  only master demo：b85m_master_dongle 

### BREAKING CHANGES
* N/A
