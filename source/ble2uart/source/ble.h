#pragma once

#define MTU_DATA_SIZE 64 // if UART -> EXTADV_RPT_DATA_LEN_MAX + 12

#define MY_RF_POWER	RF_POWER_P0p04dBm

extern u8 mac_public[6];
//extern u8 mac_random_static[6];

void init_ble(void);
void start_adv_scanning(u8 flg, u16 tdw);

