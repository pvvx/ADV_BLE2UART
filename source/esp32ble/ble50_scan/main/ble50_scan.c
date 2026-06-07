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
#include <stdarg.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/usb_serial_jtag.h"
#include "hal/usb_serial_jtag_ll.h"
#include "rgb_led.h"


#ifndef BOARD_LED_GPIO
#ifdef CONFIG_BOARD_LED_GPIO
#define BOARD_LED_GPIO          ((gpio_num_t)CONFIG_BOARD_LED_GPIO)
#else
#define BOARD_LED_GPIO          GPIO_NUM_8
#endif
#endif

#define SW_VERSION              0x02
#ifdef CONFIG_BOARD_HW_VERSION
#define HW_VERSION              CONFIG_BOARD_HW_VERSION
#else
#define HW_VERSION              0xC3
#endif

#define HEAD_CRC_ADD_LEN        13
#define FRAME_DATA_LEN          6
#define CMD_BUFFER_SIZE         64
#define DEBUG_PAYLOAD_MAX_LEN   96

#define EXTADV_RPT_DATA_LEN_MAX 229 // //253 - 24 = 229

#define IN_BUF_SIZE             64

#define OUT_BUF_SIZE            (EXTADV_RPT_DATA_LEN_MAX + HEAD_CRC_ADD_LEN)

#define IO_TASK_STACK_SIZE      4096

#define UART_BAUD_RATE_COUNT    3
#define FILTER_LIST_CAPACITY    64
#define CODED_SCAN_UNIT_10MS    16
#ifdef CONFIG_BOARD_LED_IDLE_LEVEL
#define BOARD_LED_IDLE_LEVEL    CONFIG_BOARD_LED_IDLE_LEVEL
#else
#define BOARD_LED_IDLE_LEVEL    1
#endif
#ifdef CONFIG_BOARD_LED_PULSE_US
#define BOARD_LED_PULSE_US      CONFIG_BOARD_LED_PULSE_US
#else
#define BOARD_LED_PULSE_US      25000
#endif

// Hardware PWM (LEDC) backing for the board LED (PWM type only).
// For RGB type the LED is driven via RMT (see rgb_led.c).
#ifndef CONFIG_BOARD_LED_TYPE_RGB
#define LED_PWM_FREQ_HZ         5000
#define LED_PWM_TIMER           LEDC_TIMER_0
#define LED_PWM_MODE            LEDC_LOW_SPEED_MODE
#define LED_PWM_CHANNEL         LEDC_CHANNEL_0
#define LED_PWM_RES_BITS        LEDC_TIMER_8_BIT
#define LED_DUTY_FULL           (1u << 8)   // 256 — "always HIGH" sentinel
#define LED_DUTY_OFF            LED_DUTY_FULL
#define LED_DUTY_DIM            248u   // ~3% on time — faint blink for 1M / legacy / 2M adv
#define LED_DUTY_BRIGHT         0u     // 100% — max brightness, used for Coded PHY adv
#endif

#define GPIO_OP_ANALOG_READ     7
#define TXADV_INSTANCE          0
#define TXADV_DATA_MAX_LEN      31
#define GAP_SYNC_TIMEOUT_MS     1000

#define GPIO_EVENT_QUEUE_LEN    16

enum {
    CMD_ID_INFO     = 0x00,
    CMD_ID_SCAN     = 0x01,
    CMD_ID_WMAC     = 0x02,
    CMD_ID_BMAC     = 0x03,
    CMD_ID_CLRM     = 0x04,
    CMD_ID_PRNT     = 0x05,
    CMD_ID_GPIO     = 0x06,
    CMD_ID_UART     = 0x08,
    CMD_ID_RFSDK    = 0x09,
    CMD_ID_VERSION  = 0x0A,
    CMD_ID_TXADV    = 0x0B,
    CMD_ID_CONN     = 0x0C,
    CMD_ID_TXDATA   = 0x0D,
    CMD_ID_RXDATA   = 0x0E,
    CMD_ID_VBAT     = 0x0F,
    CMD_ID_GPIOEVT  = 0x10, // GPIO edge events: enable/disable + spontaneous notifications
} CMD_ID_KEYS;

// CMD_ID_GPIOEVT sub-operations sent by the host.
enum {
    GPIOEVT_OP_QUERY   = 0,  // returns the bitmap of currently-armed pins
    GPIOEVT_OP_ENABLE  = 1,  // arm ISR on pin (both edges)
    GPIOEVT_OP_DISABLE = 2,  // disarm
    GPIOEVT_OP_CLEAR   = 3,  // disarm every pin
};

enum {
    CMD_STATUS_OK     = 0,
    CMD_STATUS_ARGS   = 1,
    CMD_STATUS_PIN    = 2,
    CMD_STATUS_DENIED = 3,
    CMD_STATUS_VALUE  = 4,
};

typedef struct {
    uint8_t len;
    uint8_t bytes[6];
} mac_filter_entry_t;

typedef struct {
    uint8_t count;
    mac_filter_entry_t entry[FILTER_LIST_CAPACITY];
} mac_filter_list_t;

typedef struct {
    mac_filter_list_t white_list;
    mac_filter_list_t black_list;
    uint8_t addr_filter_mask;
    uint8_t uart_baud_index;
    uint8_t power_index;
    uint8_t xtal_cap;
    uint8_t coded_min_units;
    uint8_t scan_channels[3];
    uint16_t scan_window_1m_units;
    uint16_t scan_window_coded_units;
    uint8_t scan_mode;
    bool scan_running;
} scanner_state_t;

typedef struct {
    bool running;
    uint8_t phy;
    uint16_t interval_units;
    uint8_t adv_len;
    uint8_t adv_data[TXADV_DATA_MAX_LEN];
} txadv_state_t;

enum {
    CONN_STATE_IDLE = 0,
    CONN_STATE_CONNECTING = 1,
    CONN_STATE_CONNECTED = 2,
};

typedef struct {
    uint8_t state;
    uint8_t peer_addr_type;
    uint8_t requested_phy;
    uint16_t interval_units;
    esp_bd_addr_t peer_addr;
} conn_runtime_t;

static const uint32_t s_uart_baud_rates[UART_BAUD_RATE_COUNT] = {
    2000000,
    921600,
    115200,
};

static scanner_state_t s_state = {
    .addr_filter_mask = 0,
    .uart_baud_index = 0,
    .power_index = ESP_PWR_LVL_P3,
    .xtal_cap = 0xFF,
    .coded_min_units = 3,
    .scan_channels = {37, 38, 39},
    .scan_window_1m_units = 48,
    .scan_window_coded_units = 48,
    .scan_mode = 0x03,
    .scan_running = false,
};

static txadv_state_t s_txadv = {0};
static conn_runtime_t s_conn = {
    .state = CONN_STATE_IDLE,
};

static esp_timer_handle_t s_board_led_timer = NULL;
static volatile uint8_t s_board_led_idle_level = BOARD_LED_IDLE_LEVEL;
static bool s_rgb_led_ready = false;
static bool s_rgb_led_on = false;   // tracks whether user has turned the RGB LED on via GPIO write
static SemaphoreHandle_t s_rgb_mutex = NULL;  // protects rgb_led_set/off from concurrent tasks
static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static SemaphoreHandle_t s_gap_sync_sem = NULL;
static volatile esp_bt_status_t s_gap_sync_status = ESP_BT_STATUS_SUCCESS;

#define USE_TXT_OUT 0
#define USE_CONNECT 1

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
static void stop_scan_if_running(void);
#if USE_CONNECT
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void clear_conn_runtime(void);
static void send_conn_response(uint8_t status);
static void send_rxdata_response(bool is_notify, uint16_t att_handle, const uint8_t *value, uint16_t value_len);
static bool start_conn_request(uint8_t phy, uint8_t peer_addr_type, const uint8_t *peer_addr);

static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static bool connect = false;
static bool get_service = false;
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
    .cfg_mask = ESP_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK | ESP_BLE_GAP_EXT_SCAN_CFG_CODE_MASK,
    .uncoded_cfg = {BLE_SCAN_TYPE_PASSIVE, 48, 48},
    .coded_cfg = {BLE_SCAN_TYPE_PASSIVE, 48, 48},
};

#if USE_CONNECT
const esp_ble_conn_params_t phy_1m_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320,
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len  = 0,
    .max_ce_len = 0,
};
const esp_ble_conn_params_t phy_2m_conn_params = {
    .scan_interval = 0x40,
    .scan_window = 0x40,
    .interval_min = 320,
    .interval_max = 320,
    .latency = 0,
    .supervision_timeout = 600,
    .min_ce_len  = 0,
    .max_ce_len = 0,
};
const esp_ble_conn_params_t phy_coded_conn_params = {
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
            clear_conn_runtime();
            send_conn_response(CMD_STATUS_DENIED);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        s_conn.state = CONN_STATE_CONNECTED;
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;
        gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = INVALID_HANDLE;
        gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = INVALID_HANDLE;
        gl_profile_tab[PROFILE_A_APP_ID].notify_char_handle = INVALID_HANDLE;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        ESP_LOG_BUFFER_HEX(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        send_conn_response(CMD_STATUS_OK);
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
            esp_err_t search_ret = esp_ble_gattc_search_service(gattc_if, p_data->open.conn_id, &remote_filter_service_uuid);
            if (search_ret != ESP_OK) {
                ESP_LOGE(GATTC_TAG, "search service request failed, error code = %x", search_ret);
            }
        }
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
            send_conn_response(CMD_STATUS_OK);
            break;
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_err_t search_ret = esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        if (search_ret != ESP_OK) {
            ESP_LOGE(GATTC_TAG, "search service request failed, error code = %x", search_ret);
        }
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
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
                            if (char_elem_result[i].uuid.len == ESP_UUID_LEN_16
                                && char_elem_result[i].uuid.uuid.uuid16 == REMOTE_NOTIFY_UUID
                                && (char_elem_result[i].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
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

        send_conn_response(CMD_STATUS_OK);

        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg for notify failed, error status = %x", p_data->reg_for_notify.status);
            send_conn_response(CMD_STATUS_DENIED);
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

        send_conn_response(CMD_STATUS_OK);

        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        ESP_LOG_BUFFER_HEX(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        send_rxdata_response(p_data->notify.is_notify, p_data->notify.handle,
                             p_data->notify.value, p_data->notify.value_len);
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
        ESP_LOG_BUFFER_HEX(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Write char success ");
        break;
    case ESP_GATTC_CANCEL_OPEN_EVT:
        if (p_data->cancel_open.status == ESP_GATT_OK) {
            clear_conn_runtime();
            send_conn_response(CMD_STATUS_OK);
        } else {
            ESP_LOGE(GATTC_TAG, "cancel open failed, error status = %x", p_data->cancel_open.status);
            send_conn_response(CMD_STATUS_DENIED);
        }
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = 0x%x", p_data->disconnect.reason);
        clear_conn_runtime();
        send_conn_response(CMD_STATUS_OK);
        break;
#endif
    default:
        break;
    }
}


//#define MYFIFO_BLK_SIZE		(EXTADV_RPT_DATA_LEN_MAX + HEAD_CRC_ADD_LEN) // 229+12 = 241 bytes
//MYFIFO_INIT(ad_fifo, MYFIFO_BLK_SIZE, 4); 	// (229+12)*4 = 964 bytes + sizeof(my_fifo_t)
static void flush_serial_frame(const uint8_t *frame, size_t len)
{
    usb_serial_jtag_write_bytes(frame, len, 5 / portTICK_PERIOD_MS);
    usb_serial_jtag_ll_txfifo_flush();
}

static void send_frame(uint8_t payload_len,
                       uint8_t byte1,
                       uint8_t byte2,
                       uint8_t byte3,
                       uint8_t byte4,
                       const uint8_t data_field[FRAME_DATA_LEN],
                       const uint8_t *payload)
{
    const size_t frame_len = HEAD_CRC_ADD_LEN + payload_len;
    uint8_t frame[HEAD_CRC_ADD_LEN + DEBUG_PAYLOAD_MAX_LEN];

    if (payload_len > DEBUG_PAYLOAD_MAX_LEN) {
        payload_len = DEBUG_PAYLOAD_MAX_LEN;
    }

    memset(frame, 0, sizeof(frame));
    frame[0] = payload_len;
    frame[1] = byte1;
    frame[2] = byte2;
    frame[3] = byte3;
    frame[4] = byte4;
    if (data_field != NULL) {
        memcpy(&frame[5], data_field, FRAME_DATA_LEN);
    }
    if (payload_len != 0 && payload != NULL) {
        memcpy(&frame[11], payload, payload_len);
    }

    uint16_t crc = crcFast(frame, frame_len - 2);
    frame[frame_len - 2] = crc & 0xFF;
    frame[frame_len - 1] = crc >> 8;
    flush_serial_frame(frame, frame_len);
}

static void send_cmd_response(uint8_t cmd, uint8_t id, const uint8_t *data, uint8_t len)
{
    uint8_t data_field[FRAME_DATA_LEN] = {0};

    if (data != NULL && len != 0) {
        memcpy(data_field, data, len > FRAME_DATA_LEN ? FRAME_DATA_LEN : len);
    }
    send_frame(0, cmd, id, len, 0xFF, data_field, NULL);
}

static void send_cmd_response_bytes(uint8_t cmd, uint8_t id, const uint8_t *data, uint8_t len)
{
    uint8_t data_field[FRAME_DATA_LEN] = {0};
    uint8_t effective_len = len;
    uint8_t payload_len = 0;

    if (effective_len > FRAME_DATA_LEN + DEBUG_PAYLOAD_MAX_LEN) {
        effective_len = FRAME_DATA_LEN + DEBUG_PAYLOAD_MAX_LEN;
    }
    if (data != NULL && effective_len != 0) {
        memcpy(data_field, data, effective_len > FRAME_DATA_LEN ? FRAME_DATA_LEN : effective_len);
    }
    if (effective_len > FRAME_DATA_LEN) {
        payload_len = effective_len - FRAME_DATA_LEN;
    }

    send_frame(payload_len, cmd, id, effective_len, 0xFF, data_field,
               payload_len != 0 ? data + FRAME_DATA_LEN : NULL);
}

static void clear_gap_sync_signal(void)
{
    if (s_gap_sync_sem == NULL) {
        return;
    }

    while (xSemaphoreTake(s_gap_sync_sem, 0) == pdTRUE) {
    }
}

static bool wait_gap_sync_signal(esp_bt_status_t *status)
{
    if (s_gap_sync_sem == NULL) {
        return false;
    }
    if (xSemaphoreTake(s_gap_sync_sem, pdMS_TO_TICKS(GAP_SYNC_TIMEOUT_MS)) != pdTRUE) {
        return false;
    }
    if (status != NULL) {
        *status = s_gap_sync_status;
    }
    return true;
}

static void send_debug_print(const char *fmt, ...)
{
    uint8_t data_field[FRAME_DATA_LEN] = {0};
    char payload[DEBUG_PAYLOAD_MAX_LEN + 1];
    va_list args;

    va_start(args, fmt);
    int written = vsnprintf(payload, sizeof(payload), fmt, args);
    va_end(args);
    if (written <= 0) {
        return;
    }

    if (written > DEBUG_PAYLOAD_MAX_LEN) {
        written = DEBUG_PAYLOAD_MAX_LEN;
    }
    send_frame((uint8_t)written, CMD_ID_PRNT, 0xFF, 0xFF, 0xFF, data_field, (const uint8_t *)payload);
}

static void clear_txadv_state(void)
{
    s_txadv.running = false;
    s_txadv.phy = 0;
    s_txadv.interval_units = 0;
    s_txadv.adv_len = 0;
    memset(s_txadv.adv_data, 0, sizeof(s_txadv.adv_data));
}

static void fill_txadv_response(uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    if (s_txadv.running) {
        resp[0] = s_txadv.phy + 1;
    }
    resp[1] = s_txadv.interval_units & 0xFF;
    resp[2] = s_txadv.interval_units >> 8;
    resp[3] = s_txadv.adv_len;
}

static esp_ble_gap_ext_adv_params_t txadv_params_for(uint8_t phy, uint16_t interval_units)
{
    esp_ble_gap_ext_adv_params_t params = {
        .interval_min = interval_units,
        .interval_max = interval_units,
        .channel_map = ADV_CHNL_ALL,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
        .max_skip = 0,
        .sid = TXADV_INSTANCE,
        .scan_req_notif = false,
    };

    switch (phy) {
    case 0:
        params.type = ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_NONCONN;
        params.primary_phy = ESP_BLE_GAP_PRI_PHY_1M;
        params.secondary_phy = ESP_BLE_GAP_PHY_1M;
        break;
    case 1:
        params.type = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED;
        params.primary_phy = ESP_BLE_GAP_PRI_PHY_1M;
        params.secondary_phy = ESP_BLE_GAP_PHY_1M;
        break;
    case 2:
    default:
        params.type = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED;
        params.primary_phy = ESP_BLE_GAP_PRI_PHY_CODED;
        params.secondary_phy = ESP_BLE_GAP_PHY_CODED;
        break;
    }

    return params;
}

static bool stop_txadv_if_running(void)
{
    static const uint8_t txadv_instance = TXADV_INSTANCE;
    esp_bt_status_t status = ESP_BT_STATUS_FAIL;

    if (!s_txadv.running) {
        return true;
    }

    clear_gap_sync_signal();
    s_gap_sync_status = ESP_BT_STATUS_FAIL;
    if (esp_ble_gap_ext_adv_stop(1, &txadv_instance) != ESP_OK) {
        return false;
    }
    if (!wait_gap_sync_signal(&status) || status != ESP_BT_STATUS_SUCCESS) {
        return false;
    }

    clear_txadv_state();
    return true;
}

static bool start_txadv(uint8_t phy, uint16_t interval_units, const uint8_t *adv_data, uint8_t adv_len)
{
    static const esp_ble_gap_ext_adv_t txadv_enable = {
        .instance = TXADV_INSTANCE,
        .duration = 0,
        .max_events = 0,
    };
    esp_bt_status_t status = ESP_BT_STATUS_FAIL;
    esp_ble_gap_ext_adv_params_t params = txadv_params_for(phy, interval_units);

    if (!stop_txadv_if_running()) {
        return false;
    }

    clear_gap_sync_signal();
    s_gap_sync_status = ESP_BT_STATUS_FAIL;
    if (esp_ble_gap_ext_adv_set_params(TXADV_INSTANCE, &params) != ESP_OK) {
        return false;
    }
    if (!wait_gap_sync_signal(&status) || status != ESP_BT_STATUS_SUCCESS) {
        return false;
    }

    clear_gap_sync_signal();
    s_gap_sync_status = ESP_BT_STATUS_FAIL;
    if (esp_ble_gap_config_ext_adv_data_raw(TXADV_INSTANCE, adv_len, adv_len != 0 ? adv_data : NULL) != ESP_OK) {
        return false;
    }
    if (!wait_gap_sync_signal(&status) || status != ESP_BT_STATUS_SUCCESS) {
        return false;
    }

    clear_gap_sync_signal();
    s_gap_sync_status = ESP_BT_STATUS_FAIL;
    if (esp_ble_gap_ext_adv_start(1, &txadv_enable) != ESP_OK) {
        return false;
    }
    if (!wait_gap_sync_signal(&status) || status != ESP_BT_STATUS_SUCCESS) {
        return false;
    }

    s_txadv.running = true;
    s_txadv.phy = phy;
    s_txadv.interval_units = interval_units;
    s_txadv.adv_len = adv_len;
    memset(s_txadv.adv_data, 0, sizeof(s_txadv.adv_data));
    if (adv_len != 0) {
        memcpy(s_txadv.adv_data, adv_data, adv_len);
    }
    return true;
}

static void clear_conn_runtime(void)
{
    memset(&s_conn, 0, sizeof(s_conn));
    s_conn.state = CONN_STATE_IDLE;
    connect = false;
    get_service = false;
    gl_profile_tab[PROFILE_A_APP_ID].conn_id = 0;
    gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = INVALID_HANDLE;
    gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = INVALID_HANDLE;
    gl_profile_tab[PROFILE_A_APP_ID].notify_char_handle = INVALID_HANDLE;
    memset(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, 0, sizeof(esp_bd_addr_t));
}

static void fill_conn_response(uint8_t *resp)
{
    uint16_t handle = gl_profile_tab[PROFILE_A_APP_ID].notify_char_handle;

    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = s_conn.state;
    resp[1] = s_conn.peer_addr_type;
    resp[2] = handle & 0xFF;
    resp[3] = handle >> 8;
    resp[4] = s_conn.interval_units & 0xFF;
    resp[5] = s_conn.interval_units >> 8;
}

static void send_conn_response(uint8_t status)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};

    fill_conn_response(resp);
    send_cmd_response(CMD_ID_CONN, status, resp, FRAME_DATA_LEN);
}

static void send_rxdata_response(bool is_notify, uint16_t att_handle, const uint8_t *value, uint16_t value_len)
{
    uint8_t resp[FRAME_DATA_LEN + DEBUG_PAYLOAD_MAX_LEN] = {0};
    uint8_t opcode = is_notify ? 0x1B : 0x1D;
    uint8_t truncated_len = value_len;

    if (truncated_len > FRAME_DATA_LEN + DEBUG_PAYLOAD_MAX_LEN - 3) {
        truncated_len = FRAME_DATA_LEN + DEBUG_PAYLOAD_MAX_LEN - 3;
    }

    resp[0] = att_handle & 0xFF;
    resp[1] = att_handle >> 8;
    resp[2] = truncated_len;
    if (truncated_len != 0 && value != NULL) {
        memcpy(&resp[3], value, truncated_len);
    }

    send_cmd_response_bytes(CMD_ID_RXDATA, opcode, resp, 3 + truncated_len);
}

static bool start_conn_request(uint8_t phy, uint8_t peer_addr_type, const uint8_t *peer_addr)
{
    esp_ble_gatt_creat_conn_params_t create_conn = {
        .remote_addr_type = (esp_ble_addr_type_t)peer_addr_type,
        .is_direct = true,
        .is_aux = phy != 0,
        .own_addr_type = (esp_ble_addr_type_t)0xFF,
        .phy_mask = phy != 0 ? ESP_BLE_GAP_PHY_CODED_PREF_MASK : ESP_BLE_GAP_PHY_1M_PREF_MASK,
        .phy_1m_conn_params = &phy_1m_conn_params,
        .phy_2m_conn_params = NULL,
        .phy_coded_conn_params = &phy_coded_conn_params,
    };

    if (gl_profile_tab[PROFILE_A_APP_ID].gattc_if == ESP_GATT_IF_NONE || peer_addr == NULL) {
        return false;
    }

    clear_conn_runtime();
    s_conn.state = CONN_STATE_CONNECTING;
    s_conn.peer_addr_type = peer_addr_type;
    s_conn.requested_phy = phy;
    s_conn.interval_units = phy != 0 ? phy_coded_conn_params.interval_max : phy_1m_conn_params.interval_max;
    memcpy(s_conn.peer_addr, peer_addr, sizeof(esp_bd_addr_t));
    memcpy(create_conn.remote_bda, peer_addr, sizeof(esp_bd_addr_t));
    connect = true;

    stop_scan_if_running();
    return esp_ble_gattc_enh_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, &create_conn) == ESP_OK;
}

static inline bool is_led_pin(uint8_t pin_code)
{
    return pin_code == (uint8_t)BOARD_LED_GPIO;
}

static inline bool is_analog_gpio(uint8_t pin_code)
{
    switch (pin_code) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        return true;
    default:
        return false;
    }
}

static inline uint16_t board_mask(void)
{
    if ((uint8_t)BOARD_LED_GPIO < 16) {
        return (uint16_t)(1u << (uint8_t)BOARD_LED_GPIO);
    }
    return 0;
}

static bool is_supported_gpio(uint8_t pin_code);

static bool analog_channel_for_gpio(uint8_t pin_code, adc_channel_t *channel)
{
    if (channel == NULL) {
        return false;
    }

    switch (pin_code) {
    case 0:
        *channel = ADC_CHANNEL_0;
        return true;
    case 1:
        *channel = ADC_CHANNEL_1;
        return true;
    case 2:
        *channel = ADC_CHANNEL_2;
        return true;
    case 3:
        *channel = ADC_CHANNEL_3;
        return true;
    case 4:
        *channel = ADC_CHANNEL_4;
        return true;
    default:
        return false;
    }
}

static void init_adc_inputs(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    const uint8_t analog_pins[] = {0, 1, 2, 3, 4};

    if (adc_oneshot_new_unit(&init_config, &s_adc1_handle) != ESP_OK) {
        s_adc1_handle = NULL;
        ESP_LOGW(GATTC_TAG, "ADC oneshot init failed; analog GPIO reads disabled");
        return;
    }

    for (size_t index = 0; index < sizeof(analog_pins) / sizeof(analog_pins[0]); ++index) {
        adc_channel_t channel;

        if (!analog_channel_for_gpio(analog_pins[index], &channel)) {
            continue;
        }
        if (adc_oneshot_config_channel(s_adc1_handle, channel, &channel_config) != ESP_OK) {
            ESP_LOGW(GATTC_TAG, "ADC channel config failed for GPIO%u", analog_pins[index]);
        }
    }
}

static bool read_gpio_analog(uint8_t pin_code, uint16_t *raw_value)
{
    int raw = 0;
    adc_channel_t channel;

    if (raw_value == NULL || s_adc1_handle == NULL || !analog_channel_for_gpio(pin_code, &channel)) {
        return false;
    }
    if (adc_oneshot_read(s_adc1_handle, channel, &raw) != ESP_OK) {
        return false;
    }
    if (raw < 0) {
        raw = 0;
    }
    *raw_value = (uint16_t)raw;
    return true;
}

#ifndef CONFIG_BOARD_LED_TYPE_RGB
static inline void board_led_set_duty(uint32_t duty)
{
    ledc_set_duty(LED_PWM_MODE, LED_PWM_CHANNEL, duty);
    ledc_update_duty(LED_PWM_MODE, LED_PWM_CHANNEL);
}

static inline uint32_t board_led_idle_duty(void)
{
    return s_board_led_idle_level ? LED_DUTY_OFF : LED_DUTY_BRIGHT;
}

static void restore_board_led_idle_level(void *arg)
{
    (void)arg;
    board_led_set_duty(board_led_idle_duty());
}

static void set_board_led_idle_level(uint8_t level)
{
    s_board_led_idle_level = level ? 1 : 0;
    if (s_board_led_timer != NULL) {
        esp_timer_stop(s_board_led_timer);
    }
    board_led_set_duty(board_led_idle_duty());
}
#else /* RGB LED version */
/* Forward declarations — defined below after set_board_led_idle_level. */
static inline void safe_rgb_led_set(uint8_t r, uint8_t g, uint8_t b);
static inline void safe_rgb_led_off(void);

static void set_board_led_idle_level(uint8_t level)
{
    s_board_led_idle_level = level ? 1 : 0;
    if (s_board_led_timer != NULL) {
        esp_timer_stop(s_board_led_timer);
    }
    if (!s_rgb_led_on) {
        if (level) {
            safe_rgb_led_off();
        } else {
            safe_rgb_led_set(0, 0, 32); // dim blue when idle-on
        }
    }
}

/* Lock around rgb_led_set / rgb_led_off because these call RMT functions
 * that are NOT thread-safe.  Both the BLE callback (esp_gap_cb) and the
 * io_task can call indicate_board_activity / GPIO-handler code paths. */
static inline void safe_rgb_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_rgb_led_ready) return;
    if (s_rgb_mutex != NULL && xSemaphoreTake(s_rgb_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        rgb_led_set(r, g, b);
        xSemaphoreGive(s_rgb_mutex);
    }
}
static inline void safe_rgb_led_off(void)
{
    safe_rgb_led_set(0, 0, 0);
}

static void rgb_led_timer_cb(void *arg)
{
    (void)arg;
    safe_rgb_led_off();
}
#endif

// Pulse the LED to indicate adv reception. When the host has configured the
// LED to be off in idle (idle_level == 1, the default), the pulse uses two
// brightness levels chosen by primary PHY:
//   - Coded PHY (primary_phy == 3): LED_DUTY_BRIGHT (full intensity)
//   - everything else (1M, 2M, legacy):     LED_DUTY_DIM    (~10% intensity)
// When the user has forced the LED on in idle (idle_level == 0), the pulse
// briefly dims to LED_DUTY_DIM regardless of PHY so the activity is still
// visible against the bright background.
static void indicate_board_activity(uint8_t primary_phy)
{
#ifdef CONFIG_BOARD_LED_TYPE_RGB
    if (s_rgb_led_ready) {
        if (primary_phy == 3) {
            safe_rgb_led_set(0, 32, 0);   // green — Coded PHY
        } else {
            safe_rgb_led_set(0, 0, 32);   // blue  — 1M / legacy
        }
        if (s_board_led_timer != NULL) {
            esp_timer_stop(s_board_led_timer);
            esp_timer_start_once(s_board_led_timer, BOARD_LED_PULSE_US);
        }
    }
#else
    uint32_t pulse_duty;
    if (s_board_led_idle_level == 0) {
        pulse_duty = LED_DUTY_DIM;
    } else {
        pulse_duty = (primary_phy == 3) ? LED_DUTY_BRIGHT : LED_DUTY_DIM;
    }
    board_led_set_duty(pulse_duty);
    if (s_board_led_timer != NULL) {
        esp_timer_stop(s_board_led_timer);
        esp_timer_start_once(s_board_led_timer, BOARD_LED_PULSE_US);
    }
#endif
}

#ifndef CONFIG_BOARD_LED_TYPE_RGB
static void init_board_led_pwm(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LED_PWM_MODE,
        .timer_num = LED_PWM_TIMER,
        .duty_resolution = LED_PWM_RES_BITS,
        .freq_hz = LED_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t ch_cfg = {
        .speed_mode = LED_PWM_MODE,
        .channel = LED_PWM_CHANNEL,
        .timer_sel = LED_PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BOARD_LED_GPIO,
        .duty = LED_DUTY_OFF,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
}
#endif /* !CONFIG_BOARD_LED_TYPE_RGB (init_board_led_pwm) */

static bool is_supported_gpio(uint8_t pin_code)
{
    switch (pin_code) {
#ifdef CONFIG_BOARD_GPIO_0
    case 0:
#endif
#ifdef CONFIG_BOARD_GPIO_1
    case 1:
#endif
#ifdef CONFIG_BOARD_GPIO_2
    case 2:
#endif
#ifdef CONFIG_BOARD_GPIO_3
    case 3:
#endif
#ifdef CONFIG_BOARD_GPIO_4
    case 4:
#endif
#ifdef CONFIG_BOARD_GPIO_5
    case 5:
#endif
#ifdef CONFIG_BOARD_GPIO_6
    case 6:
#endif
#ifdef CONFIG_BOARD_GPIO_7
    case 7:
#endif
#ifdef CONFIG_BOARD_GPIO_8
    case 8:
#endif
#ifdef CONFIG_BOARD_GPIO_9
    case 9:
#endif
#ifdef CONFIG_BOARD_GPIO_10
    case 10:
#endif
#ifdef CONFIG_BOARD_GPIO_12
    case 12:
#endif
#ifdef CONFIG_BOARD_GPIO_13
    case 13:
#endif
#ifdef CONFIG_BOARD_GPIO_14
    case 14:
#endif
#ifdef CONFIG_BOARD_GPIO_15
    case 15:
#endif
#ifdef CONFIG_BOARD_GPIO_18
    case 18:
#endif
#ifdef CONFIG_BOARD_GPIO_19
    case 19:
#endif
#ifdef CONFIG_BOARD_GPIO_20
    case 20:
#endif
#ifdef CONFIG_BOARD_GPIO_21
    case 21:
#endif
#ifdef CONFIG_BOARD_GPIO_22
    case 22:
#endif
#ifdef CONFIG_BOARD_GPIO_23
    case 23:
#endif
        return true;
    default:
        return false;
    }
}

static bool apply_gpio_config(uint8_t pin_code, uint8_t input_en, uint8_t output_en, uint8_t pull)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(pin_code),
        .mode = GPIO_MODE_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (!is_supported_gpio(pin_code)) {
        return false;
    }

    if (input_en && output_en) {
        io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    } else if (output_en) {
        io_conf.mode = GPIO_MODE_OUTPUT;
    } else if (input_en) {
        io_conf.mode = GPIO_MODE_INPUT;
    }

    switch (pull) {
    case 1:
    case 3:
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        break;
    case 2:
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        break;
    default:
        break;
    }

    return gpio_config(&io_conf) == ESP_OK;
}

static uint16_t scan_coded_floor_units(void)
{
    return (uint16_t)s_state.coded_min_units * CODED_SCAN_UNIT_10MS;
}

static uint16_t clamp_scan_window_units(uint16_t units)
{
    return units < 10 ? 10 : units;
}

static uint8_t total_filter_count(void)
{
    return s_state.white_list.count + s_state.black_list.count;
}

static void clear_filter_lists(void)
{
    memset(&s_state.white_list, 0, sizeof(s_state.white_list));
    memset(&s_state.black_list, 0, sizeof(s_state.black_list));
}

static uint8_t add_filter_entry(mac_filter_list_t *list, const uint8_t *wire_mac, size_t wire_len)
{
    mac_filter_entry_t *entry;

    if (wire_len == 0 || wire_len > FRAME_DATA_LEN || list->count >= FILTER_LIST_CAPACITY) {
        return list->count;
    }

    entry = &list->entry[list->count++];
    entry->len = wire_len;
    memset(entry->bytes, 0, sizeof(entry->bytes));
    for (size_t i = 0; i < wire_len; ++i) {
        entry->bytes[i] = wire_mac[wire_len - 1 - i];
    }
    return list->count;
}

static bool filter_list_matches(const mac_filter_list_t *list, const uint8_t *mac)
{
    for (size_t i = 0; i < list->count; ++i) {
        if (memcmp(mac, list->entry[i].bytes, list->entry[i].len) == 0) {
            return true;
        }
    }
    return false;
}

static bool chk_mac(const uint8_t *mac)
{
    if (s_state.white_list.count != 0 && !filter_list_matches(&s_state.white_list, mac)) {
        return false;
    }
    if (s_state.black_list.count != 0 && filter_list_matches(&s_state.black_list, mac)) {
        return false;
    }
    return true;
}

static void refresh_scan_params(void)
{
    const uint8_t flg = s_state.scan_mode;
    uint16_t window_1m = clamp_scan_window_units(s_state.scan_window_1m_units);
    uint16_t window_coded = clamp_scan_window_units(s_state.scan_window_coded_units);
    uint16_t coded_floor = scan_coded_floor_units();
    if (window_coded < coded_floor) {
        window_coded = coded_floor;
    }

    ext_scan_params.cfg_mask = flg & 0x03;
    ext_scan_params.own_addr_type = (flg >> 6) & 0x03;
    ext_scan_params.scan_duplicate = ((flg >> 3) & 0x01) ? BLE_SCAN_DUPLICATE_ENABLE : BLE_SCAN_DUPLICATE_DISABLE;
    ext_scan_params.uncoded_cfg.scan_type = ((flg >> 2) & 0x01) ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE;
    ext_scan_params.coded_cfg.scan_type = ext_scan_params.uncoded_cfg.scan_type;
    ext_scan_params.uncoded_cfg.scan_interval = window_1m;
    ext_scan_params.uncoded_cfg.scan_window = window_1m;
    ext_scan_params.coded_cfg.scan_interval = window_coded;
    ext_scan_params.coded_cfg.scan_window = window_coded;

    s_state.scan_window_1m_units = window_1m;
    s_state.scan_window_coded_units = window_coded;
    s_state.addr_filter_mask = (flg >> 4) & 0x03;
}

static void stop_scan_if_running(void)
{
    if (s_state.scan_running) {
        esp_ble_gap_stop_ext_scan();
        s_state.scan_running = false;
    }
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
        if ((s_state.scan_mode & 0x03) != 0) {
            esp_ble_gap_start_ext_scan(EXT_SCAN_DURATION, EXT_SCAN_PERIOD);
        }
        break;
    }
    case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
        if (param->ext_scan_start.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        s_state.scan_running = true;
        ESP_LOGI(GATTC_TAG, "Scan start success");
        break;
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        s_gap_sync_status = param->ext_adv_set_params.status;
        if (s_gap_sync_sem != NULL) {
            xSemaphoreGive(s_gap_sync_sem);
        }
        break;
    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        s_gap_sync_status = param->ext_adv_data_set.status;
        if (s_gap_sync_sem != NULL) {
            xSemaphoreGive(s_gap_sync_sem);
        }
        break;
    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        s_gap_sync_status = param->ext_adv_start.status;
        if (s_gap_sync_sem != NULL) {
            xSemaphoreGive(s_gap_sync_sem);
        }
        break;
    case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
        s_gap_sync_status = param->ext_adv_stop.status;
        if (s_gap_sync_sem != NULL) {
            xSemaphoreGive(s_gap_sync_sem);
        }
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
			&& (param->ext_adv_report.params.addr_type & s_state.addr_filter_mask) == 0) {
            out_buf[0] = len;
            out_buf[1] = param->ext_adv_report.params.rssi;
            out_buf[2] = param->ext_adv_report.params.event_type;
            out_buf[3] = (param->ext_adv_report.params.addr_type & 0x0f) | (param->ext_adv_report.params.dir_addr_type << 4);
            out_buf[4] = param->ext_adv_report.params.primary_phy | (param->ext_adv_report.params.secondly_phy << 4);
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
                indicate_board_activity(param->ext_adv_report.params.primary_phy);
        }
#endif
        break;
    }
    case ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT:
        if (param->ext_scan_stop.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "extend Scan stop failed, error status = %x", param->ext_scan_stop.status);
            break;
        }
        s_state.scan_running = false;
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

static uint8_t read_buf[CMD_BUFFER_SIZE];

static uint8_t current_power_index(void)
{
    esp_power_level_t level = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN);
    if (level != ESP_PWR_LVL_INVALID) {
        s_state.power_index = (uint8_t)level;
    }
    return s_state.power_index;
}

static void fill_gpio_response(uint8_t op, uint8_t pin_code, uint8_t *resp)
{
    uint16_t mask = board_mask();

    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = op;
    resp[1] = pin_code;
    if (is_supported_gpio(pin_code)) {
        if (pin_code == (uint8_t)BOARD_LED_GPIO) {
#ifdef CONFIG_BOARD_LED_TYPE_RGB
            resp[2] = s_rgb_led_on ? 1 : 0;
#else
            resp[2] = s_board_led_idle_level ? 1 : 0;
#endif
        } else {
            resp[2] = gpio_get_level((gpio_num_t)pin_code) ? 1 : 0;
        }
    }
    resp[3] = is_led_pin(pin_code) ? 1 : 0;
    resp[4] = mask & 0xFF;
    resp[5] = (mask >> 8) & 0xFF;
}

static void fill_gpio_analog_response(uint8_t pin_code, uint16_t raw_value, uint8_t *resp)
{
    uint16_t mask = board_mask();

    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = GPIO_OP_ANALOG_READ;
    resp[1] = pin_code;
    resp[2] = raw_value & 0xFF;
    resp[3] = raw_value >> 8;
    resp[4] = mask & 0xFF;
    resp[5] = (mask >> 8) & 0xFF;
}

static void fill_uart_response(uint8_t op, uint8_t *resp)
{
    uint32_t baud = s_uart_baud_rates[s_state.uart_baud_index];

    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = op;
    resp[1] = s_state.uart_baud_index;
    resp[2] = UART_BAUD_RATE_COUNT;
    resp[3] = baud & 0xFF;
    resp[4] = (baud >> 8) & 0xFF;
    resp[5] = (baud >> 16) & 0xFF;
}

static void fill_rfsdk_response(uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = current_power_index();
    resp[1] = s_state.xtal_cap;
    resp[2] = s_state.scan_channels[0];
    resp[3] = s_state.scan_channels[1];
    resp[4] = s_state.scan_channels[2];
    resp[5] = s_state.coded_min_units;
}

static void fill_version_response(uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = HW_VERSION;
    resp[1] = 0x00;
    resp[2] = 0x01;
    resp[3] = ESP_IDF_VERSION_MAJOR;
    resp[4] = ESP_IDF_VERSION_MINOR;
    resp[5] = ESP_IDF_VERSION_PATCH;
}

static void handle_scan_command(const uint8_t *cmd_buf, int len)
{
    uint8_t echo[FRAME_DATA_LEN] = {0};
    const int data_len = len - 3;
    uint8_t flags;

    if (data_len != 3 && data_len != 5) {
        send_cmd_response(CMD_ID_SCAN, total_filter_count(), NULL, 0);
        return;
    }

    flags = cmd_buf[1];
    memcpy(echo, &cmd_buf[1], data_len > FRAME_DATA_LEN ? FRAME_DATA_LEN : data_len);
    stop_scan_if_running();
    s_state.scan_mode = flags;

    if ((flags & 0x03) != 0) {
        s_state.scan_window_1m_units = clamp_scan_window_units((uint16_t)(cmd_buf[2] | (cmd_buf[3] << 8)));
        if (data_len == 5) {
            uint16_t coded = (uint16_t)(cmd_buf[4] | (cmd_buf[5] << 8));
            s_state.scan_window_coded_units = coded == 0 ? s_state.scan_window_1m_units : clamp_scan_window_units(coded);
        } else {
            s_state.scan_window_coded_units = s_state.scan_window_1m_units;
        }

        refresh_scan_params();
        esp_err_t scan_ret = esp_ble_gap_set_ext_scan_params(&ext_scan_params);
        if (scan_ret != ESP_OK) {
            ESP_LOGE(GATTC_TAG, "set extend scan params error, error code = %x", scan_ret);
            send_debug_print("scan params error: 0x%x", scan_ret);
        }
    } else {
        s_state.scan_running = false;
    }

    send_cmd_response(CMD_ID_SCAN, total_filter_count(), echo, data_len);
}

static void handle_mac_list_command(uint8_t cmd, const uint8_t *cmd_buf, int len)
{
    mac_filter_list_t *list = cmd == CMD_ID_WMAC ? &s_state.white_list : &s_state.black_list;
    const int mac_len = len - 3;
    uint8_t echo[FRAME_DATA_LEN] = {0};

    if (mac_len <= 0 || mac_len > FRAME_DATA_LEN) {
        send_cmd_response(cmd, list->count, NULL, 0);
        return;
    }

    memcpy(echo, &cmd_buf[1], mac_len);
    send_cmd_response(cmd, add_filter_entry(list, &cmd_buf[1], mac_len), echo, FRAME_DATA_LEN);
}

// ---------- GPIO event subsystem (CMD_ID_GPIOEVT) -------------------------
//
// Hardware-driven GPIO edge notifications. The host arms a pin via
// CMD_ID_GPIOEVT op=1; from that moment every rising/falling edge on the pin
// triggers an ISR that posts a (pin, level, timestamp) tuple to a FreeRTOS
// queue. The io_task drains that queue once per loop iteration and emits a
// CMD_ID_GPIOEVT response frame for each event — same wire format as other
// command responses (the host distinguishes "spontaneous event" by the cmd
// byte being CMD_ID_GPIOEVT and the high bit of the index field meaning
// "this is an event, not an op echo"). The high bit is set on events so the
// host parser cannot confuse a level value with a request status code.

typedef struct {
    uint8_t  pin;
    uint8_t  level;
    uint32_t timestamp_ms;
} gpio_event_msg_t;

static QueueHandle_t s_gpio_event_queue = NULL;
static bool s_gpio_isr_service_installed = false;
static uint32_t s_gpio_event_enabled_mask = 0;   // bitmap, one bit per pin id (0..31)

#define GPIOEVT_EVENT_FLAG  0x80u   // set in the "id" field of event frames

static void IRAM_ATTR gpio_event_isr(void *arg)
{
    if (s_gpio_event_queue == NULL) {
        return;
    }
    uint8_t pin = (uint8_t)(uintptr_t)arg;
    gpio_event_msg_t msg = {
        .pin = pin,
        .level = gpio_get_level((gpio_num_t)pin) ? 1u : 0u,
        .timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000),
    };
    BaseType_t hp_woken = pdFALSE;
    xQueueSendFromISR(s_gpio_event_queue, &msg, &hp_woken);
    if (hp_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static bool ensure_gpio_event_infra(void)
{
    if (s_gpio_event_queue == NULL) {
        s_gpio_event_queue = xQueueCreate(GPIO_EVENT_QUEUE_LEN, sizeof(gpio_event_msg_t));
        if (s_gpio_event_queue == NULL) {
            return false;
        }
    }
    if (!s_gpio_isr_service_installed) {
        esp_err_t err = gpio_install_isr_service(0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            return false;
        }
        s_gpio_isr_service_installed = true;
    }
    return true;
}

static bool gpio_event_enable_pin(uint8_t pin)
{
    if (pin >= 32 || pin == (uint8_t)BOARD_LED_GPIO) {
        // LED pin is owned by LEDC; reconfiguring it as GPIO interrupt would
        // unbind the PWM output. Refuse to arm events on it.
        return false;
    }
    if (!ensure_gpio_event_infra()) {
        return false;
    }
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    if (gpio_config(&io_conf) != ESP_OK) {
        return false;
    }
    if (gpio_isr_handler_add((gpio_num_t)pin, gpio_event_isr, (void *)(uintptr_t)pin) != ESP_OK) {
        return false;
    }
    s_gpio_event_enabled_mask |= (1u << pin);
    return true;
}

static bool gpio_event_disable_pin(uint8_t pin)
{
    if (pin >= 32) {
        return false;
    }
    if ((s_gpio_event_enabled_mask & (1u << pin)) == 0) {
        return true;  // already disabled — idempotent
    }
    gpio_set_intr_type((gpio_num_t)pin, GPIO_INTR_DISABLE);
    gpio_isr_handler_remove((gpio_num_t)pin);
    s_gpio_event_enabled_mask &= ~(1u << pin);
    return true;
}

static void gpio_event_disable_all(void)
{
    for (uint8_t p = 0; p < 32; p++) {
        if (s_gpio_event_enabled_mask & (1u << p)) {
            gpio_set_intr_type((gpio_num_t)p, GPIO_INTR_DISABLE);
            gpio_isr_handler_remove((gpio_num_t)p);
        }
    }
    s_gpio_event_enabled_mask = 0;
}

static void drain_gpio_event_queue(void)
{
    if (s_gpio_event_queue == NULL) {
        return;
    }
    gpio_event_msg_t msg;
    while (xQueueReceive(s_gpio_event_queue, &msg, 0) == pdTRUE) {
        uint8_t data[FRAME_DATA_LEN] = {
            msg.pin,
            msg.level,
            (uint8_t)( msg.timestamp_ms        & 0xFF),
            (uint8_t)((msg.timestamp_ms >> 8)  & 0xFF),
            (uint8_t)((msg.timestamp_ms >> 16) & 0xFF),
            (uint8_t)((msg.timestamp_ms >> 24) & 0xFF),
        };
        // High bit set in id field marks this as a spontaneous event (vs.
        // a request-acknowledge with the regular CMD_STATUS_* values).
        send_cmd_response(CMD_ID_GPIOEVT, GPIOEVT_EVENT_FLAG | (msg.pin & 0x1F), data, FRAME_DATA_LEN);
    }
}

static void handle_gpioevt_command(const uint8_t *cmd_buf, int len)
{
    uint8_t status = CMD_STATUS_OK;
    uint8_t op = len >= 4 ? cmd_buf[1] : 0xFF;
    uint8_t pin = len >= 5 ? cmd_buf[2] : 0xFF;
    uint8_t resp[FRAME_DATA_LEN] = {0};

    switch (op) {
    case GPIOEVT_OP_QUERY:
        // resp[0..3] = enabled-pins bitmap (little endian)
        resp[0] = (uint8_t)( s_gpio_event_enabled_mask        & 0xFF);
        resp[1] = (uint8_t)((s_gpio_event_enabled_mask >> 8)  & 0xFF);
        resp[2] = (uint8_t)((s_gpio_event_enabled_mask >> 16) & 0xFF);
        resp[3] = (uint8_t)((s_gpio_event_enabled_mask >> 24) & 0xFF);
        break;
    case GPIOEVT_OP_ENABLE:
        if (len < 5 || !is_supported_gpio(pin) || pin == (uint8_t)BOARD_LED_GPIO) {
            status = CMD_STATUS_PIN;
            break;
        }
        if (!gpio_event_enable_pin(pin)) {
            status = CMD_STATUS_DENIED;
        }
        resp[0] = pin;
        resp[1] = (s_gpio_event_enabled_mask >> 0)  & 0xFF;
        resp[2] = (s_gpio_event_enabled_mask >> 8)  & 0xFF;
        resp[3] = (s_gpio_event_enabled_mask >> 16) & 0xFF;
        resp[4] = (s_gpio_event_enabled_mask >> 24) & 0xFF;
        break;
    case GPIOEVT_OP_DISABLE:
        if (len < 5) {
            status = CMD_STATUS_ARGS;
            break;
        }
        if (!gpio_event_disable_pin(pin)) {
            status = CMD_STATUS_DENIED;
        }
        resp[0] = pin;
        resp[1] = (s_gpio_event_enabled_mask >> 0)  & 0xFF;
        resp[2] = (s_gpio_event_enabled_mask >> 8)  & 0xFF;
        resp[3] = (s_gpio_event_enabled_mask >> 16) & 0xFF;
        resp[4] = (s_gpio_event_enabled_mask >> 24) & 0xFF;
        break;
    case GPIOEVT_OP_CLEAR:
        gpio_event_disable_all();
        break;
    default:
        status = CMD_STATUS_ARGS;
        break;
    }
    send_cmd_response(CMD_ID_GPIOEVT, status, resp, FRAME_DATA_LEN);
}

static void handle_gpio_command(const uint8_t *cmd_buf, int len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t op = len >= 4 ? cmd_buf[1] : 0;
    uint8_t pin_code = len >= 5 ? cmd_buf[2] : 0;
    uint8_t status = CMD_STATUS_OK;
    uint16_t analog_raw = 0;
    bool analog_reply = false;

    if (len < 5) {
        send_cmd_response(CMD_ID_GPIO, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    if (!is_supported_gpio(pin_code)) {
        fill_gpio_response(op, pin_code, resp);
        send_cmd_response(CMD_ID_GPIO, CMD_STATUS_PIN, resp, FRAME_DATA_LEN);
        return;
    }

    switch (op) {
    case 0:
    case 1:
        break;
    case 2:
        if (len < 6) {
            status = CMD_STATUS_ARGS;
            break;
        }
        if (pin_code == (uint8_t)BOARD_LED_GPIO) {
#ifdef CONFIG_BOARD_LED_TYPE_RGB
            if (cmd_buf[3]) {
                s_rgb_led_on = true;
                safe_rgb_led_set(32, 32, 32);   // ON = white
            } else {
                s_rgb_led_on = false;
                safe_rgb_led_off();             // OFF
            }
#else
            // Board LED is driven by LEDC; calling apply_gpio_config() here
            // would gpio_config() the pin and detach the LEDC PWM output.
            set_board_led_idle_level(cmd_buf[3] ? 1 : 0);
#endif
        } else {
            apply_gpio_config(pin_code, 0, 1, 0);
            gpio_set_level((gpio_num_t)pin_code, cmd_buf[3] ? 1 : 0);
        }
        break;
    case 3:
        if (pin_code == (uint8_t)BOARD_LED_GPIO) {
#ifdef CONFIG_BOARD_LED_TYPE_RGB
            s_rgb_led_on = !s_rgb_led_on;
            if (s_rgb_led_on) {
                safe_rgb_led_set(32, 32, 32);   // ON = white
            } else {
                safe_rgb_led_off();             // OFF
            }
#else
            set_board_led_idle_level(s_board_led_idle_level ? 0 : 1);
#endif
        } else {
            apply_gpio_config(pin_code, 0, 1, 0);
            gpio_set_level((gpio_num_t)pin_code, !gpio_get_level((gpio_num_t)pin_code));
        }
        break;
    case 4:
        if (len < 8) {
            status = CMD_STATUS_ARGS;
            break;
        }
        if (!apply_gpio_config(pin_code, cmd_buf[3], cmd_buf[4], cmd_buf[5])) {
            status = CMD_STATUS_PIN;
        }
        break;
    case GPIO_OP_ANALOG_READ:
        analog_reply = true;
        if (!is_analog_gpio(pin_code)) {
            status = CMD_STATUS_PIN;
            break;
        }
        if (!read_gpio_analog(pin_code, &analog_raw)) {
            status = CMD_STATUS_DENIED;
        }
        break;
    case 8: // GPIO_OP_RGB — set WS2812 colour (pin, R, G, B)
#ifdef CONFIG_BOARD_LED_TYPE_RGB
        if (len < 8) {
            status = CMD_STATUS_ARGS;
            break;
        }
        if (pin_code != (uint8_t)BOARD_LED_GPIO) {
            status = CMD_STATUS_PIN;
            break;
        }
        safe_rgb_led_set(cmd_buf[3], cmd_buf[4], cmd_buf[5]);
#else
        status = CMD_STATUS_DENIED;
#endif
        break;
    default:
        status = CMD_STATUS_ARGS;
        break;
    }

    if (analog_reply) {
        fill_gpio_analog_response(pin_code, analog_raw, resp);
    } else {
        fill_gpio_response(op, pin_code, resp);
    }
    send_cmd_response(CMD_ID_GPIO, status, resp, FRAME_DATA_LEN);
}

static void handle_uart_command(const uint8_t *cmd_buf, int len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t op = len >= 4 ? cmd_buf[1] : 0;

    if (len < 4) {
        send_cmd_response(CMD_ID_UART, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    switch (op) {
    case 0:
        fill_uart_response(op, resp);
        send_cmd_response(CMD_ID_UART, CMD_STATUS_OK, resp, FRAME_DATA_LEN);
        break;
    case 1:
        if (len < 7) {
            send_cmd_response(CMD_ID_UART, CMD_STATUS_ARGS, NULL, 0);
            break;
        }
        resp[0] = op;
        resp[1] = cmd_buf[2];
        resp[2] = cmd_buf[3];
        resp[3] = cmd_buf[4];
        resp[4] = s_state.uart_baud_index;
        resp[5] = UART_BAUD_RATE_COUNT;
        send_cmd_response(CMD_ID_UART, CMD_STATUS_OK, resp, FRAME_DATA_LEN);
        break;
    case 2:
        fill_uart_response(op, resp);
        send_cmd_response(CMD_ID_UART, CMD_STATUS_DENIED, resp, FRAME_DATA_LEN);
        break;
    default:
        send_cmd_response(CMD_ID_UART, CMD_STATUS_ARGS, NULL, 0);
        break;
    }
}

static void handle_rfsdk_command(const uint8_t *cmd_buf, int len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t op = len >= 4 ? cmd_buf[1] : 0;
    uint8_t status = CMD_STATUS_OK;

    if (len < 4) {
        send_cmd_response(CMD_ID_RFSDK, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    switch (op) {
    case 0:
        break;
    case 1:
        if (len < 5 || cmd_buf[2] > ESP_PWR_LVL_P20) {
            status = CMD_STATUS_VALUE;
            break;
        }
        if (esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, (esp_power_level_t)cmd_buf[2]) != ESP_OK ||
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, (esp_power_level_t)cmd_buf[2]) != ESP_OK ||
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, (esp_power_level_t)cmd_buf[2]) != ESP_OK) {
            status = CMD_STATUS_DENIED;
            break;
        }
        s_state.power_index = cmd_buf[2];
        break;
    case 2:
    case 3:
        status = CMD_STATUS_DENIED;
        break;
    case 4:
        if (len < 5) {
            status = CMD_STATUS_ARGS;
            break;
        }
        s_state.coded_min_units = cmd_buf[2];
        refresh_scan_params();
        break;
    default:
        status = CMD_STATUS_ARGS;
        break;
    }

    fill_rfsdk_response(resp);
    send_cmd_response(CMD_ID_RFSDK, status, resp, FRAME_DATA_LEN);
}

static void handle_version_command(void)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    fill_version_response(resp);
    send_cmd_response(CMD_ID_VERSION, SW_VERSION, resp, FRAME_DATA_LEN);
}

static void handle_conn_command(const uint8_t *cmd_buf, int len)
{
    uint8_t status = CMD_STATUS_OK;
    uint8_t op = len >= 4 ? cmd_buf[1] : 0xFF;

    switch (op) {
    case 0:
        break;
    case 1:
    case 2:
        if (len < 11) {
            status = CMD_STATUS_ARGS;
            break;
        }
        if (s_conn.state != CONN_STATE_IDLE) {
            status = CMD_STATUS_DENIED;
            break;
        }
        if (!start_conn_request(op == 2 ? 1 : 0, cmd_buf[2], &cmd_buf[3])) {
            clear_conn_runtime();
            status = CMD_STATUS_DENIED;
        }
        break;
    case 3:
        if (s_conn.state != CONN_STATE_CONNECTED || gl_profile_tab[PROFILE_A_APP_ID].gattc_if == ESP_GATT_IF_NONE) {
            status = CMD_STATUS_DENIED;
            break;
        }
        if (esp_ble_gattc_close(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                                gl_profile_tab[PROFILE_A_APP_ID].conn_id) != ESP_OK) {
            status = CMD_STATUS_DENIED;
        }
        break;
    case 4: {
        esp_ble_gattc_cancel_open_params_t cancel_params = {
            .gattc_if = gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
        };

        if (s_conn.state != CONN_STATE_CONNECTING || gl_profile_tab[PROFILE_A_APP_ID].gattc_if == ESP_GATT_IF_NONE) {
            status = CMD_STATUS_DENIED;
            break;
        }
        memcpy(cancel_params.remote_bda, s_conn.peer_addr, sizeof(esp_bd_addr_t));
        if (esp_ble_gattc_cancel_open(&cancel_params) != ESP_OK) {
            status = CMD_STATUS_DENIED;
        }
        break;
    }
    default:
        status = CMD_STATUS_ARGS;
        break;
    }

    send_conn_response(status);
}

static void handle_txadv_command(const uint8_t *cmd_buf, int len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t status = CMD_STATUS_OK;
    uint8_t op = len >= 4 ? cmd_buf[1] : 0xFF;

    if (s_gap_sync_sem == NULL) {
        status = CMD_STATUS_DENIED;
        fill_txadv_response(resp);
        send_cmd_response(CMD_ID_TXADV, status, resp, FRAME_DATA_LEN);
        return;
    }

    switch (op) {
    case 0:
        if (!stop_txadv_if_running()) {
            status = CMD_STATUS_DENIED;
        }
        break;
    case 2:
        // Status query — return CMD_STATUS_OK with current state in resp.
        break;
    case 1: {
        uint8_t phy;
        uint16_t interval_units;
        uint8_t adv_len;

        if (len < 8) {
            status = CMD_STATUS_ARGS;
            break;
        }

        phy = cmd_buf[2];
        interval_units = (uint16_t)cmd_buf[3] | ((uint16_t)cmd_buf[4] << 8);
        adv_len = cmd_buf[5];

        if (phy > 2 || interval_units < 0x20 || adv_len > TXADV_DATA_MAX_LEN) {
            status = CMD_STATUS_VALUE;
            break;
        }
        if (len < 8 + adv_len) {
            status = CMD_STATUS_ARGS;
            break;
        }
        if (!start_txadv(phy, interval_units, &cmd_buf[6], adv_len)) {
            status = CMD_STATUS_DENIED;
        }
        break;
    }
    default:
        status = CMD_STATUS_ARGS;
        break;
    }

    fill_txadv_response(resp);
    send_cmd_response(CMD_ID_TXADV, status, resp, FRAME_DATA_LEN);
}

static void handle_txdata_command(const uint8_t *cmd_buf, int len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t status = CMD_STATUS_OK;

    if (len >= 6) {
        resp[0] = cmd_buf[1];
        resp[1] = cmd_buf[2];
        resp[2] = cmd_buf[3];
    }

    if (len < 6) {
        status = CMD_STATUS_ARGS;
    } else if (len < 6 + cmd_buf[3]) {
        status = CMD_STATUS_ARGS;
    } else if (s_conn.state != CONN_STATE_CONNECTED || gl_profile_tab[PROFILE_A_APP_ID].gattc_if == ESP_GATT_IF_NONE) {
        status = CMD_STATUS_DENIED;
        resp[3] = ESP_GATT_ERROR;
    } else {
        esp_err_t tx_ret = esp_ble_gattc_write_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                                                    gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                    (uint16_t)cmd_buf[1] | ((uint16_t)cmd_buf[2] << 8),
                                                    cmd_buf[3],
                                                    (uint8_t *)&cmd_buf[4],
                                                    ESP_GATT_WRITE_TYPE_NO_RSP,
                                                    ESP_GATT_AUTH_REQ_NONE);
        if (tx_ret == ESP_OK) {
            resp[3] = ESP_GATT_OK;
        } else {
            status = CMD_STATUS_DENIED;
            resp[3] = ESP_GATT_ERROR;
        }
    }

    send_cmd_response(CMD_ID_TXDATA, status, resp, FRAME_DATA_LEN);
}

static void io_task(void *arg)
{
	usb_serial_jtag_driver_config_t usb_serial_config = {
        .tx_buffer_size = 2048,
	    .rx_buffer_size = 256,
    };
	ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_config));

    static bool s_init_msg_sent = false;
    while (1) {
        int len = usb_serial_jtag_read_bytes(read_buf, sizeof(read_buf), 3 / portTICK_PERIOD_MS);
        if (!s_init_msg_sent) {
            s_init_msg_sent = true;
            send_debug_print("Board LED: %s gpio=%d%s",
#ifdef CONFIG_BOARD_LED_TYPE_RGB
                s_rgb_led_ready ? "RGB" : "RGB-FAIL",
#else
                "PWM",
#endif
                (uint8_t)BOARD_LED_GPIO,
                is_supported_gpio((uint8_t)BOARD_LED_GPIO) ? "" : " (unsupported!)");
        }
        // Drain any pending GPIO events whether or not we just read a command.
        // At a 3 ms read tick this runs ~330x/s, fast enough that the queue
        // (16 slots) never overflows for realistic button-press / GPIO activity.
        drain_gpio_event_queue();
        if (len <= 2 || crcFast(read_buf, len) != 0) {
            continue;
        }

        switch (read_buf[0]) {
        case CMD_ID_INFO:
			esp_read_mac(read_buf, ESP_MAC_BT);
			// byte[6] = rgb_led_ready flag (protocol extension for debug)
			read_buf[6] = s_rgb_led_ready ? 0x01 : 0x00;
			send_cmd_response(CMD_ID_INFO, SW_VERSION, read_buf, FRAME_DATA_LEN + 1);
			break;
		case CMD_ID_SCAN:
			handle_scan_command(read_buf, len);
			break;
		case CMD_ID_WMAC:
		case CMD_ID_BMAC:
			handle_mac_list_command(read_buf[0], read_buf, len);
			break;
		case CMD_ID_CLRM:
			clear_filter_lists();
			send_cmd_response(CMD_ID_CLRM, FILTER_LIST_CAPACITY, NULL, 0);
			break;
		case CMD_ID_GPIO:
			handle_gpio_command(read_buf, len);
			break;
		case CMD_ID_UART:
			handle_uart_command(read_buf, len);
			break;
		case CMD_ID_RFSDK:
			handle_rfsdk_command(read_buf, len);
			break;
		case CMD_ID_VERSION:
			handle_version_command();
			break;
		case CMD_ID_VBAT:
			send_cmd_response(CMD_ID_VBAT, CMD_STATUS_DENIED, NULL, 0);
			break;
		case CMD_ID_TXADV:
            handle_txadv_command(read_buf, len);
			break;
		case CMD_ID_CONN:
			handle_conn_command(read_buf, len);
			break;
		case CMD_ID_TXDATA:
			handle_txdata_command(read_buf, len);
			break;
		case CMD_ID_RXDATA:
			send_cmd_response(CMD_ID_RXDATA, CMD_STATUS_DENIED, NULL, 0);
			break;
		case CMD_ID_GPIOEVT:
			handle_gpioevt_command(read_buf, len);
			break;
		default:
			send_debug_print("unsupported cmd: 0x%02x", read_buf[0]);
			break;
		}
	}
}

static void quiet_protocol_uart_logging(void)
{
    /* The protocol shares USB Serial/JTAG with ESP-IDF logs. Keep the link binary-clean. */
    esp_log_level_set("*", ESP_LOG_NONE);
}

void app_main(void)
{
    quiet_protocol_uart_logging();

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

    refresh_scan_params();
    current_power_index();
    s_gap_sync_sem = xSemaphoreCreateBinary();
    if (is_supported_gpio((uint8_t)BOARD_LED_GPIO)) {
        const esp_timer_create_args_t board_led_timer_args = {
#ifndef CONFIG_BOARD_LED_TYPE_RGB
            .callback = restore_board_led_idle_level,
#else
            .callback = rgb_led_timer_cb,  // RGB: restore turns LED off
#endif
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "board_led",
        };

        ESP_ERROR_CHECK(esp_timer_create(&board_led_timer_args, &s_board_led_timer));
#ifdef CONFIG_BOARD_LED_TYPE_RGB
        esp_err_t rgb_ret = rgb_led_init();
        if (rgb_ret != ESP_OK) {
            ESP_LOGE(GATTC_TAG, "rgb_led_init failed (0x%x) — RGB LED will not work", rgb_ret);
            s_rgb_led_ready = false;
        } else {
            ESP_LOGI(GATTC_TAG, "RGB LED initialized on GPIO%d", CONFIG_BOARD_LED_GPIO);
            s_rgb_led_ready = true;
            s_rgb_mutex = xSemaphoreCreateMutex();
        }
        send_debug_print("RGB: %s gpio=%d supported=1",
            s_rgb_led_ready ? "OK" : "FAIL",
            (uint8_t)BOARD_LED_GPIO);
#else
        // Hardware PWM: pin matrix is routed to LEDC, duty changes are O(1)
        // register writes. No CPU/interrupt overhead during PWM cycles, so the
        // BLE RX path is not impacted.
        init_board_led_pwm();
#endif
        set_board_led_idle_level(BOARD_LED_IDLE_LEVEL);
    } else {
        send_debug_print("RGB: gpio=%d NOT supported!", (uint8_t)BOARD_LED_GPIO);
    }

    init_adc_inputs();

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

