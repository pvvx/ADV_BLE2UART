#include "tl_common.h"
#include "app_buffer.h"
#include "drivers.h"
#include "app_config.h"
#include "drivers/8258/gpio_8258.h"
#include "ble.h"
#include "app.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "scaning.h"
#include "drv_uart.h"
#include "crc.h"

RAM u8 mac_public[6];
// u8 mac_random_static[6];

__attribute__((optimize("-Os")))
int app_controller_event_callback(u32 h, u8 *p, int n) {
	if (h & HCI_FLAG_EVENT_BT_STD) { // ble controller hci event
		u8 evtCode = h & 0xff;
		if(evtCode == HCI_EVT_LE_META) { //LE Event
			// (p[0] == HCI_SUB_EVT_LE_DIRECT_ADVERTISE_REPORT) ???
			if (p[0] == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT)
				ble_ext_adv_callback(p);
			else if (p[0] == HCI_SUB_EVT_LE_ADVERTISING_REPORT)
				ble_adv_callback(p);
			else if (p[0] == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT)
				ble_le_periodic_adv_callback(p);
			else if (p[0] == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED)
				ble_le_periodic_adv_sync_established_callback(p);
			else if (p[0] == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST)
				ble_le_periodic_adv_sync_lost_callback(p);
#if 0	// debug
			else {
				//send_debug(p, 10);
				extern my_fifo_t ad_fifo;
				u8 *s = my_fifo_wptr(&ad_fifo);
				if(s) {
					s[0] = 10;
					s[1] = h;
					s[2] = 0;
					s[3] = 0xff;
					memset(&s[4], 0, 6);
					memcpy(&s[10], p, 10);
					my_fifo_next(&ad_fifo);
				}
			}
		}
	} else {
		//send_debug(p, 10);
		extern my_fifo_t ad_fifo;
		u8 *s = my_fifo_wptr(&ad_fifo);
		if(s) {
			s[0] = 10;
			s[1] = h;
			s[2] = 0;
			s[3] = 0xff;
			memset(&s[4], 0, 6);
			memcpy(&s[10], p, 10);
			my_fifo_next(&ad_fifo);
#endif
		}
	}
	return 0;
}

__attribute__((optimize("-Os")))
void init_ble(void) {
	////////////////// BLE stack initialization Begin //////////////////////
#if 1
	u8 mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);
#else
	generateRandomNum(5, mac_public);
	mac_public[5] = 0xC0; 			// for random static
#endif

	blc_ll_initBasicMCU(); //must

	init_uart(921600);
	crcInit();

	blc_ll_initStandby_module(mac_public); //must

	blc_ll_initExtendedScanning_module();	//extended scan module
    blc_ll_initExtendedInitiating_module();

	blc_ll_initPeriodicAdvertisingSynchronization_module();
	blc_ll_initAclConnection_module();

#if BLE_MASTER_ENABLE
	blc_ll_setMaxConnectionNumber(MASTER_MAX_NUM, SLAVE_MAX_NUM);
	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS,
			ACL_MASTER_MAX_TX_OCTETS, ACL_SLAVE_MAX_TX_OCTETS);
	/* all ACL connection share same RX FIFO */
	blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
	/* ACL Master TX FIFO */
	blc_ll_initAclConnMasterTxFifo(app_acl_mstTxfifo, ACL_MASTER_TX_FIFO_SIZE,
			ACL_MASTER_TX_FIFO_NUM, MASTER_MAX_NUM);
	blc_ll_setAclMasterConnectionInterval(CONN_INTERVAL_31P25MS);
#endif // BLE_MASTER_ENABLE
	blc_hci_registerControllerEventHandler(app_controller_event_callback);
#if 0
	blc_hci_setEventMask_cmd(0xffffffff);
	blc_hci_le_setEventMask_cmd(0xffffffff);
	blc_hci_le_setEventMask_2_cmd(0xffffffff);
#else
	//bluetooth event
	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd( HCI_LE_EVT_MASK_ADVERTISING_REPORT
			| HCI_LE_EVT_MASK_DIRECT_ADVERTISING_REPORT
			| HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_ESTABLISHED
			| HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT
			| HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_LOST
			| HCI_LE_EVT_MASK_SCAN_REQUEST_RECEIVED
			| HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT);
#endif

	blc_controller_check_appBufferInitialization();

#if BLE_MASTER_ENABLE
	blc_hci_registerControllerDataHandler (blc_l2cap_pktHandler);
	blc_l2cap_initAclConnMasterMtuBuffer(mtu_m_rx_fifo, MTU_M_BUFF_SIZE_MAX, 0, 0);
	blc_att_setMasterRxMTUSize(ATT_MTU_MASTER_RX_MAX_SIZE);
#endif // BLE_MASTER_ENABLE

	rf_set_power_level_index(MY_RF_POWER);
	//start_adv_scanning(3, SCAN_INTERVAL_30MS);
}

// Scannning_Interval, Time = N * 0.625 ms
void start_adv_scanning(u8 flg, u16 tdw) {
#if defined(GPIO_LED_R)
	gpio_write(GPIO_LED_R, 0);
#endif
#if defined(GPIO_LED_G)
	gpio_write(GPIO_LED_G, 0);
#endif
#if defined(GPIO_LED_B)
	gpio_write(GPIO_LED_B, 0);
#endif
	if(flg & 3) {
		if(tdw < SCAN_INTERVAL_10MS)
			tdw = SCAN_INTERVAL_10MS;
		u32 t1 = 0, t2 = 0;
		if(flg & 1)
			t1 = tdw;
		if(flg & 2)
			t2 = tdw;
		u32 mode = (flg & 1) | ((flg & 2) << 1);
		mac_list.filtr = (flg >> 4) & 3;
		//set scan parameter and scan enable
		//blc_ll_setExtScanParam( OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M_CODED,
		if (blc_ll_setExtScanParam(
            (flg >> 6) & 3, // ownAddrType - Own_Address_Type
            SCAN_FP_ALLOW_ADV_ANY, // scan_fp - Scanning_Filter_Policy
            mode, // scan_phys - Scanning_PHYs, "SCAN_PHY_1M" or "SCAN_PHY_CODED"
            (flg >> 2) & 1, // Scan_Type for 1M PHY, Passive Scanning or Active Scanning.
            t1, t1, // Scan_Interval and Duration of the scan on the primary advertising physical channel for 1M PHY
            (flg >> 2) & 1, // Scan_Type for Coded PHY, Passive Scanning or Active Scanning.
            t2, t2 // Scan_Interval and Duration of the scan on the on the primary advertising physical channel for Coded PHY
        )) {
#if defined(GPIO_LED_R)
            gpio_write(GPIO_LED_R, 1);  // Show error...
            sleep_us(5000000); // ...for 5 seconds
#endif
        }
		if (blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, (flg >> 3) & 1,
				SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS)
        ) {
#if defined(GPIO_LED_R)
            gpio_write(GPIO_LED_R, 1);  // Show error...
            sleep_us(2000000); // ...for 2 seconds
#endif
        };
	} else {
		if (blc_ll_setExtScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE,
				SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS)
        ) {
#if defined(GPIO_LED_R)
            gpio_write(GPIO_LED_R, 1);  // Show error...
            sleep_us(2000000); // ...for 2 seconds
#endif
        };
	}
}
