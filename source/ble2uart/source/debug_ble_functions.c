#include "app_config.h"
#include "stack/ble/ble.h"
#if DEBUG_MSG > 0

#include "app_printf.h"
#pragma message("--> DEBUG_MSG enabled in app_config.h")

ble_sts_t debug_blc_ll_setExtScanParam(
        own_addr_type_t     p_ownAddrType,
        scan_fp_type_t      p_scan_fp,
        scan_phy_t          p_scan_phys,
        scan_type_t         p_scanType_0,
        scan_inter_t        p_scanInter_0,
        scan_wind_t         p_scanWindow_0,
        scan_type_t         p_scanType_1,
        scan_inter_t        p_scanInter_1,
        scan_wind_t         p_scanWindow_1) {

    char * ownAddrType = "Unknown";
    switch (p_ownAddrType) {
        case 0: ownAddrType = "PUBLIC"; break;
        case 1: ownAddrType = "RANDOM"; break;
        case 2: ownAddrType = "RESOLVE_PRIVATE_PUBLIC"; break;
        case 3: ownAddrType = "RESOLVE_PRIVATE_RANDOM"; break;
    }
    char * scan_fp = "Unknown";
    switch (p_scan_fp) {
        case 0: scan_fp = "ALLOW_ADV_ANY"; break;
        case 1: scan_fp = "ALLOW_ADV_WL"; break;
        case 2: scan_fp = "ALLOW_UNDIRECT_ADV"; break;
        case 3: scan_fp = "ALLOW_ADV_WL_DIRECT_ADV_MACTH"; break;
    }
    char * scan_phys = "Unknown";
    switch (p_scan_phys) {
        case SCAN_PHY_1M: scan_phys = "1M"; break;
        case SCAN_PHY_CODED: scan_phys = "CODED"; break;
        case SCAN_PHY_1M_CODED: scan_phys = "1M_&_CODED"; break;
    }
    uart_printf("blc_ll_setExtScanParam(AddrType=%s, fp=%s, scan_phys=%s, Type_1M=%s, Interval_1M=%d, Duration_1M=%d, Type_C=%s, Interval_C=%d, Duration_C=%d)",
        ownAddrType, // ownAddrType - Own_Address_Type
        scan_fp, // scan_fp - Scanning_Filter_Policy
        scan_phys, // scan_phys - Scanning_PHYs, "SCAN_PHY_1M" or "SCAN_PHY_CODED"
        (p_scanType_0) != 0 ? "Active" : "Passive", // Scan_Type for 1M PHY, Passive Scanning or Active Scanning.
        p_scanInter_0, p_scanWindow_0, // Scan_Interval and Duration of the scan on the primary advertising physical channel for 1M PHY
        (p_scanType_1) != 0 ? "Active" : "Passive", // Scan_Type for Coded PHY, Passive Scanning or Active Scanning.
        p_scanInter_1, p_scanWindow_1 // Scan_Interval and Duration of the scan on the on the primary advertising physical channel for Coded PHY
    );

    return blc_ll_setExtScanParam(
        p_ownAddrType,
        p_scan_fp,
        p_scan_phys,
        p_scanType_0,
        p_scanInter_0, p_scanWindow_0,
        p_scanType_1,
        p_scanInter_1, p_scanWindow_1
    );
}

ble_sts_t debug_blc_ll_setExtScanEnable(
        scan_en_t           extScan_en,
        dupe_fltr_en_t      filter_duplicate,
        scan_durn_t         duration,
        scan_period_t       period) {

    uart_printf("blc_ll_setExtScanEnable(extScan_en=%s, filter_duplicate=%s, duration=%d%s, period=%d%s)",
        extScan_en ? "Scanning_enabled": "Scanning_disabled",
        filter_duplicate ? "Filter_Duplicates" : "Do_Not_Filter_Duplicates",
        duration,
        duration !=0 ? "" : "=Continuous",
        period,
        period != 0 ? "" : "=Continuous"
    );

    return blc_ll_setExtScanEnable(extScan_en, filter_duplicate, duration, period);
}

void debug_app_controller_event_callback(u32 h, u8 *p, int n) {
    char * event_type = "Unknown";
	if (h & HCI_FLAG_EVENT_BT_STD) { // ble controller hci event
		u8 evtCode = h & 0xff;
		if (evtCode == HCI_EVT_LE_META) { //LE Event
			if (p[0] == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT) {
                //event_type = "HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT";
                event_type = NULL;  // do not debug this event
            }
			else if (p[0] == HCI_SUB_EVT_LE_ADVERTISING_REPORT) {
                event_type = "HCI_SUB_EVT_LE_ADVERTISING_REPORT";
            }
			else if (p[0] == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT) {
                event_type = "HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT";
            }
			else if (p[0] == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED) {
                event_type = "HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED";
            }
			else if (p[0] == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST) {
                event_type = "HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST";
            }
        if (event_type != NULL)
            uart_printf("app_controller_event_callback(event=%d, event_type=%s)", evtCode, event_type);
		}
        else
            uart_printf("app_controller_event_callback(event=%d, event_type='NOT LE event')", evtCode);
	}
    else
        uart_printf("app_controller_event_callback(event=%d, event_type='not HCI_FLAG_EVENT_BT_STD')", h);
}

#endif
