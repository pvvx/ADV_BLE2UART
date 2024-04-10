#include "app_config.h"
#if DEBUG_MSG > 0

#define blc_ll_setExtScanParam(x1, x2, x3, x4, x5, x6, x7, x8, x9) debug_blc_ll_setExtScanParam(x1, x2, x3, x4, x5, x6, x7, x8, x9)
#define blc_ll_setExtScanEnable(x1, x2, x3, x4) debug_blc_ll_setExtScanEnable(x1, x2, x3, x4)

ble_sts_t debug_blc_ll_setExtScanParam(
    own_addr_type_t     p_ownAddrType,
    scan_fp_type_t      p_scan_fp,
    scan_phy_t          p_scan_phys,
    scan_type_t         p_scanType_0,
    scan_inter_t        p_scanInter_0,
    scan_wind_t         p_scanWindow_0,
    scan_type_t         p_scanType_1,
    scan_inter_t        p_scanInter_1,
    scan_wind_t         p_scanWindow_1
);

ble_sts_t debug_blc_ll_setExtScanEnable(
    scan_en_t           extScan_en,
    dupe_fltr_en_t      filter_duplicate,
    scan_durn_t         duration,
    scan_period_t       period
);

void debug_app_controller_event_callback(u32 h, u8 *p, int n);

#endif
