#include "app_config.h"
#include "tl_common.h"
#include "app_buffer.h"
#include "drivers.h"
#include "drivers/B85/gpio.h"
#include "ble.h"
#include "app.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "scanning.h"
#include "drv_uart.h"
#include "crc.h"
#include "tinyFlash.h"
#if DEBUG_MSG > 0
#include "debug_ble_functions.h"
#endif

RAM u8 mac_public[6];

// Connection state
RAM u8  acl_conn_state;    // 0=idle 1=connecting 2=connected
RAM u16 acl_conn_handle;   // valid when state==2
RAM u8  acl_conn_interval; // conn interval x 1.25ms
RAM u8  acl_peer_addr_type;
RAM u8  acl_peer_addr[6];  // last connected peer MAC
// u8 mac_random_static[6];

unsigned int baudrate_list[] = { 2000000, 921600, 115200 };  // List of available UART baudrates in bit-per-second

static u16 coded_min_scan_window = SCAN_INTERVAL_100MS;
static u8 primary_scan_channels[3] = {37, 38, 39};
static u8 runtime_rf_power = MY_RF_POWER;
static u8 runtime_rf_cap = 0xff;

// Extended advertising and ACL central role modules are NOT initialized:
// each pulls ~4-8 KB of hidden library BSS from liblt_8258.a, leaving
// insufficient stack space (<1 KB). CMD_ID_TXADV and CMD_ID_CONN connect
// return CMD_STATUS_DENIED in this build.

// Define the Index list of persistent data used by tinyFlash
enum
{
    STORAGE_BAUD = 1,
};

void rgb_blink(int sleep) {
#if defined(GPIO_LED_R) && defined(GPIO_LED_G) && defined(GPIO_LED_G)
    sleep_us(sleep);
    gpio_write(GPIO_LED_R, 1);
    gpio_write(GPIO_LED_G, 1);
    gpio_write(GPIO_LED_B, 1);
    sleep_us(sleep);
    gpio_write(GPIO_LED_R, 0);
    gpio_write(GPIO_LED_G, 0);
    gpio_write(GPIO_LED_B, 0);
#endif
}

u8 read_baud_rate(void) {
    u8 baudrate_index;
    u8 baud_buf[1];
    u8 size_baud_list = sizeof(baudrate_list) / sizeof(baudrate_list[0]);
    u8 len = 1;

    if ( tinyFlash_Read(STORAGE_BAUD, baud_buf, &len) )
        baudrate_index = 0;
    else
        baudrate_index = baud_buf[0];
    if (baudrate_index >= size_baud_list) {
        rgb_blink(50000);
        return 0;
    }
    return baudrate_index;
}

void change_baud_rate(void) {
    u8 baudrate_index = read_baud_rate();
    u8 size_baud_list = sizeof(baudrate_list) / sizeof(baudrate_list[0]);
    u8 baud_buf[1];

    do {
        rgb_blink(50000);
    } while (!gpio_read(KEY_USER));
    if (++baudrate_index == size_baud_list)
        baudrate_index = 0;

    baud_buf[0] = baudrate_index;
    tinyFlash_Write(STORAGE_BAUD, (unsigned char*)baud_buf, (u8) 1);

#if defined(GPIO_LED_W)
    gpio_write(GPIO_LED_W, 1);
    sleep_us(1000000);
    for (size_t i=0; i<baudrate_index + 1; i++)
        rgb_blink(500000);
    sleep_us(1000000);
    gpio_write(GPIO_LED_W, 0);
#endif

    init_uart(baudrate_list[baudrate_index]);
}

u8 get_baud_rate_count(void) {
    return sizeof(baudrate_list) / sizeof(baudrate_list[0]);
}

u32 get_baud_rate_value(u8 baudrate_index) {
    if(baudrate_index >= get_baud_rate_count())
        return 0;
    return baudrate_list[baudrate_index];
}

u8 set_baud_rate_index(u8 baudrate_index) {
    u8 baud_buf[1];
    if(baudrate_index >= get_baud_rate_count())
        return 1;
    baud_buf[0] = baudrate_index;
    tinyFlash_Write(STORAGE_BAUD, (unsigned char*)baud_buf, (u8) 1);
    init_uart(baudrate_list[baudrate_index]);
    return 0;
}

__attribute__((optimize("-Os")))
// Callbacks into scanning.c to push async events into ad_fifo
extern void scanning_conn_event_cb(u8 state, u16 handle, u16 interval_125us, u8 peer_addr_type, u8 *peer_addr);

int app_controller_event_callback(u32 h, u8 *p, int n) {
#if DEBUG_MSG > 0
    debug_app_controller_event_callback(h, p, n);
#endif
	if (h & HCI_FLAG_EVENT_BT_STD) { // ble controller hci event
		u8 evtCode = h & 0xff;

		if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE) {
			hci_disconnectionCompleteEvt_t *e = (hci_disconnectionCompleteEvt_t *)p;
			if(e->connHandle == acl_conn_handle) {
				acl_conn_state = 0;
				acl_conn_handle = 0;
			}
			scanning_conn_event_cb(0, e->connHandle, 0, acl_peer_addr_type, acl_peer_addr);
		} else if(evtCode == HCI_EVT_LE_META) { //LE Event
			if(p[0] == HCI_SUB_EVT_LE_CONNECTION_COMPLETE) {
				hci_le_connectionCompleteEvt_t *e = (hci_le_connectionCompleteEvt_t *)p;
				if(e->status == BLE_SUCCESS && e->role == ACL_ROLE_CENTRAL) {
					acl_conn_state = 2;
					acl_conn_handle = e->connHandle;
					acl_conn_interval = (u8)e->connInterval;
					acl_peer_addr_type = e->peerAddrType;
					memcpy(acl_peer_addr, e->peerAddr, 6);
				} else {
					acl_conn_state = 0;
					acl_conn_handle = 0;
				}
				scanning_conn_event_cb(acl_conn_state, acl_conn_handle, e->connInterval,
						e->peerAddrType, e->peerAddr);
			}
			// (p[0] == HCI_SUB_EVT_LE_DIRECT_ADVERTISE_REPORT) ???
			else if (p[0] == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT)
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

// GATT data handler removed — L2CAP/GATT not initialized to save ~1180 B of SMP/GATT library BSS.
// CMD_ID_TXDATA and CMD_ID_RXDATA are disabled in this build.

// --- Connection management API ---

u8 conn_state_get(void)          { return acl_conn_state; }
u16 conn_handle_get(void)        { return acl_conn_handle; }
void conn_peer_addr_get(u8 *out) { memcpy(out, acl_peer_addr, 6); }
u8 conn_peer_addr_type_get(void) { return acl_peer_addr_type; }
u16 conn_interval_get(void)      { return acl_conn_interval; }

ble_sts_t conn_start(u8 peer_addr_type, u8 *peer_addr, u8 init_phy) {
	(void)peer_addr_type; (void)peer_addr; (void)init_phy;
	// ACL central role module not initialized — CMD_ID_CONN connect disabled in this build.
	return (ble_sts_t)1; // BLE_ERR_CMD_DISALLOWED
}

ble_sts_t conn_stop(void) {
	if(acl_conn_state == 2)
		return blc_ll_disconnect(acl_conn_handle, HCI_ERR_REMOTE_USER_TERM_CONN);
	if(acl_conn_state == 1)
		return blc_ll_createConnectionCancel();
	return BLE_SUCCESS;
}

__attribute__((optimize("-Os")))
void init_ble(void) {
	////////////////// BLE stack initialization Begin //////////////////////
#if 1
	u8 mac_random_static[6];
	blc_readFlashSize_autoConfigCustomFlashSector();
	blc_app_loadCustomizedParameters_normal();
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
#else
	generateRandomNum(5, mac_public);
	mac_public[5] = 0xC0; 			// for random static
#endif

	blc_ll_initBasicMCU(); //must

	tinyFlash_Init(0x70000, 0x4000);
    init_uart(baudrate_list[read_baud_rate()]);
	crcInit();

	blc_ll_initStandby_module(mac_public); //must

	blc_ll_initExtendedScanning_module();	//extended scan module
	blc_ll_initExtendedInitiating_module();

	blc_ll_initPeriodicAdvertisingSynchronization_module();

	blc_ll_initAclConnection_module();

	blc_hci_registerControllerEventHandler(app_controller_event_callback);
	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd( HCI_LE_EVT_MASK_ADVERTISING_REPORT
			| HCI_LE_EVT_MASK_DIRECT_ADVERTISING_REPORT
			| HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_ESTABLISHED
			| HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT
			| HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_LOST
			| HCI_LE_EVT_MASK_SCAN_REQUEST_RECEIVED
			| HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT);

	//blc_controller_check_appBufferInitialization(); // removed: may crash if ACL FIFOs not inited

	blc_gap_init();

	rf_set_power_level_index(MY_RF_POWER);
	//start_adv_scanning(3, SCAN_INTERVAL_30MS);
}

void set_coded_min_scan_window(u16 tdw) {
	if(tdw < SCAN_INTERVAL_10MS)
		tdw = SCAN_INTERVAL_10MS;
	coded_min_scan_window = tdw;
}

u16 get_coded_min_scan_window(void) {
	return coded_min_scan_window;
}

void set_primary_scan_channels(u8 chn0, u8 chn1, u8 chn2) {
	primary_scan_channels[0] = chn0;
	primary_scan_channels[1] = chn1;
	primary_scan_channels[2] = chn2;
	blc_ll_setCustomizedPrimaryChannel(chn0, chn1, chn2);
}

void get_primary_scan_channels(u8 *chn0, u8 *chn1, u8 *chn2) {
	*chn0 = primary_scan_channels[0];
	*chn1 = primary_scan_channels[1];
	*chn2 = primary_scan_channels[2];
}

void set_runtime_rf_power(u8 power) {
	runtime_rf_power = power;
	rf_set_power_level_index((RF_PowerTypeDef)power);
}

u8 get_runtime_rf_power(void) {
	return runtime_rf_power;
}

void set_runtime_rf_cap(u8 cap) {
	runtime_rf_cap = cap;
	rf_update_internal_cap(cap);
}

u8 get_runtime_rf_cap(void) {
	return runtime_rf_cap;
}

ble_sts_t txadv_start(u8 phy, u16 interval_units, const u8 *adv_data, u8 adv_len) {
	(void)phy; (void)interval_units; (void)adv_data; (void)adv_len;
	return (ble_sts_t)1; // ext adv module not initialized
}

void txadv_stop(void) {
	// ext adv module not initialized — no-op
}

// Scannning_Interval, Time = N * 0.625 ms

/*
During scanning, the Link Layer listens on a primary advertising channel index
for the duration of the scan window (scanWindow). The scan interval (scanInterval)
is defined as the interval between the start of two consecutive scan windows.
If the scanWindow and the scanInterval parameters are set to the same value
by the Host, the Link Layer should scan continuously.
*/

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
		if(flg & 2) {
			t2 = tdw;
			if(t2 < coded_min_scan_window)
				t2 = coded_min_scan_window;
		}
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

#if defined(GPIO_LED_R) && defined(GPIO_LED_W)
            gpio_write(GPIO_LED_R, 1);  // Show error...
            gpio_write(GPIO_LED_W, 1);
            sleep_us(5000000); // ...for 5 seconds
            gpio_write(GPIO_LED_R, 0);
            gpio_write(GPIO_LED_W, 0);
#endif
        }
		if (blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, (flg >> 3) & 1,
				SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS)
        ) {
#if defined(GPIO_LED_R) && defined(GPIO_LED_W)
            gpio_write(GPIO_LED_R, 1);  // Show error...
            gpio_write(GPIO_LED_W, 1);
            sleep_us(2000000); // ...for 2 seconds
            gpio_write(GPIO_LED_R, 0);
            gpio_write(GPIO_LED_W, 0);
#endif
        };
	} else {
		if (blc_ll_setExtScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE,
				SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS)
        ) {
#if defined(GPIO_LED_R) && defined(GPIO_LED_W)
            gpio_write(GPIO_LED_R, 1);  // Show error...
            gpio_write(GPIO_LED_W, 1);
            sleep_us(2000000); // ...for 2 seconds
            gpio_write(GPIO_LED_R, 0);
            gpio_write(GPIO_LED_W, 0);
#endif
        };
	}
}
