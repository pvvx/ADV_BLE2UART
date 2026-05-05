#pragma once

#define MTU_DATA_SIZE 64 // if UART -> EXTADV_RPT_DATA_LEN_MAX + 12

#define MY_RF_POWER	RF_POWER_P0p04dBm

extern u8 mac_public[6];

void init_ble(void);
void start_adv_scanning(u8 flg, u16 tdw);

u8 read_baud_rate(void);
void change_baud_rate(void);

u8  get_baud_rate_count(void);
u32 get_baud_rate_value(u8 baudrate_index);
u8  set_baud_rate_index(u8 baudrate_index);

void set_coded_min_scan_window(u16 tdw);
void set_primary_scan_channels(u8 chn0, u8 chn1, u8 chn2);
void set_runtime_rf_power(u8 power);
void set_runtime_rf_cap(u8 cap);

// CMD_ID_TXADV: phy values  0=Legacy 1M, 1=Extended 1M, 2=Extended Coded
ble_sts_t txadv_start(u8 phy, u16 interval_units, const u8 *adv_data, u8 adv_len);
void      txadv_stop(void);

// CMD_ID_CONN: central role — connect/disconnect/status/data
u8        conn_state_get(void);
u16       conn_handle_get(void);
void      conn_peer_addr_get(u8 *out);
u16       conn_interval_get(void);
ble_sts_t conn_start(u8 peer_addr_type, u8 *peer_addr, u8 init_phy); // 0=1M 1=Coded
ble_sts_t conn_stop(void);
// conn_txdata disabled: L2CAP/GATT not initialized (saves ~1180 B SMP/GATT library BSS)
