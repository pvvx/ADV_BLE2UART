#include "app_config.h"
#include "tl_common.h"
#include "app_buffer.h"
#include "drivers.h"
#include "drivers/B85/gpio.h"
#include "ble.h"
#include "app.h"
#include "stack/ble/ble.h"
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
unsigned int baudrate_list[] = { 2000000, 921600, 115200 };  // List of available UART baudrates in bit-per-second

static u16 coded_min_scan_window = SCAN_INTERVAL_100MS;
static u8 primary_scan_channels[3] = {37, 38, 39};

/* The two scan PHYs share the same radio, so their scan windows must not add up
   to more than the available time.  The host command (CMD_ID_SCAN) sends a scan
   *window*; the interval is derived from it here:
     one PHY enabled -> interval = 1.5 x window  (66% duty, i.e. the values of
                                                  Telink's Coded branch: 150/100)
     1M + Coded      -> interval = 3   x window  (33% + 33% = 66% of the radio)
   Before this the interval was set equal to the window, so every enabled PHY
   asked for 100% of the radio (200% with both PHYs): a schedule that cannot be
   built, and on V4.0.2.5 the stack then stops at the first Coded packet.
   Both values are in 0.625 ms units. */
#define SCAN_INTERVAL_FROM_WINDOW(win, flg) \
 ((u16)((((flg) & 3) == 3) ? (u32)(win) * 3 : ((u32)(win) * 3) / 2))
static u8 runtime_rf_power = MY_RF_POWER;
static u8 runtime_rf_cap = 0xff;

/* ---------------------------------------------------------------------------
 * CMD_ID_TXADV - transmit a custom advertisement
 *
 * The LEGACY advertising API (blc_ll_setAdvParam / setAdvData / setAdvEnable)
 * is used: only legacy 1M PDUs can be transmitted ("phy" must be 0).  The
 * extended advertising API of the V4.0.2.5 library wedges the RF interrupt
 * handler in the configs tried, so it is not used here.
 * ------------------------------------------------------------------------- */
#define TXADV_ADV_DATA_LEN  31   /* legacy advertising payload limit */

#if TXADV_ENABLE
/* The legacy advertising module keeps the pointer passed to setAdvData, so the
   data has to stay valid while advertising is enabled. */
static u8 txadv_advData[TXADV_ADV_DATA_LEN];
static u8 txadv_enabled;
#endif

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
u16 conn_interval_get(void)      { return acl_conn_interval; }

ble_sts_t conn_start(u8 peer_addr_type, u8 *peer_addr, u8 init_phy) {
	ble_sts_t st;

	/* Only the legacy initiating module is initialized: it is mutually
	   exclusive with the extended initiating module, and the scan-only
	   configuration that runs on V4.0.2.5 uses it.  Legacy initiating
	   connects on the 1M PHY only. */
	if(init_phy)
		return (ble_sts_t)LL_ERR_INVALID_PARAMETER;

	st = blc_ll_createConnection(SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS,
			INITIATE_FP_ADV_SPECIFY, peer_addr_type, peer_addr,
			OWN_ADDRESS_PUBLIC,
			CONN_INTERVAL_10MS, CONN_INTERVAL_10MS,
			0, CONN_TIMEOUT_4S, 0, 0xFFFF);
	if(st == BLE_SUCCESS)
		acl_conn_state = 1;
	return st;
}

ble_sts_t conn_stop(void) {
	if(acl_conn_state == 2)
		return blc_ll_disconnect(acl_conn_handle, HCI_ERR_REMOTE_USER_TERM_CONN);
	if(acl_conn_state == 1)
		return blc_ll_createConnectionCancel();
	return BLE_SUCCESS;
}

// GATT client (CMD_ID_TXDATA, CMD_ID_RXDATA, CMD_ID_CONN ops 5..10).
// Only compiled when GATT_ENABLE is set: the L2CAP/ATT/GATT host stack does not
// fit in SRAM together with the extended advertising module (CMD_ID_TXADV).
#if GATT_ENABLE

/* Incoming ATT packet from the peer (master role). */
static int gatt_data_handler(u16 connHandle, u8 *pkt) {
	rf_packet_att_t *pAtt = (rf_packet_att_t *)pkt;
	/* l2capLen counts the opcode (1) + handle (2) + value */
	u16 len = (pAtt->l2capLen > 3) ? (u16)(pAtt->l2capLen - 3) : 0;

	scanning_gatt_callback(connHandle, pAtt->opcode, pAtt->handle, len, pAtt->dat);

	/* An indication must be confirmed at ATT level. */
	if(pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
		blc_gatt_pushConfirm(connHandle);
	return 0;
}

static ble_sts_t gatt_ready(void) {
	if(acl_conn_state != 2)
		return (ble_sts_t)LL_ERR_CONNECTION_NOT_ESTABLISH;
	return BLE_SUCCESS;
}

ble_sts_t conn_gatt_write_cmd(u16 att_handle, const u8 *data, u8 len) {
	ble_sts_t st = gatt_ready();
	if(st != BLE_SUCCESS)
		return st;
	return blc_gatt_pushWriteCommand(acl_conn_handle, att_handle, (u8 *)data, (int)len);
}

ble_sts_t conn_gatt_read(u16 att_handle) {
	ble_sts_t st = gatt_ready();
	if(st != BLE_SUCCESS)
		return st;
	return blc_gatt_pushReadRequest(acl_conn_handle, att_handle);
}

ble_sts_t conn_gatt_write(u16 att_handle, const u8 *data, u8 len) {
	ble_sts_t st = gatt_ready();
	if(st != BLE_SUCCESS)
		return st;
	return blc_gatt_pushWriteRequest(acl_conn_handle, att_handle, (u8 *)data, (int)len);
}

ble_sts_t conn_gatt_disc_services(void) {
	ble_sts_t st = gatt_ready();
	u16 uuid = GATT_UUID_PRIMARY_SERVICE;
	if(st != BLE_SUCCESS)
		return st;
	return blc_gatt_pushReadByGroupTypeRequest(acl_conn_handle, 0x0001, 0xFFFF,
			(u8 *)&uuid, sizeof(uuid));
}

ble_sts_t conn_gatt_disc_chars(u16 start, u16 end) {
	ble_sts_t st = gatt_ready();
	u16 uuid = GATT_UUID_CHARACTER;
	if(st != BLE_SUCCESS)
		return st;
	return blc_gatt_pushReadByTypeRequest(acl_conn_handle, start, end,
			(u8 *)&uuid, sizeof(uuid));
}

ble_sts_t conn_gatt_disc_descrs(u16 start, u16 end) {
	ble_sts_t st = gatt_ready();
	if(st != BLE_SUCCESS)
		return st;
	return blc_gatt_pushFindInformationRequest(acl_conn_handle, start, end);
}

ble_sts_t conn_gatt_set_mtu(u16 mtu) {
	if(mtu > ATT_MTU_MASTER_RX_MAX_SIZE)
		mtu = ATT_MTU_MASTER_RX_MAX_SIZE;
	if(mtu < 23)	/* default ATT MTU */
		mtu = 23;
	return blc_att_setMasterRxMTUSize(mtu);
}

#endif /* GATT_ENABLE */

// NOTE: no __attribute__((optimize(...))) here: see the toolchain pitfall note
// in scanning.c / the README (it corrupts switch jump tables).
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

	bb_checkpoint(0x00050001u);

	/* Extended scanning + legacy initiating modules: the scanner and the
	   CMD_ID_CONN central role need them. */
	blc_ll_initExtendedScanning_module();	//extended scan module
	blc_ll_initLegacyInitiating_module();  //initiating module (blc_ll_createConnection API)

	/* Coded PHY / 2M PHY are disabled by default in this SDK to save SRAM
	   ("In the tc_ble_sdk, to conserve SRAM, the Coded PHY/2M PHY is disabled
	   by default", handbook chapter "Coded PHY/2M PHY") and must be enabled
	   explicitly with blc_ll_init2MPhyCodedPhy_feature().
	   WITHOUT this call the extended scan never starts in tc_ble_sdk
	   V4.0.2.5: blc_ll_setExtScanEnable() returns BLE_SUCCESS, but the LL state
	   is never established, g_scheMng+0x14 stays 0, the ISR then switches off
	   the System Timer interrupt and the firmware freezes (with V4.0.2.1/4.0.2.3
	   the omission went unnoticed on 1M-only scans, which is why this was never
	   caught here).  rf_drv_ble_init() alone is NOT enough. */
	blc_ll_init2MPhyCodedPhy_feature();	//enable Coded PHY/2M PHY feature
	bb_checkpoint(0x00050002u);

	/* CMD_ID_TXADV: the LEGACY advertising module, required by
	   blc_ll_setAdvParam/Data/Enable ("phy" of CMD_ID_TXADV is then only
	   0 = 1M ADV_NONCONN_IND).

	   WARNING - measured on the TB-03F-KIT, native V4.0.2.5 library, with an
	   interleaved A/B (three runs each, CMD_ID_SCAN mode 3, 15 s):

	     build                                          Coded scan
	     --------------------------------------------   ----------
	     this line removed (module not referenced)      3/3 OK
	     this line present (module may never be used)   0/3 OK

	   The failing runs die on the first Coded advertisement: the host sees the
	   boot report frames instead of reports and the scan-stop command is not
	   answered.  It is not the call itself (it is not executed unless TXADV is
	   started) but what referencing it pulls into the link: 2308 bytes of
	   library BSS (`_ram_use_end_` 0x84D928 -> 0x84E22C), which re-orders the
	   RAM layout.  The Coded-PHY scan of this library is sensitive to it; a
	   2308 byte application array changes nothing.

	   Consequence: CMD_ID_TXADV (legacy advertising) and the Coded-PHY scan
	   cannot both be had on V4.0.2.5.  TXADV_ENABLE=0 keeps the full 1M + Coded
	   scan (and makes 0x0B answer CMD_STATUS_DENIED); TXADV_ENABLE=1 keeps the
	   legacy advertisement transmitter at the cost of the Coded-PHY scan.
	   Scanning and TXADV also share the radio, so the command handlers stop one
	   before starting the other (see scan_task() in scanning.c). */
#if TXADV_ENABLE
	blc_ll_initLegacyAdvertising_module();
#endif
	bb_checkpoint(0x00050003u);

	/* NOTE: blc_ll_initPeriodicAdvertisingSynchronization_module() is NOT
	   called here any more.  The reference configuration that works on
	   V4.0.2.5 (vendor/acl_central_demo, and coded_phy_scan_repro) does not
	   enable it, no code in this firmware ever creates a periodic sync
	   (no blc_ll_periodic* call), and the V4.0.2.5 library reworked the
	   whole blt_pda_sync_* family, which is the machinery behind it. */
	//blc_ll_initPeriodicAdvertisingSynchronization_module();

	blc_ll_initAclConnection_module();

        /* ACL connection configuration, as in the stock vendor/acl_central_demo:
           the Link Layer expects these to be set up even in a scan-only build.
           Without them the Coded PHY extended scan never receives anything on
           V4.0.2.5 and the firmware stops (the ACL FIFOs are also required by
           blc_hci_registerControllerDataHandler() below). */
        blc_ll_initAclCentralRole_module();
        /* The advertising role: advertising is a peripheral-role activity and
           CMD_ID_TXADV (legacy advertising API) needs it - without the peripheral
           role module blc_ll_setAdvEnable answers 0x0D (LIMITED_RESOURCES),
           measured in ext_adv_test mode 2 vs mode 3.  Only needed (and only
           initialised) when TXADV_ENABLE=1. */
#if TXADV_ENABLE
        blc_ll_initAclPeriphrRole_module();
        blc_ll_initAclPeriphrTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE,
                                    ACL_SLAVE_TX_FIFO_NUM, SLAVE_MAX_NUM);
#endif
        blc_ll_setMaxConnectionNumber(MASTER_MAX_NUM, SLAVE_MAX_NUM);
        blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS,
                                         ACL_MASTER_MAX_TX_OCTETS,
                                         ACL_SLAVE_MAX_TX_OCTETS);
        blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
        blc_ll_initAclCentralTxFifo(app_acl_mstTxfifo, ACL_MASTER_TX_FIFO_SIZE,
                                    ACL_MASTER_TX_FIFO_NUM, MASTER_MAX_NUM);
        blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_10MS);

	blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler);
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

        /* L2CAP/ATT buffers used by the controller data handler above. */
        blc_l2cap_initAclConnMasterMtuBuffer(mtu_m_rx_fifo, MTU_M_BUFF_SIZE_MAX, 0, 0);
        blc_att_setMasterRxMTUSize(ATT_MTU_MASTER_RX_MAX_SIZE);

#if GATT_ENABLE
        /* GATT client: the host stack routes every incoming ATT packet to this
           handler once it is registered (GAP/L2CAP were set up above). */
        blc_gatt_register_data_handler(gatt_data_handler);
#endif

	rf_set_power_level_index(MY_RF_POWER);

	bb_checkpoint(0x00050010u);
}

void set_coded_min_scan_window(u16 tdw) {
	if(tdw < SCAN_INTERVAL_10MS)
		tdw = SCAN_INTERVAL_10MS;
	coded_min_scan_window = tdw;
}

void set_primary_scan_channels(u8 chn0, u8 chn1, u8 chn2) {
	primary_scan_channels[0] = chn0;
	primary_scan_channels[1] = chn1;
	primary_scan_channels[2] = chn2;
	blc_ll_setCustomizedPrimaryChannel(chn0, chn1, chn2);
}

void set_runtime_rf_power(u8 power) {
	runtime_rf_power = power;
	rf_set_power_level_index((RF_PowerTypeDef)power);
}

void set_runtime_rf_cap(u8 cap) {
	runtime_rf_cap = cap;
	rf_update_internal_cap(cap);
}

/* -------------------------------------------------------------------------
 * System-Timer keep-alive
 *
 * With the native V4.0.2.5 library, blc_sdk_irq_handler() switches
 * FLD_IRQ_SYSTEM_TIMER off as soon as g_scheMng+0x14 (the "who needs the System
 * Timer" bitmap) becomes 0.  The scheduler tick then never returns, so the main
 * loop stops for good: no more UART answers, LEDs frozen.
 *
 * Bit 19 (0x00080000) is masked out by tlkstk_sch_updateScheduler (mask
 * 0xff87ffff), so the scheduler ignores it, but it keeps the field non-zero and
 * therefore the timer interrupt alive.
 * ------------------------------------------------------------------------- */
/* SCHED_TIMER_REQ_OFFSET / SCHED_TIMER_KEEPALIVE_BIT and the g_scheMng extern now
   live in ble.h because scanning.c (sched_timer_guard) uses them too. */

#if TXADV_ENABLE
static ble_sts_t txadv_do_params(u8 phy, u16 interval_units) {
	u32 interval = interval_units;

	bb_checkpoint(0x000c0001u);
	/* The legacy API transmits legacy PDUs only: "phy" 0 = 1M ADV_NONCONN_IND. */
	if(phy != 0)
		return (ble_sts_t)LL_ERR_INVALID_PARAMETER;
	if(interval < ADV_INTERVAL_20MS)
		interval = ADV_INTERVAL_20MS;
	else if(interval > ADV_INTERVAL_10_24S)
		interval = ADV_INTERVAL_10_24S;

	return blc_ll_setAdvParam((adv_inter_t)interval, (adv_inter_t)interval,
				  ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
				  0, 0, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
}

static ble_sts_t txadv_do_data(const u8 *adv_data, u8 adv_len) {
	bb_checkpoint(0x000c0002u);
	if(adv_len > TXADV_ADV_DATA_LEN)
		return (ble_sts_t)LL_ERR_INVALID_PARAMETER;
	memcpy(txadv_advData, adv_data, adv_len);
	return blc_ll_setAdvData(txadv_advData, adv_len);
}

static ble_sts_t txadv_do_enable(void) {
	ble_sts_t st;

	bb_checkpoint(0x000c0003u);
	st = blc_ll_setAdvEnable(BLC_ADV_ENABLE);
	if(st == BLE_SUCCESS)
		txadv_enabled = 1;
	return st;
}

static void txadv_do_disable(void) {
	bb_checkpoint(0x000c0004u);
	blc_ll_setAdvEnable(BLC_ADV_DISABLE);
	txadv_enabled = 0;
}

ble_sts_t txadv_start(u8 phy, u16 interval_units, const u8 *adv_data, u8 adv_len) {
	ble_sts_t st;

	/* A parameter update is refused while the set is enabled (0x0C). */
	if(txadv_enabled)
		txadv_do_disable();

	st = txadv_do_params(phy, interval_units);
	if(st != BLE_SUCCESS)
		return st;

	st = txadv_do_data(adv_data, adv_len);
	if(st != BLE_SUCCESS)
		return st;

	return txadv_do_enable();
}

void txadv_stop(void) {
	if(txadv_enabled)
		txadv_do_disable();
}
#else /* !TXADV_ENABLE */
ble_sts_t txadv_start(u8 phy, u16 interval_units, const u8 *adv_data, u8 adv_len) {
	(void)phy; (void)interval_units; (void)adv_data; (void)adv_len;
	return (ble_sts_t)LL_ERR_INVALID_PARAMETER;   /* TXADV_ENABLE = 0 */
}

void txadv_stop(void) {
}
#endif /* TXADV_ENABLE */

// Scannning_Interval, Time = N * 0.625 ms

/*
During scanning, the Link Layer listens on a primary advertising channel index
for the duration of the scan window (scanWindow). The scan interval (scanInterval)
is defined as the interval between the start of two consecutive scan windows.
If the scanWindow and the scanInterval parameters are set to the same value
by the Host, the Link Layer should scan continuously.
*/

u8 scanning_active;

void start_adv_scanning(u8 flg, u16 tdw_1m, u16 tdw_coded) {
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
		if(tdw_1m < SCAN_INTERVAL_10MS)
			tdw_1m = SCAN_INTERVAL_10MS;
		u32 t1 = 0, t2 = 0;
		if(flg & 1)
			t1 = tdw_1m;
		if(flg & 2) {
			if(tdw_coded == 0) {
				// Legacy behaviour: use tdw_1m as base, apply coded_min floor
				t2 = tdw_1m;
			} else {
				t2 = tdw_coded;
			}
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
            SCAN_INTERVAL_FROM_WINDOW(t1, flg), t1, // Scan_Interval and Duration of the scan on the primary advertising physical channel for 1M PHY
            (flg >> 2) & 1, // Scan_Type for Coded PHY, Passive Scanning or Active Scanning.
            SCAN_INTERVAL_FROM_WINDOW(t2, flg), t2 // Scan_Interval and Duration of the scan on the on the primary advertising physical channel for Coded PHY
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
        } else {
            scanning_active = 1;
        }
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
        } else {
            scanning_active = 0;
        }
	}
}
