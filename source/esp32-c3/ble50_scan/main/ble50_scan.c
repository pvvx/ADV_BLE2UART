/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */



/****************************************************************************
*
* This file is for gatt_security_client demo. It can scan ble device, connect one device that needs to be encrypted.
* run gatt_security_server demo, the gatt_security_client demo will automatically connect the gatt_security_server,
* then paring and bonding.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "crc.h"
#include "freertos/FreeRTOS.h"
#include "crc.h"
#include "driver/usb_serial_jtag.h"
#include "hal/usb_serial_jtag_ll.h"


#define SW_VERSION		0x01


#define HEAD_CRC_ADD_LEN	13

#define EXTADV_RPT_DATA_LEN_MAX 229 // //253 - 24 = 229

#define IN_BUF_SIZE 		64

#define OUT_BUF_SIZE 		(EXTADV_RPT_DATA_LEN_MAX + HEAD_CRC_ADD_LEN)

#define IO_TASK_STACK_SIZE	2048

enum {
	CMD_ID_INFO		= 0x00,
	CMD_ID_SCAN 	= 0x01, //  Scan on/off, parameters
	CMD_ID_WMAC		= 0x02, // add white mac
	CMD_ID_BMAC 	= 0x03, // add black mac
	CMD_ID_CLRM		= 0x04  // clear mac list
} CMD_ID_KEYS;

#define MAC_MAX_SCAN_LIST	64

enum {
	WHITE_LIST,
	BALCK_LIST
} mode_mac_list_e;

typedef struct _mac_list_t {
	uint8_t	mode;	// mode_mac_list_e
	uint8_t	count;
	uint8_t	filtr;
	uint8_t	res;
	uint8_t	mac[MAC_MAX_SCAN_LIST][6];
} mac_list_t;

mac_list_t mac_list;

#define USE_TXT_OUT 0
#define USE_CONNECT 0

#define GATTC_TAG             "SEC_GATTC_DEMO"
#define REMOTE_SERVICE_UUID   0x00FF
#define REMOTE_NOTIFY_UUID    0xFF01
#define EXT_SCAN_DURATION     0
#define EXT_SCAN_PERIOD       0

#if USE_CONNECT
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;
#endif

///Declare static functions
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
#if USE_CONNECT
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static bool connect = false;
static bool get_service = false;
static const char remote_device_name[] = "ESP_BLE50_SERVER";
#endif

#if USE_TXT_OUT
char scan_device_name[32+1];
#else
uint8_t out_buf[OUT_BUF_SIZE];
#endif


static esp_ble_ext_scan_params_t ext_scan_params = {
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
    .cfg_mask = ESP_BLE_GAP_EXT_SCAN_CFG_CODE_MASK, // | ESP_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK,
    .uncoded_cfg = {BLE_SCAN_TYPE_PASSIVE, 40, 40},
    .coded_cfg = {BLE_SCAN_TYPE_PASSIVE, 40, 40},
};

#if USE_CONNECT
const esp_ble_gap_conn_params_t phy_1m_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320,
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len  = 0,
    .max_ce_len = 0,
};
const esp_ble_gap_conn_params_t phy_2m_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320,
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len  = 0,
    .max_ce_len = 0,
};
const esp_ble_gap_conn_params_t phy_coded_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320, // 306-> 362Kbps
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len  = 0,
    .max_ce_len = 0,
};

#endif

#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t notify_char_handle;
    esp_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
    	.gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

#if USE_CONNECT

static const char *esp_key_type_to_str(esp_ble_key_type_t key_type)
{
   const char *key_str = NULL;
   switch(key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;

    }
     return key_str;
}

static char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req)
{
   char *auth_str = NULL;
   switch(auth_req) {
    case ESP_LE_AUTH_NO_BOND:
        auth_str = "ESP_LE_AUTH_NO_BOND";
        break;
    case ESP_LE_AUTH_BOND:
        auth_str = "ESP_LE_AUTH_BOND";
        break;
    case ESP_LE_AUTH_REQ_MITM:
        auth_str = "ESP_LE_AUTH_REQ_MITM";
        break;
    case ESP_LE_AUTH_REQ_BOND_MITM:
        auth_str = "ESP_LE_AUTH_REQ_BOND_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_ONLY:
        auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
        break;
    case ESP_LE_AUTH_REQ_SC_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
        break;
    default:
        auth_str = "INVALID BLE AUTH REQ";
        break;
   }

   return auth_str;
}
#endif

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
#if USE_CONNECT
	esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
#endif

	switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_ble_gap_config_local_privacy(true);
        break;
#if USE_CONNECT
    case ESP_GATTC_CONNECT_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT");
        break;
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, error status = %x", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            get_service = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        if (get_service){
            uint16_t count  = 0;
            uint16_t offset = 0;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                        ESP_GATT_DB_CHARACTERISTIC,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                        INVALID_HANDLE,
                                                                        &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
            }
            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                    break;
                }else{
                    ret_status = esp_ble_gattc_get_all_char(gattc_if,
                                                            gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                            char_elem_result,
                                                            &count,
                                                            offset);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_all_char error, %d", __LINE__);
                        free(char_elem_result);
                        char_elem_result = NULL;
                        break;
                    }
                    if (count > 0){

                        for (int i = 0; i < count; ++i)
                        {
                            if (char_elem_result[i].uuid.len == ESP_UUID_LEN_16  && (char_elem_result[i].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
                            {
                                gl_profile_tab[PROFILE_A_APP_ID].notify_char_handle = char_elem_result[i].char_handle;
                                esp_ble_gattc_register_for_notify (gattc_if,
                                                                   gl_profile_tab[PROFILE_A_APP_ID].remote_bda,
                                                                   char_elem_result[i].char_handle);
                                break;
                            }
                        }
                    }
                }
                free(char_elem_result);
                char_elem_result = NULL;
            }
        }

        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg for notify failed, error status = %x", p_data->reg_for_notify.status);
            break;
        }

            uint16_t count = 0;
            uint16_t offset = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                        ESP_GATT_DB_DESCRIPTOR,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                        gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                        p_data->reg_for_notify.handle,
                                                                        &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error, %d", __LINE__);
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                    break;
                }else{
                    ret_status = esp_ble_gattc_get_all_descr(gattc_if,
                                                             gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                             p_data->reg_for_notify.handle,
                                                             descr_elem_result,
                                                             &count,
                                                             offset);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_all_descr error, %d", __LINE__);
                    free(descr_elem_result);
                    descr_elem_result = NULL;
                    break;
                }

                    for (int i = 0; i < count; ++i)
                    {
                        if (descr_elem_result[i].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[i].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG)
                        {
                            esp_ble_gattc_write_char_descr (gattc_if,
                                                            gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                            descr_elem_result[i].handle,
                                                            sizeof(notify_en),
                                                            (uint8_t *)&notify_en,
                                                            ESP_GATT_WRITE_TYPE_RSP,
                                                            ESP_GATT_AUTH_REQ_NONE);

                            break;
                        }
                    }
                }
                free(descr_elem_result);
                descr_elem_result = NULL;
            }

        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success");
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = 0x%x", p_data->disconnect.reason);
        connect = false;
        get_service = false;
        break;
#endif
    default:
        break;
    }
}


//#define MYFIFO_BLK_SIZE		(EXTADV_RPT_DATA_LEN_MAX + HEAD_CRC_ADD_LEN) // 229+12 = 241 bytes
//MYFIFO_INIT(ad_fifo, MYFIFO_BLK_SIZE, 4); 	// (229+12)*4 = 964 bytes + sizeof(my_fifo_t)
mac_list_t mac_list;

void send_resp(uint8_t cmd, uint8_t id, uint8_t *pmac, uint8_t len) {
	uint8_t s[HEAD_CRC_ADD_LEN];
	memset(s, 0, HEAD_CRC_ADD_LEN);
	//s[0] = 0;
	s[1] = cmd; // rssi
	s[2] = id; // ev type
	s[3] = len; // addr type
	s[4] = 0xff; // phy = 0xff -> cmd response
	if(len)
		memcpy(&s[5], pmac, (len > 6)? 6 : len);
	len = HEAD_CRC_ADD_LEN - 2;
	uint16_t crc = crcFast(s, len);
	s[len++] = crc;
	s[len++] = crc >> 8;
    usb_serial_jtag_write_bytes(s, len, 5 / portTICK_PERIOD_MS);
    usb_serial_jtag_ll_txfifo_flush();
}


static int chk_mac(uint8_t *pmac) {
	int ret = 0;
	if(mac_list.count) {
		for(int i = 0; i < mac_list.count; i++) {
			if(memcmp(&mac_list.mac[i], pmac, 6) == 0) {
				ret = 1;
				break;
			}
		}
		if(mac_list.mode == BALCK_LIST)
			ret = !ret;
	} else
		ret = 1;
	return ret;
}


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "config local privacy failed, error code =%x", param->local_privacy_cmpl.status);
            break;
        }
/*
        esp_err_t scan_ret = esp_ble_gap_set_ext_scan_params(&ext_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set extend scan params error, error code = %x", scan_ret);
        } */
        break;
    case ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT: {
        if (param->set_ext_scan_params.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "extend scan parameters set failed, error status = %x", param->set_ext_scan_params.status);
            break;
        }
        //the unit of the duration is second
        esp_ble_gap_start_ext_scan(EXT_SCAN_DURATION, EXT_SCAN_PERIOD);
        break;
    }
    case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
        if (param->ext_scan_start.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Scan start success");
        break;
#if USE_CONNECT
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
        /* Call the following function to input the passkey which is displayed on the remote device */
        //esp_ble_passkey_reply(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, true, 0x00);
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
        break;
    case ESP_GAP_BLE_OOB_REQ_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
        uint8_t tk[16] = {1}; //If you paired with OOB, both devices need to use the same tk
        esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
        break;
    }
    case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
        break;
    case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should send the security response with negative(false) accept value*/
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        /* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
        show the passkey number to the user to confirm it with the number displayed by peer device. */
        esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        ESP_LOGI(GATTC_TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%" PRIu32, param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        ///show the passkey number to the user to input it in the peer device.
        ESP_LOGI(GATTC_TAG, "The passkey Notify number:%06" PRIu32, param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_KEY_EVT:
        //shows the ble key info share with peer device to the user.
        ESP_LOGI(GATTC_TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(GATTC_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(GATTC_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGI(GATTC_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGI(GATTC_TAG, "auth mode = %s",esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
        }
        break;
    }
#endif
    case ESP_GAP_BLE_EXT_ADV_REPORT_EVT: {
#if USE_TXT_OUT
        uint8_t *adv_name = NULL;
        uint8_t adv_name_len = 0;

        if(param->ext_adv_report.params.event_type & ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY) {
//            break;
            ESP_LOGI(GATTC_TAG, "legacy adv, adv type 0x%x data len %d", param->ext_adv_report.params.event_type, param->ext_adv_report.params.adv_data_len);
        } else {
            ESP_LOGI(GATTC_TAG, "extend adv, adv type 0x%x data len %d", param->ext_adv_report.params.event_type, param->ext_adv_report.params.adv_data_len);
        }
#if 0
        /**
        * @brief extend adv report parameters
        */
        typedef struct {
            // uint8_t props;
            // uint8_t legacy_event_type;
            esp_ble_gap_adv_type_t event_type;              /*!< extend advertising type */
            uint8_t addr_type;                              /*!< extend advertising address type */
            esp_bd_addr_t addr;                             /*!< extend advertising address */
            esp_ble_gap_pri_phy_t primary_phy;              /*!< extend advertising primary phy */
            esp_ble_gap_phy_t secondly_phy;                 /*!< extend advertising secondary phy */
            uint8_t sid;                                    /*!< extend advertising sid */
            uint8_t tx_power;                               /*!< extend advertising tx power */
            int8_t rssi;                                    /*!< extend advertising rssi */
            uint16_t per_adv_interval;                      /*!< periodic advertising interval */
            uint8_t dir_addr_type;                          /*!< direct address type */
            esp_bd_addr_t dir_addr;                         /*!< direct address */
            esp_ble_gap_ext_adv_data_status_t data_status;  /*!< data type */
            uint8_t adv_data_len;                           /*!< extend advertising data length */
            uint8_t adv_data[251];                          /*!< extend advertising data */
        } esp_ble_gap_ext_adv_reprot_t;
#endif
    	printf("%02x %02x ", param->ext_adv_report.params.event_type, param->ext_adv_report.params.primary_phy | (param->ext_adv_report.params.secondly_phy << 4));
        for (int j = 0; j < 6; j++) {
        	printf("%02x", param->ext_adv_report.params.addr[j]);
        }
    	printf(" %i ", param->ext_adv_report.params.rssi);
        for (int k = 0; k < param->ext_adv_report.params.adv_data_len; k++) {
        	printf("%02x", param->ext_adv_report.params.adv_data[k]);
        }
        adv_name = esp_ble_resolve_adv_data(param->ext_adv_report.params.adv_data,
                                            ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
        if(adv_name_len) {
        	adv_name_len = MIN(adv_name_len, sizeof(scan_device_name)-1);
        	memcpy(scan_device_name, (char *)adv_name, adv_name_len);
        	scan_device_name[adv_name_len] = 0;
        	printf(" %s\n", (char *)scan_device_name);
        } else
        	printf("\n");
#else
        int len = param->ext_adv_report.params.adv_data_len;
        if(len && len <= EXTADV_RPT_DATA_LEN_MAX
        		&& chk_mac(param->ext_adv_report.params.addr)
        		&& (param->ext_adv_report.params.addr_type & mac_list.filtr) == 0) {
            out_buf[0] = len;
            out_buf[1] = param->ext_adv_report.params.rssi;
            out_buf[2] = param->ext_adv_report.params.event_type;
            out_buf[3] = param->ext_adv_report.params.primary_phy | (param->ext_adv_report.params.secondly_phy << 4);
            out_buf[4] = (param->ext_adv_report.params.addr_type & 0x0f) | (param->ext_adv_report.params.dir_addr_type << 4);
            //memcpy(&out_buf[5], param->ext_adv_report.params.addr, 6);
            out_buf[5] = param->ext_adv_report.params.addr[5];
            out_buf[6] = param->ext_adv_report.params.addr[4];
            out_buf[7] = param->ext_adv_report.params.addr[3];
            out_buf[8] = param->ext_adv_report.params.addr[2];
            out_buf[9] = param->ext_adv_report.params.addr[1];
            out_buf[10] = param->ext_adv_report.params.addr[0];
			memcpy(&out_buf[11], param->ext_adv_report.params.adv_data, len);
			len += 11;
			uint16_t crc = crcFast(out_buf, len);
			out_buf[len++] = crc;
			out_buf[len++] = crc >> 8;
		    usb_serial_jtag_write_bytes(out_buf, len, 5 / portTICK_PERIOD_MS);
		    usb_serial_jtag_ll_txfifo_flush();
        }
#endif
#if USE_CONNECT
        if (!connect && strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
            connect = true;
            esp_ble_gap_stop_ext_scan();
            esp_log_buffer_hex("adv addr", param->ext_adv_report.params.addr, 6);
            esp_log_buffer_char("adv name", adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "Stop extend scan and create aux open, primary_phy %d secondary phy %d", param->ext_adv_report.params.primary_phy, param->ext_adv_report.params.secondly_phy);

            esp_ble_gap_prefer_ext_connect_params_set(param->ext_adv_report.params.addr,
                                                     ESP_BLE_GAP_PHY_1M_PREF_MASK | ESP_BLE_GAP_PHY_2M_PREF_MASK | ESP_BLE_GAP_PHY_CODED_PREF_MASK ,
                                                     &phy_1m_conn_params, &phy_2m_conn_params, &phy_coded_conn_params);
            esp_ble_gattc_aux_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                                    param->ext_adv_report.params.addr,
                                    param->ext_adv_report.params.addr_type, true);
        }
#endif
        break;
    }
    case ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT:
        if (param->ext_scan_stop.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "extend Scan stop failed, error status = %x", param->ext_scan_stop.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop extend scan successfully");
        break;

    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d", event, gattc_if);
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

uint8_t read_buf[32];

static void io_task(void *arg) {
//	uint8_t buf[HEAD_CRC_ADD_LEN];
	usb_serial_jtag_driver_config_t usb_serial_config = {.tx_buffer_size = 2048,
	                                                     .rx_buffer_size = 256 };
	ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_config));

	while(1) {
		// vTaskDelay( 5 / portTICK_PERIOD_MS );
		int len = usb_serial_jtag_read_bytes(read_buf, HEAD_CRC_ADD_LEN, 3 / portTICK_PERIOD_MS);
		//int len = uart_read(buf, sizeof(buf));
		if(len > 2 && crcFast(read_buf, len) == 0) {
			int cmd = read_buf[0];
			if(cmd == CMD_ID_SCAN && len >= 6) {
				esp_ble_gap_stop_ext_scan();
#if 0
				static esp_ble_ext_scan_params_t ext_scan_params = {
					.own_addr_type = BLE_ADDR_TYPE_PUBLIC,
				    .filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
				    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
				    .cfg_mask = ESP_BLE_GAP_EXT_SCAN_CFG_CODE_MASK, // | ESP_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK,
				    .uncoded_cfg = {BLE_SCAN_TYPE_PASSIVE, 40, 40},
				    .coded_cfg = {BLE_SCAN_TYPE_PASSIVE, 40, 40},
				};
#endif
				uint8_t flg = read_buf[1];
				if(flg & 3) {
					uint16_t tdw = read_buf[2] | (read_buf[3] << 8);
					if(tdw < 10)
						tdw = 10;
					ext_scan_params.cfg_mask = flg & 3; // ESP_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK, ESP_BLE_GAP_EXT_SCAN_CFG_CODE_MASK
					/*
					BLE_ADDR_TYPE_PUBLIC        = 0x00,     // Public Device Address
					BLE_ADDR_TYPE_RANDOM        = 0x01,     // Random Device Address. To set this address, use the function esp_ble_gap_set_rand_addr(esp_bd_addr_t rand_addr)
				    BLE_ADDR_TYPE_RPA_PUBLIC    = 0x02,     // Resolvable Private Address (RPA) with public identity address
				    BLE_ADDR_TYPE_RPA_RANDOM    = 0x03      // Resolvable Private Address (RPA) with random identity address. To set this address, use the function esp_ble_gap_set_rand_addr(esp_bd_addr_t rand_addr)
					*/
					ext_scan_params.own_addr_type = flg >> 6;
					ext_scan_params.uncoded_cfg.scan_type = (flg >> 2) & 1; // =0 Passive scan, =1 Active scan
					ext_scan_params.coded_cfg.scan_type  = (flg >> 2) & 1; // =0 Passive scan, =1 Active scan
					ext_scan_params.uncoded_cfg.scan_interval = tdw;
					ext_scan_params.uncoded_cfg.scan_window = tdw;
					ext_scan_params.coded_cfg.scan_interval  = tdw;
					ext_scan_params.coded_cfg.scan_window  = tdw;
					mac_list.filtr = (flg >> 4) & 3;
					esp_err_t scan_ret = esp_ble_gap_set_ext_scan_params(&ext_scan_params);
					if(scan_ret) {
						ESP_LOGE(GATTC_TAG, "set extend scan params error, error code = %x", scan_ret);
					}
		        }
				send_resp(cmd, mac_list.count, &read_buf[1], 3);
			} else if(len >= 6 + 3) {
				if(read_buf[0] == CMD_ID_WMAC) {
					mac_list.mode = WHITE_LIST;
					if(mac_list.count < MAC_MAX_SCAN_LIST) {
						//memcpy(&mac_list.mac[mac_list.count], &read_buf[1], 6);
						mac_list.mac[mac_list.count][0] = read_buf[6];
						mac_list.mac[mac_list.count][1] = read_buf[5];
						mac_list.mac[mac_list.count][2] = read_buf[4];
						mac_list.mac[mac_list.count][3] = read_buf[3];
						mac_list.mac[mac_list.count][4] = read_buf[2];
						mac_list.mac[mac_list.count][5] = read_buf[1];
						mac_list.count++;
					}
					send_resp(cmd, mac_list.count, &read_buf[1], 6);
				} else if(read_buf[0] == CMD_ID_BMAC) {
					mac_list.mode = BALCK_LIST;
					if(mac_list.count < MAC_MAX_SCAN_LIST) {
						//memcpy(&mac_list.mac[mac_list.count], &read_buf[1], 6);
						mac_list.mac[mac_list.count][0] = read_buf[6];
						mac_list.mac[mac_list.count][1] = read_buf[5];
						mac_list.mac[mac_list.count][2] = read_buf[4];
						mac_list.mac[mac_list.count][3] = read_buf[3];
						mac_list.mac[mac_list.count][4] = read_buf[2];
						mac_list.mac[mac_list.count][5] = read_buf[1];
						mac_list.count++;
					}
					send_resp(cmd, mac_list.count, &read_buf[1], 6);
				}
			} else if(len >= 3) {
				if(read_buf[0] == CMD_ID_CLRM)  {
					mac_list.count = 0;
					send_resp(cmd, MAC_MAX_SCAN_LIST, &read_buf[1], 0);
				}
				else if(read_buf[0] == CMD_ID_INFO) {
					esp_read_mac(read_buf, ESP_MAC_BT);
					send_resp(cmd, SW_VERSION, read_buf, 6);
				}
			}
		}
	}
}

void app_main(void)
{

    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gap register error, error code = %x", __func__, ret);
        return;
    }
    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register error, error code = %x", __func__, ret);
        return;
    }
    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register error, error code = %x", __func__, ret);
    }
    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
    }

#if USE_CONNECT

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
#endif

    crcInit();

	xTaskCreate(io_task, "io_task", IO_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
//	              2 | portPRIVILEGE_BIT, NULL);

}

