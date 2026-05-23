/********************************** (C) COPYRIGHT *******************************
 * File Name          : observer.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : BLE observer / broadcaster / central application task
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "observer.h"
#include "app_usb.h"

#include <stdbool.h>
#include <string.h>

/*********************************************************************
 * MACROS
 */
#define CH582M_HW_VERSION              0x58

#define TXADV_RUNTIME_STOPPED          0
#define TXADV_RUNTIME_LEGACY_1M        1
#define TXADV_RUNTIME_EXT_1M           2
#define TXADV_RUNTIME_EXT_CODED        3

#define CONN_STATE_IDLE                0
#define CONN_STATE_CONNECTING          1
#define CONN_STATE_CONNECTED           2

#define INVALID_CONN_HANDLE            0xFFFF
#define INVALID_ATT_HANDLE             0x0000

#define UART_BAUD_RATE_COUNT           3

/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    GPIO_OP_STATUS = 0,
    GPIO_OP_READ = 1,
    GPIO_OP_WRITE = 2,
    GPIO_OP_TOGGLE = 3,
    GPIO_OP_CONFIG = 4,
    GPIO_OP_PWM = 5,
    GPIO_OP_PWM_STOP = 6,
    GPIO_OP_ANALOG_READ = 7,
} gpio_op_t;

typedef enum {
    UART_OP_QUERY = 0,
    UART_OP_PING = 1,
    UART_OP_SET_BAUD = 2,
} uart_op_t;

typedef enum {
    RFSDK_OP_QUERY = 0,
    RFSDK_OP_SET_POWER = 1,
    RFSDK_OP_SET_CAP = 2,
    RFSDK_OP_SET_CHANNELS = 3,
    RFSDK_OP_SET_CODED_MIN = 4,
} rfsdk_op_t;

typedef enum {
    TXADV_OP_STOP = 0,
    TXADV_OP_START = 1,
    TXADV_OP_STATUS = 2,
} txadv_op_t;

typedef enum {
    CONN_OP_STATUS = 0,
    CONN_OP_CONNECT_1M = 1,
    CONN_OP_CONNECT_CODED = 2,
    CONN_OP_DISCONNECT = 3,
    CONN_OP_CANCEL = 4,
} conn_op_t;

typedef enum {
    GPIOEVT_OP_QUERY = 0,
    GPIOEVT_OP_ENABLE = 1,
    GPIOEVT_OP_DISABLE = 2,
    GPIOEVT_OP_CLEAR = 3,
} gpioevt_op_t;

typedef enum {
    CONN_DISC_IDLE = 0,
    CONN_DISC_SERVICE,
    CONN_DISC_NOTIFY_CHAR,
    CONN_DISC_NOTIFY_CCCD,
    CONN_DISC_ENABLE_CCCD,
} conn_discovery_state_t;

typedef struct {
    uint8_t running_phy;
    uint16_t interval_units;
    uint8_t adv_len;
    uint8_t adv_data[TXADV_MAX_DATA_LEN];
} txadv_runtime_t;

typedef struct {
    uint8_t state;
    uint8_t pending_phy_bits;
    uint8_t peer_addr_type;
    uint8_t peer_addr[B_ADDR_LEN];
    uint16_t conn_handle;
    uint16_t interval_units;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t notify_handle;
    uint16_t cccd_handle;
    uint8_t notify_ready;
    uint8_t wait_for_scan_cancel;
    conn_discovery_state_t discovery_state;
} conn_runtime_t;

typedef struct {
    uint8_t port;
    uint8_t bit;
    uint32_t mask;
} gpio_pin_desc_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8_t ObserverTaskId;

scan_params_t scan_params = {
    .enabled = FALSE,
    .phys = GAP_PHY_BIT_LE_1M | GAP_PHY_BIT_LE_CODED,
    .active_scan = DEFAULT_DISCOVERY_ACTIVE_SCAN,
    .duplicate_filter = FALSE,
    .filter_random_addresses = FALSE,
    .filter_private_addresses = FALSE,
    .own_addr_type = 0,
    .window_1m_units = 48,
    .window_coded_units = 48,
};

app_drv_fifo_t app_tx_fifo;

uint8_t app_cmd_buf[APP_RX_BUFFER_LENGTH];
uint8_t app_cmd_len;

mac_filter_list_t white_list;
mac_filter_list_t black_list;

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t app_tx_buffer[APP_TX_BUFFER_LENGTH];
static txadv_runtime_t s_txadv;
static conn_runtime_t s_conn;
static uint8_t s_scan_running;
static uint8_t s_uart_baud_index;
static uint16_t s_uart_reset_sequence;
static uint8_t s_uart_reset_pattern;
static uint8_t s_rfsdk_power = BLE_TX_POWER;
static uint8_t s_rfsdk_cap = 0xFF;
static uint8_t s_rfsdk_channels[3] = {37, 38, 39};
static uint8_t s_rfsdk_coded_min_10ms = 10;
static uint8_t s_txdata_buffer[TXDATA_MAX_DATA_LEN];
static uint8_t s_cccd_enable_value[2] = {0x01, 0x00};

static const uint32_t s_uart_baud_rates[UART_BAUD_RATE_COUNT] = {
    2000000u,
    921600u,
    115200u,
};

static const uint8_t s_remote_service_uuid[2] = {0xFF, 0x00};
static const uint8_t s_remote_notify_uuid[2] = {0x01, 0xFF};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void ObserverEventCB(gapRoleEvent_t *pEvent);
static void CentralEventCB(gapRoleEvent_t *pEvent);
static void BroadcasterStateCB(gapRole_States_t newState);
static void Observer_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);

static uint16_t crc16_modbus(const uint8_t *data, uint16_t len);
static void enqueue_tx_bytes(const uint8_t *data, uint16_t len);
static void send_frame(uint8_t payload_len,
                       uint8_t byte1,
                       uint8_t byte2,
                       uint8_t byte3,
                       uint8_t byte4,
                       const uint8_t data_field[FRAME_DATA_LEN],
                       const uint8_t *payload);
static void send_cmd_response(uint8_t cmd, uint8_t id, const uint8_t *data, uint8_t len);
static void send_cmd_response_bytes(uint8_t cmd, uint8_t id, const uint8_t *data, uint8_t len);
static void send_adv_report(int8_t rssi,
                            uint8_t event_type,
                            uint8_t addr_type,
                            uint8_t phy,
                            const uint8_t *addr,
                            const uint8_t *payload,
                            uint16_t payload_len);
static void clear_conn_runtime(void);
static void send_conn_response(uint8_t status);
static void send_txdata_response(uint8_t status, uint16_t att_handle, uint8_t data_len, uint8_t ble_status);
static void send_rxdata_response(uint8_t opcode, uint16_t att_handle, const uint8_t *value, uint16_t value_len);

static uint8_t total_filter_count(void);
static uint8_t add_filter_entry(mac_filter_list_t *list, const uint8_t *mac, uint8_t mac_len);
static bool mac_filter_matches(const mac_filter_list_t *list, const uint8_t *mac);
static bool should_accept_adv(const uint8_t *mac, uint8_t addr_type);

static uint16_t clamp_scan_window_units(uint16_t units);
static void refresh_scan_params(void);
static bStatus_t start_scan_if_enabled(void);
static void stop_scan_now(void);
static bStatus_t begin_connection_request(void);

static bool decode_pin_id(uint8_t pin_id, gpio_pin_desc_t *pin);
static uint8_t read_pin_level(const gpio_pin_desc_t *pin);
static void write_pin_level(const gpio_pin_desc_t *pin, uint8_t value);
static void toggle_pin_level(const gpio_pin_desc_t *pin);
static void configure_pin_input(const gpio_pin_desc_t *pin, uint8_t pull_mode);
static void configure_pin_output(const gpio_pin_desc_t *pin);
static void fill_gpio_response(uint8_t op, uint8_t pin_id, uint16_t value, uint8_t *resp);
static uint16_t read_vbat_mv(void);
static int16_t read_die_temperature_celsius(void);

static void fill_uart_response(uint8_t op, uint8_t *resp);
static void fill_uart_ping_response(uint16_t sequence, uint8_t pattern, uint8_t *resp);
static void fill_rfsdk_response(uint8_t *resp);
static void fill_txadv_response(uint8_t *resp);
static void fill_version_response(uint8_t *resp);
static void fill_vbat_response(uint8_t *resp);

static void handle_scan_command(const uint8_t *cmd_buf, uint16_t len);
static void handle_mac_list_command(uint8_t cmd, const uint8_t *cmd_buf, uint16_t len);
static void handle_gpio_command(const uint8_t *cmd_buf, uint16_t len);
static void handle_uart_command(const uint8_t *cmd_buf, uint16_t len);
static void handle_rfsdk_command(const uint8_t *cmd_buf, uint16_t len);
static void handle_txadv_command(const uint8_t *cmd_buf, uint16_t len);
static void handle_conn_command(const uint8_t *cmd_buf, uint16_t len);
static void handle_txdata_command(const uint8_t *cmd_buf, uint16_t len);
static void handle_gpioevt_command(const uint8_t *cmd_buf, uint16_t len);
static void parse_command(void);

static bStatus_t start_service_discovery(void);
static bStatus_t start_notify_char_discovery(void);
static bStatus_t start_notify_cccd_discovery(void);
static bStatus_t enable_notify_cccd(void);
static void handle_gatt_msg(gattMsgEvent_t *pEvent);
static void free_gatt_payload(gattMsgEvent_t *pEvent);

/*********************************************************************
 * PROFILE CALLBACKS
 */
static const gapRoleObserverCB_t ObserverRoleCB = {
    ObserverEventCB,
};

static const gapCentralRoleCB_t CentralRoleCB = {
    NULL,
    CentralEventCB,
    NULL,
};

static const gapRolesBroadcasterCBs_t BroadcasterRoleCB = {
    BroadcasterStateCB,
    NULL,
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
void Observer_Init(void)
{
    ObserverTaskId = TMOS_ProcessEventRegister(Observer_ProcessEvent);

    app_drv_fifo_init(&app_tx_fifo, app_tx_buffer, APP_TX_BUFFER_LENGTH);

    memset(&white_list, 0, sizeof(white_list));
    memset(&black_list, 0, sizeof(black_list));
    memset(&s_txadv, 0, sizeof(s_txadv));
    clear_conn_runtime();

    GAPRole_BroadcasterInit();
    GAPRole_ObserverInit();
    GAPRole_CentralInit();
    GATT_InitClient();
    GATT_RegisterForInd(ObserverTaskId);

    tmos_set_event(ObserverTaskId, START_DEVICE_EVT);
}

static uint16_t crc16_modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t index;
    uint8_t bit;

    for(index = 0; index < len; index++)
    {
        crc ^= data[index];
        for(bit = 0; bit < 8; bit++)
        {
            if(crc & 1U)
            {
                crc = (crc >> 1) ^ 0xA001U;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

static void enqueue_tx_bytes(const uint8_t *data, uint16_t len)
{
    uint16_t write_len = len;

    if(data == NULL || len == 0)
    {
        return;
    }

    if(app_drv_fifo_write(&app_tx_fifo, (uint8_t *)data, &write_len) == APP_DRV_FIFO_RESULT_SUCCESS)
    {
        tmos_set_event(ObserverTaskId, NEW_BLEDATA_EVT);
    }
}

static void send_frame(uint8_t payload_len,
                       uint8_t byte1,
                       uint8_t byte2,
                       uint8_t byte3,
                       uint8_t byte4,
                       const uint8_t data_field[FRAME_DATA_LEN],
                       const uint8_t *payload)
{
    uint8_t frame[HEAD_CRC_ADD_LEN + MAX_ADV_PAYLOAD];
    uint16_t frame_len;
    uint16_t crc;

    if(payload_len > MAX_ADV_PAYLOAD)
    {
        payload_len = MAX_ADV_PAYLOAD;
    }

    memset(frame, 0, sizeof(frame));
    frame_len = HEAD_CRC_ADD_LEN + payload_len;

    frame[0] = payload_len;
    frame[1] = byte1;
    frame[2] = byte2;
    frame[3] = byte3;
    frame[4] = byte4;

    if(data_field != NULL)
    {
        memcpy(&frame[5], data_field, FRAME_DATA_LEN);
    }
    if(payload_len != 0 && payload != NULL)
    {
        memcpy(&frame[11], payload, payload_len);
    }

    crc = crc16_modbus(frame, frame_len - 2);
    frame[frame_len - 2] = crc & 0xFF;
    frame[frame_len - 1] = (crc >> 8) & 0xFF;

    enqueue_tx_bytes(frame, frame_len);
}

static void send_cmd_response(uint8_t cmd, uint8_t id, const uint8_t *data, uint8_t len)
{
    uint8_t data_field[FRAME_DATA_LEN] = {0};

    if(data != NULL && len != 0)
    {
        memcpy(data_field, data, len > FRAME_DATA_LEN ? FRAME_DATA_LEN : len);
    }

    send_frame(0, cmd, id, len, 0xFF, data_field, NULL);
}

static void send_cmd_response_bytes(uint8_t cmd, uint8_t id, const uint8_t *data, uint8_t len)
{
    uint8_t data_field[FRAME_DATA_LEN] = {0};
    uint8_t effective_len = len;
    uint8_t payload_len = 0;

    if(effective_len > FRAME_DATA_LEN + MAX_ADV_PAYLOAD)
    {
        effective_len = FRAME_DATA_LEN + MAX_ADV_PAYLOAD;
    }
    if(data != NULL && effective_len != 0)
    {
        memcpy(data_field, data, effective_len > FRAME_DATA_LEN ? FRAME_DATA_LEN : effective_len);
    }
    if(effective_len > FRAME_DATA_LEN)
    {
        payload_len = effective_len - FRAME_DATA_LEN;
    }

    send_frame(payload_len,
               cmd,
               id,
               effective_len,
               0xFF,
               data_field,
               payload_len != 0 ? data + FRAME_DATA_LEN : NULL);
}

static void send_adv_report(int8_t rssi,
                            uint8_t event_type,
                            uint8_t addr_type,
                            uint8_t phy,
                            const uint8_t *addr,
                            const uint8_t *payload,
                            uint16_t payload_len)
{
    uint8_t addr_field[FRAME_DATA_LEN] = {0};

    if(addr != NULL)
    {
        memcpy(addr_field, addr, B_ADDR_LEN);
    }
    if(payload_len > MAX_ADV_PAYLOAD)
    {
        payload_len = MAX_ADV_PAYLOAD;
    }

    send_frame((uint8_t)payload_len,
               (uint8_t)rssi,
               event_type,
               addr_type,
               phy,
               addr_field,
               payload_len != 0 ? payload : NULL);
}

static void clear_conn_runtime(void)
{
    memset(&s_conn, 0, sizeof(s_conn));
    s_conn.conn_handle = INVALID_CONN_HANDLE;
}

static void send_conn_response(uint8_t status)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};

    resp[0] = s_conn.state;
    resp[1] = s_conn.peer_addr_type;
    if(s_conn.conn_handle != INVALID_CONN_HANDLE)
    {
        resp[2] = s_conn.conn_handle & 0xFF;
        resp[3] = (s_conn.conn_handle >> 8) & 0xFF;
    }
    resp[4] = s_conn.interval_units & 0xFF;
    resp[5] = (s_conn.interval_units >> 8) & 0xFF;

    send_cmd_response(CMD_ID_CONN, status, resp, FRAME_DATA_LEN);
}

static void send_txdata_response(uint8_t status, uint16_t att_handle, uint8_t data_len, uint8_t ble_status)
{
    uint8_t resp[4];

    resp[0] = att_handle & 0xFF;
    resp[1] = (att_handle >> 8) & 0xFF;
    resp[2] = data_len;
    resp[3] = ble_status;

    send_cmd_response(CMD_ID_TXDATA, status, resp, sizeof(resp));
}

static void send_rxdata_response(uint8_t opcode, uint16_t att_handle, const uint8_t *value, uint16_t value_len)
{
    uint8_t resp[FRAME_DATA_LEN + TXDATA_MAX_DATA_LEN] = {0};
    uint16_t effective_len = value_len;

    if(effective_len > TXDATA_MAX_DATA_LEN)
    {
        effective_len = TXDATA_MAX_DATA_LEN;
    }

    resp[0] = att_handle & 0xFF;
    resp[1] = (att_handle >> 8) & 0xFF;
    resp[2] = (uint8_t)effective_len;

    if(effective_len != 0 && value != NULL)
    {
        memcpy(&resp[3], value, effective_len);
    }

    send_cmd_response_bytes(CMD_ID_RXDATA, opcode, resp, (uint8_t)(3 + effective_len));
}

static uint8_t total_filter_count(void)
{
    return white_list.count + black_list.count;
}

static uint8_t add_filter_entry(mac_filter_list_t *list, const uint8_t *mac, uint8_t mac_len)
{
    uint8_t copy_len = mac_len;

    if(list == NULL || mac == NULL)
    {
        return 0;
    }
    if(copy_len == 0 || copy_len > FRAME_DATA_LEN)
    {
        copy_len = FRAME_DATA_LEN;
    }
    if(list->count >= MAC_MAX_SCAN_LIST)
    {
        return list->count;
    }

    memset(list->mac[list->count], 0, FRAME_DATA_LEN);
    memcpy(list->mac[list->count], mac, copy_len);
    list->mac_len[list->count] = copy_len;
    list->count++;

    return list->count;
}

static bool mac_filter_matches(const mac_filter_list_t *list, const uint8_t *mac)
{
    uint8_t index;

    if(list == NULL || mac == NULL || list->count == 0)
    {
        return false;
    }

    for(index = 0; index < list->count; index++)
    {
        uint8_t cmp_len = list->mac_len[index];

        if(cmp_len == 0 || cmp_len > FRAME_DATA_LEN)
        {
            cmp_len = FRAME_DATA_LEN;
        }
        if(memcmp(list->mac[index], mac + (FRAME_DATA_LEN - cmp_len), cmp_len) == 0)
        {
            return true;
        }
    }

    return false;
}

static bool should_accept_adv(const uint8_t *mac, uint8_t addr_type)
{
    uint8_t base_addr_type = addr_type & 0x0F;

    if(scan_params.filter_random_addresses && base_addr_type != ADDRTYPE_PUBLIC)
    {
        return false;
    }
    if(scan_params.filter_private_addresses &&
       (base_addr_type == ADDRTYPE_PRIVATE_NONRESOLVE || base_addr_type == ADDRTYPE_PRIVATE_RESOLVE))
    {
        return false;
    }
    if(white_list.count != 0 && !mac_filter_matches(&white_list, mac))
    {
        return false;
    }
    if(black_list.count != 0 && mac_filter_matches(&black_list, mac))
    {
        return false;
    }

    return true;
}

static uint16_t clamp_scan_window_units(uint16_t units)
{
    if(units == 0)
    {
        units = 48;
    }
    if(units > 0x4000)
    {
        units = 0x4000;
    }
    return units;
}

static void refresh_scan_params(void)
{
    uint16_t window_1m = clamp_scan_window_units(scan_params.window_1m_units);
    uint16_t window_coded = clamp_scan_window_units(scan_params.window_coded_units);
    uint16_t coded_min = (uint16_t)s_rfsdk_coded_min_10ms * 16U;

    if(window_coded < coded_min)
    {
        window_coded = coded_min;
    }

    scan_params.window_1m_units = window_1m;
    scan_params.window_coded_units = window_coded;

    GAP_SetParamValue(TGAP_DISC_SCAN_PHY, scan_params.phys);
    GAP_SetParamValue(TGAP_FILTER_ADV_REPORTS, scan_params.duplicate_filter ? TRUE : FALSE);
    GAP_SetParamValue(TGAP_DISC_SCAN_INT, window_1m);
    GAP_SetParamValue(TGAP_DISC_SCAN_WIND, window_1m);
    GAP_SetParamValue(TGAP_DISC_SCAN_CODED_INT, window_coded);
    GAP_SetParamValue(TGAP_DISC_SCAN_CODED_WIND, window_coded);
    GAP_SetParamValue(TGAP_DISC_SCAN, 0);
    GAP_SetParamValue(TGAP_DISC_SCAN_DURATION, 0);
}

static bStatus_t start_scan_if_enabled(void)
{
    bStatus_t status;

    if(!scan_params.enabled || scan_params.phys == 0)
    {
        s_scan_running = FALSE;
        return SUCCESS;
    }
    if(s_conn.state != CONN_STATE_IDLE || s_conn.wait_for_scan_cancel)
    {
        return SUCCESS;
    }

    refresh_scan_params();
    status = GAPRole_ObserverStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                            scan_params.active_scan ? TRUE : FALSE,
                                            DEFAULT_DISCOVERY_WHITE_LIST);
    if(status == SUCCESS || status == bleAlreadyInRequestedMode)
    {
        s_scan_running = TRUE;
        return SUCCESS;
    }

    return status;
}

static void stop_scan_now(void)
{
    s_scan_running = FALSE;
    GAPRole_ObserverCancelDiscovery();
}

static bStatus_t begin_connection_request(void)
{
    uint16_t phy = (s_conn.pending_phy_bits == GAP_PHY_BIT_LE_CODED) ? GAP_PHY_BIT_LE_CODED : GAP_PHY_BIT_LE_1M;

    GAP_SetParamValue(TGAP_CONN_EST_INT_PHY, phy);

    return GAPRole_CentralEstablishLink(FALSE, FALSE, s_conn.peer_addr_type, s_conn.peer_addr);
}

static bool decode_pin_id(uint8_t pin_id, gpio_pin_desc_t *pin)
{
    if(pin == NULL)
    {
        return false;
    }

    memset(pin, 0, sizeof(*pin));

    if(pin_id <= 0x0F)
    {
        pin->port = 'A';
        pin->bit = pin_id;
    }
    else if(pin_id >= 0x10 && pin_id <= 0x1F)
    {
        pin->port = 'B';
        pin->bit = pin_id - 0x10;
    }
    else if(pin_id >= 0x20 && pin_id <= 0x27)
    {
        pin->port = 'B';
        pin->bit = (pin_id - 0x20) + 16;
    }
    else
    {
        return false;
    }

    if((pin->port == 'A' && pin->bit > 15) || (pin->port == 'B' && pin->bit > 23))
    {
        return false;
    }

    pin->mask = 1UL << pin->bit;
    return true;
}

static uint8_t read_pin_level(const gpio_pin_desc_t *pin)
{
    if(pin == NULL)
    {
        return 0;
    }
    if(pin->port == 'A')
    {
        return GPIOA_ReadPortPin(pin->mask) ? 1 : 0;
    }
    return GPIOB_ReadPortPin(pin->mask) ? 1 : 0;
}

static void write_pin_level(const gpio_pin_desc_t *pin, uint8_t value)
{
    if(pin == NULL)
    {
        return;
    }
    if(pin->port == 'A')
    {
        if(value)
        {
            GPIOA_SetBits(pin->mask);
        }
        else
        {
            GPIOA_ResetBits(pin->mask);
        }
    }
    else
    {
        if(value)
        {
            GPIOB_SetBits(pin->mask);
        }
        else
        {
            GPIOB_ResetBits(pin->mask);
        }
    }
}

static void toggle_pin_level(const gpio_pin_desc_t *pin)
{
    if(pin == NULL)
    {
        return;
    }
    if(pin->port == 'A')
    {
        GPIOA_InverseBits(pin->mask);
    }
    else
    {
        GPIOB_InverseBits(pin->mask);
    }
}

static void configure_pin_input(const gpio_pin_desc_t *pin, uint8_t pull_mode)
{
    GPIOModeTypeDef mode = GPIO_ModeIN_Floating;

    if(pin == NULL)
    {
        return;
    }

    if(pull_mode == 1)
    {
        mode = GPIO_ModeIN_PU;
    }
    else if(pull_mode == 2)
    {
        mode = GPIO_ModeIN_PD;
    }

    if(pin->port == 'A')
    {
        GPIOA_ModeCfg(pin->mask, mode);
    }
    else
    {
        GPIOB_ModeCfg(pin->mask, mode);
    }
}

static void configure_pin_output(const gpio_pin_desc_t *pin)
{
    if(pin == NULL)
    {
        return;
    }

    if(pin->port == 'A')
    {
        GPIOA_ModeCfg(pin->mask, GPIO_ModeOut_PP_5mA);
    }
    else
    {
        GPIOB_ModeCfg(pin->mask, GPIO_ModeOut_PP_5mA);
    }
}

static void fill_gpio_response(uint8_t op, uint8_t pin_id, uint16_t value, uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = op;
    resp[1] = pin_id;
    resp[2] = value & 0xFF;
    resp[3] = (value >> 8) & 0xFF;
    resp[4] = 0;
    resp[5] = 0;
}

static uint16_t read_vbat_mv(void)
{
    uint8_t saved_channel = R8_ADC_CHANNEL;
    uint8_t saved_cfg = R8_ADC_CFG;
    uint8_t saved_tkey_cfg = R8_TKEY_CFG;
    uint8_t saved_temp_sensor = R8_TEM_SENSOR;
    uint16_t raw;
    uint16_t mv;

    ADC_InterBATSampInit();
    raw = ADC_ExcutSingleConver();

    R8_ADC_CHANNEL = saved_channel;
    R8_ADC_CFG = saved_cfg;
    R8_TKEY_CFG = saved_tkey_cfg;
    R8_TEM_SENSOR = saved_temp_sensor;

    mv = (uint16_t)(((uint32_t)raw * 3300U + 2047U) / 4095U);
    return mv;
}

static int16_t read_die_temperature_celsius(void)
{
    uint8_t saved_channel = R8_ADC_CHANNEL;
    uint8_t saved_cfg = R8_ADC_CFG;
    uint8_t saved_tkey_cfg = R8_TKEY_CFG;
    uint8_t saved_temp_sensor = R8_TEM_SENSOR;
    uint16_t raw;
    int temp;

    ADC_InterTSSampInit();
    raw = ADC_ExcutSingleConver();

    R8_ADC_CHANNEL = saved_channel;
    R8_ADC_CFG = saved_cfg;
    R8_TKEY_CFG = saved_tkey_cfg;
    R8_TEM_SENSOR = saved_temp_sensor;

    temp = adc_to_temperature_celsius(raw);
    if(temp < -32767 || temp > 32767)
    {
        return (int16_t)0x8000;
    }

    return (int16_t)temp;
}

static void fill_uart_response(uint8_t op, uint8_t *resp)
{
    uint32_t baud = s_uart_baud_rates[s_uart_baud_index];

    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = op;
    resp[1] = s_uart_baud_index;
    resp[2] = UART_BAUD_RATE_COUNT;
    resp[3] = baud & 0xFF;
    resp[4] = (baud >> 8) & 0xFF;
    resp[5] = (baud >> 16) & 0xFF;
}

static void fill_uart_ping_response(uint16_t sequence, uint8_t pattern, uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = UART_OP_PING;
    resp[1] = sequence & 0xFF;
    resp[2] = (sequence >> 8) & 0xFF;
    resp[3] = pattern;
    resp[4] = (uint8_t)~pattern;
    resp[5] = s_uart_baud_index;
}

static void fill_rfsdk_response(uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = s_rfsdk_power;
    resp[1] = s_rfsdk_cap;
    resp[2] = s_rfsdk_channels[0];
    resp[3] = s_rfsdk_channels[1];
    resp[4] = s_rfsdk_channels[2];
    resp[5] = s_rfsdk_coded_min_10ms;
}

static void fill_txadv_response(uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = s_txadv.running_phy;
    resp[1] = s_txadv.interval_units & 0xFF;
    resp[2] = (s_txadv.interval_units >> 8) & 0xFF;
    resp[3] = s_txadv.adv_len;
}

static void fill_version_response(uint8_t *resp)
{
    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = CH582M_HW_VERSION;
    resp[1] = 5;
    resp[2] = 0;
    resp[3] = 0;
    resp[4] = 0;
    resp[5] = 0;
}

static void fill_vbat_response(uint8_t *resp)
{
    uint16_t batt_mv = read_vbat_mv();
    int16_t temp_c = read_die_temperature_celsius();

    memset(resp, 0, FRAME_DATA_LEN);
    resp[0] = batt_mv & 0xFF;
    resp[1] = (batt_mv >> 8) & 0xFF;
    resp[2] = temp_c & 0xFF;
    resp[3] = (temp_c >> 8) & 0xFF;
}

static void handle_scan_command(const uint8_t *cmd_buf, uint16_t len)
{
    uint8_t echo[FRAME_DATA_LEN] = {0};
    uint8_t flags;
    uint16_t data_len;

    if(len < 4)
    {
        send_cmd_response(CMD_ID_SCAN, total_filter_count(), NULL, 0);
        return;
    }

    data_len = len - 1;
    if(data_len != 3 && data_len != 5)
    {
        send_cmd_response(CMD_ID_SCAN, total_filter_count(), NULL, 0);
        return;
    }

    flags = cmd_buf[1];
    memcpy(echo, &cmd_buf[1], data_len > FRAME_DATA_LEN ? FRAME_DATA_LEN : data_len);

    stop_scan_now();

    scan_params.enabled = ((flags & 0x03) != 0) ? TRUE : FALSE;
    scan_params.phys = 0;
    if(flags & 0x01)
    {
        scan_params.phys |= GAP_PHY_BIT_LE_1M;
    }
    if(flags & 0x02)
    {
        scan_params.phys |= GAP_PHY_BIT_LE_CODED;
    }
    scan_params.active_scan = (flags >> 2) & 0x01;
    scan_params.duplicate_filter = (flags >> 3) & 0x01;
    scan_params.filter_private_addresses = (flags >> 4) & 0x01;
    scan_params.filter_random_addresses = (flags >> 5) & 0x01;
    scan_params.own_addr_type = (flags >> 6) & 0x03;

    scan_params.window_1m_units = clamp_scan_window_units((uint16_t)(cmd_buf[2] | (cmd_buf[3] << 8)));
    if(data_len == 5)
    {
        uint16_t coded = (uint16_t)(cmd_buf[4] | (cmd_buf[5] << 8));
        scan_params.window_coded_units = coded == 0 ? scan_params.window_1m_units : clamp_scan_window_units(coded);
    }
    else
    {
        scan_params.window_coded_units = scan_params.window_1m_units;
    }

    if(scan_params.enabled)
    {
        if(start_scan_if_enabled() != SUCCESS)
        {
            scan_params.enabled = FALSE;
            scan_params.phys = 0;
        }
    }

    send_cmd_response(CMD_ID_SCAN, total_filter_count(), echo, (uint8_t)data_len);
}

static void handle_mac_list_command(uint8_t cmd, const uint8_t *cmd_buf, uint16_t len)
{
    mac_filter_list_t *list = (cmd == CMD_ID_WMAC) ? &white_list : &black_list;
    uint8_t echo[FRAME_DATA_LEN] = {0};
    uint8_t mac_len;

    if(len < 2)
    {
        send_cmd_response(cmd, list->count, NULL, 0);
        return;
    }

    mac_len = len - 1;
    if(mac_len == 0 || mac_len > FRAME_DATA_LEN)
    {
        send_cmd_response(cmd, list->count, NULL, 0);
        return;
    }

    memcpy(echo, &cmd_buf[1], mac_len);
    send_cmd_response(cmd, add_filter_entry(list, &cmd_buf[1], mac_len), echo, mac_len);
}

static void handle_gpio_command(const uint8_t *cmd_buf, uint16_t len)
{
    gpio_pin_desc_t pin;
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t op = (len >= 2) ? cmd_buf[1] : 0xFF;
    uint8_t pin_id = (len >= 3) ? cmd_buf[2] : 0;
    uint8_t status = CMD_STATUS_OK;

    if(len < 3)
    {
        send_cmd_response(CMD_ID_GPIO, CMD_STATUS_ARGS, NULL, 0);
        return;
    }
    if(!decode_pin_id(pin_id, &pin))
    {
        fill_gpio_response(op, pin_id, 0, resp);
        send_cmd_response(CMD_ID_GPIO, CMD_STATUS_PIN, resp, FRAME_DATA_LEN);
        return;
    }

    switch(op)
    {
        case GPIO_OP_STATUS:
        case GPIO_OP_READ:
            fill_gpio_response(op, pin_id, read_pin_level(&pin), resp);
            break;

        case GPIO_OP_WRITE:
            if(len < 4)
            {
                send_cmd_response(CMD_ID_GPIO, CMD_STATUS_ARGS, NULL, 0);
                return;
            }
            configure_pin_output(&pin);
            write_pin_level(&pin, cmd_buf[3] ? 1 : 0);
            fill_gpio_response(op, pin_id, read_pin_level(&pin), resp);
            break;

        case GPIO_OP_TOGGLE:
            configure_pin_output(&pin);
            toggle_pin_level(&pin);
            fill_gpio_response(op, pin_id, read_pin_level(&pin), resp);
            break;

        case GPIO_OP_CONFIG:
            if(len < 6)
            {
                send_cmd_response(CMD_ID_GPIO, CMD_STATUS_ARGS, NULL, 0);
                return;
            }
            if(cmd_buf[4])
            {
                configure_pin_output(&pin);
            }
            else
            {
                if(cmd_buf[5] > 2)
                {
                    status = CMD_STATUS_VALUE;
                }
                configure_pin_input(&pin, cmd_buf[5]);
            }
            fill_gpio_response(op, pin_id, read_pin_level(&pin), resp);
            break;

        case GPIO_OP_PWM:
        case GPIO_OP_PWM_STOP:
        case GPIO_OP_ANALOG_READ:
            fill_gpio_response(op, pin_id, 0, resp);
            status = CMD_STATUS_DENIED;
            break;

        default:
            send_cmd_response(CMD_ID_GPIO, CMD_STATUS_ARGS, NULL, 0);
            return;
    }

    send_cmd_response(CMD_ID_GPIO, status, resp, FRAME_DATA_LEN);
}

static void handle_uart_command(const uint8_t *cmd_buf, uint16_t len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t status = CMD_STATUS_OK;

    if(len < 2)
    {
        send_cmd_response(CMD_ID_UART, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    switch(cmd_buf[1])
    {
        case UART_OP_QUERY:
            fill_uart_response(UART_OP_QUERY, resp);
            break;

        case UART_OP_PING:
            if(len < 5)
            {
                send_cmd_response(CMD_ID_UART, CMD_STATUS_ARGS, NULL, 0);
                return;
            }
            s_uart_reset_sequence = (uint16_t)(cmd_buf[2] | (cmd_buf[3] << 8));
            s_uart_reset_pattern = cmd_buf[4];
            fill_uart_ping_response(s_uart_reset_sequence, s_uart_reset_pattern, resp);
            break;

        case UART_OP_SET_BAUD:
            if(len < 3)
            {
                send_cmd_response(CMD_ID_UART, CMD_STATUS_ARGS, NULL, 0);
                return;
            }
            if(cmd_buf[2] >= UART_BAUD_RATE_COUNT)
            {
                status = CMD_STATUS_VALUE;
            }
            else
            {
                s_uart_baud_index = cmd_buf[2];
            }
            fill_uart_response(UART_OP_SET_BAUD, resp);
            break;

        default:
            send_cmd_response(CMD_ID_UART, CMD_STATUS_ARGS, NULL, 0);
            return;
    }

    send_cmd_response(CMD_ID_UART, status, resp, FRAME_DATA_LEN);
}

static void handle_rfsdk_command(const uint8_t *cmd_buf, uint16_t len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t status = CMD_STATUS_OK;

    if(len < 2)
    {
        send_cmd_response(CMD_ID_RFSDK, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    switch(cmd_buf[1])
    {
        case RFSDK_OP_QUERY:
            break;

        case RFSDK_OP_SET_POWER:
            if(len < 3)
            {
                send_cmd_response(CMD_ID_RFSDK, CMD_STATUS_ARGS, NULL, 0);
                return;
            }
            if(LL_SetTxPowerLevel(cmd_buf[2]) == SUCCESS)
            {
                s_rfsdk_power = cmd_buf[2];
            }
            else
            {
                status = CMD_STATUS_VALUE;
            }
            break;

        case RFSDK_OP_SET_CAP:
        case RFSDK_OP_SET_CHANNELS:
            status = CMD_STATUS_DENIED;
            break;

        case RFSDK_OP_SET_CODED_MIN:
            if(len < 3)
            {
                send_cmd_response(CMD_ID_RFSDK, CMD_STATUS_ARGS, NULL, 0);
                return;
            }
            if(cmd_buf[2] == 0)
            {
                status = CMD_STATUS_VALUE;
            }
            else
            {
                s_rfsdk_coded_min_10ms = cmd_buf[2];
            }
            break;

        default:
            send_cmd_response(CMD_ID_RFSDK, CMD_STATUS_ARGS, NULL, 0);
            return;
    }

    fill_rfsdk_response(resp);
    send_cmd_response(CMD_ID_RFSDK, status, resp, FRAME_DATA_LEN);
}

static void handle_txadv_command(const uint8_t *cmd_buf, uint16_t len)
{
    uint8_t resp[FRAME_DATA_LEN] = {0};
    uint8_t status = CMD_STATUS_OK;
    uint8_t op;

    if(len < 2)
    {
        send_cmd_response(CMD_ID_TXADV, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    op = cmd_buf[1];

    if(op == TXADV_OP_STOP)
    {
        uint8_t disabled = FALSE;

        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(disabled), &disabled);
        memset(&s_txadv, 0, sizeof(s_txadv));
    }
    else if(op == TXADV_OP_STATUS)
    {
        /* nothing to do */
    }
    else if(op == TXADV_OP_START)
    {
        uint8_t requested_phy;
        uint8_t runtime_phy;
        uint8_t adv_type = GAP_ADTYPE_ADV_NONCONN_IND;
        uint8_t enabled = TRUE;
        uint8_t phy_supported;
        uint16_t interval_units;
        uint16_t adv_len;
        uint16_t primary_phy = GAP_PHY_VAL_LE_1M;
        uint16_t secondary_phy = GAP_PHY_VAL_LE_1M;
        uint16_t phy_options = GAP_PHY_OPTIONS_NOPRE;
        uint8_t disabled = FALSE;

        if(len < 6)
        {
            send_cmd_response(CMD_ID_TXADV, CMD_STATUS_ARGS, NULL, 0);
            return;
        }

        requested_phy = cmd_buf[2];
        interval_units = (uint16_t)(cmd_buf[3] | (cmd_buf[4] << 8));
        adv_len = cmd_buf[5];

        if(adv_len > TXADV_MAX_DATA_LEN || len != (uint16_t)(6 + adv_len))
        {
            send_cmd_response(CMD_ID_TXADV, CMD_STATUS_ARGS, NULL, 0);
            return;
        }

        switch(requested_phy)
        {
            case 0:
                runtime_phy = TXADV_RUNTIME_LEGACY_1M;
                phy_supported = GAP_PHY_BIT_LE_1M;
                break;

            case 1:
                runtime_phy = TXADV_RUNTIME_EXT_1M;
                phy_supported = GAP_PHY_BIT_LE_1M;
                break;

            case 2:
                runtime_phy = TXADV_RUNTIME_EXT_CODED;
                phy_supported = GAP_PHY_BIT_LE_CODED;
                primary_phy = GAP_PHY_VAL_LE_CODED;
                secondary_phy = GAP_PHY_VAL_LE_CODED;
                phy_options = GAP_PHY_OPTIONS_S8;
                break;

            default:
                fill_txadv_response(resp);
                send_cmd_response(CMD_ID_TXADV, CMD_STATUS_VALUE, resp, FRAME_DATA_LEN);
                return;
        }

        if(interval_units == 0)
        {
            interval_units = 160;
        }

        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(disabled), &disabled);
        GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(adv_type), &adv_type);
        GAPRole_SetParameter(GAPROLE_PHY_TX_SUPPORTED, sizeof(phy_supported), &phy_supported);
        GAPRole_SetParameter(GAPROLE_PHY_RX_SUPPORTED, sizeof(phy_supported), &phy_supported);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, adv_len, (void *)&cmd_buf[6]);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, interval_units);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, interval_units);
        GAP_SetParamValue(TGAP_ADV_PRIMARY_PHY, primary_phy);
        GAP_SetParamValue(TGAP_ADV_SECONDARY_PHY, secondary_phy);
        GAP_SetParamValue(TGAP_ADV_PRIMARY_PHY_OPTIONS, phy_options);
        GAP_SetParamValue(TGAP_ADV_SECONDARY_PHY_OPTIONS, phy_options);
        GAP_SetParamValue(TGAP_ADV_ADVERTISING_DURATION, 0);
        GAP_SetParamValue(TGAP_ADV_MAX_EVENTS, 0);

        if(GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(enabled), &enabled) != SUCCESS)
        {
            status = CMD_STATUS_DENIED;
        }
        else
        {
            s_txadv.running_phy = runtime_phy;
            s_txadv.interval_units = interval_units;
            s_txadv.adv_len = adv_len;
            memset(s_txadv.adv_data, 0, sizeof(s_txadv.adv_data));
            if(adv_len != 0)
            {
                memcpy(s_txadv.adv_data, &cmd_buf[6], adv_len);
            }
        }
    }
    else
    {
        send_cmd_response(CMD_ID_TXADV, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    fill_txadv_response(resp);
    send_cmd_response(CMD_ID_TXADV, status, resp, FRAME_DATA_LEN);
}

static void handle_conn_command(const uint8_t *cmd_buf, uint16_t len)
{
    bStatus_t ble_status;
    uint8_t op;

    if(len < 2)
    {
        send_cmd_response(CMD_ID_CONN, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    op = cmd_buf[1];

    switch(op)
    {
        case CONN_OP_STATUS:
            send_conn_response(CMD_STATUS_OK);
            return;

        case CONN_OP_CONNECT_1M:
        case CONN_OP_CONNECT_CODED:
            if(len < 9)
            {
                send_cmd_response(CMD_ID_CONN, CMD_STATUS_ARGS, NULL, 0);
                return;
            }
            if(s_conn.state != CONN_STATE_IDLE)
            {
                send_conn_response(CMD_STATUS_DENIED);
                return;
            }

            clear_conn_runtime();
            s_conn.state = CONN_STATE_CONNECTING;
            s_conn.peer_addr_type = cmd_buf[2];
            memcpy(s_conn.peer_addr, &cmd_buf[3], B_ADDR_LEN);
            s_conn.pending_phy_bits = (op == CONN_OP_CONNECT_CODED) ? GAP_PHY_BIT_LE_CODED : GAP_PHY_BIT_LE_1M;

            if(s_scan_running)
            {
                s_conn.wait_for_scan_cancel = TRUE;
                stop_scan_now();
                send_conn_response(CMD_STATUS_OK);
                return;
            }

            ble_status = begin_connection_request();
            if(ble_status != SUCCESS)
            {
                clear_conn_runtime();
                start_scan_if_enabled();
                send_conn_response(CMD_STATUS_DENIED);
                return;
            }
            send_conn_response(CMD_STATUS_OK);
            return;

        case CONN_OP_DISCONNECT:
            if(s_conn.state != CONN_STATE_CONNECTED || s_conn.conn_handle == INVALID_CONN_HANDLE)
            {
                send_conn_response(CMD_STATUS_DENIED);
                return;
            }
            GAPRole_TerminateLink(s_conn.conn_handle);
            send_conn_response(CMD_STATUS_OK);
            return;

        case CONN_OP_CANCEL:
            send_conn_response(CMD_STATUS_DENIED);
            return;

        default:
            send_cmd_response(CMD_ID_CONN, CMD_STATUS_ARGS, NULL, 0);
            return;
    }
}

static void handle_txdata_command(const uint8_t *cmd_buf, uint16_t len)
{
    attWriteReq_t req;
    uint16_t att_handle;
    uint8_t data_len;
    bStatus_t ble_status;

    if(len < 4)
    {
        send_cmd_response(CMD_ID_TXDATA, CMD_STATUS_ARGS, NULL, 0);
        return;
    }
    if(s_conn.state != CONN_STATE_CONNECTED || s_conn.conn_handle == INVALID_CONN_HANDLE)
    {
        send_txdata_response(CMD_STATUS_DENIED, 0, 0, bleNotConnected);
        return;
    }

    att_handle = (uint16_t)(cmd_buf[1] | (cmd_buf[2] << 8));
    data_len = cmd_buf[3];

    if(data_len > TXDATA_MAX_DATA_LEN || len != (uint16_t)(4 + data_len))
    {
        send_cmd_response(CMD_ID_TXDATA, CMD_STATUS_ARGS, NULL, 0);
        return;
    }

    memset(&req, 0, sizeof(req));
    memset(s_txdata_buffer, 0, sizeof(s_txdata_buffer));
    if(data_len != 0)
    {
        memcpy(s_txdata_buffer, &cmd_buf[4], data_len);
    }

    req.handle = att_handle;
    req.len = data_len;
    req.pValue = s_txdata_buffer;
    req.sig = 0;
    req.cmd = 1;

    ble_status = GATT_WriteNoRsp(s_conn.conn_handle, &req);
    send_txdata_response(ble_status == SUCCESS ? CMD_STATUS_OK : CMD_STATUS_DENIED,
                         att_handle,
                         data_len,
                         ble_status);
}

static void handle_gpioevt_command(const uint8_t *cmd_buf, uint16_t len)
{
    uint8_t op = (len >= 2) ? cmd_buf[1] : GPIOEVT_OP_QUERY;

    if(op == GPIOEVT_OP_QUERY)
    {
        uint8_t resp[4] = {0};
        send_cmd_response(CMD_ID_GPIOEVT, CMD_STATUS_DENIED, resp, sizeof(resp));
    }
    else
    {
        uint8_t resp[5] = {0};
        if(len >= 3)
        {
            resp[0] = cmd_buf[2];
        }
        send_cmd_response(CMD_ID_GPIOEVT, CMD_STATUS_DENIED, resp, sizeof(resp));
    }
}

static void parse_command(void)
{
    uint16_t cmd_len;
    uint16_t crc_expect;
    uint16_t crc_recv;

    if(app_cmd_len < 3)
    {
        app_cmd_len = 0;
        return;
    }

    crc_expect = crc16_modbus(app_cmd_buf, app_cmd_len - 2);
    crc_recv = (uint16_t)(app_cmd_buf[app_cmd_len - 2] | (app_cmd_buf[app_cmd_len - 1] << 8));
    cmd_len = app_cmd_len - 2;
    app_cmd_len = 0;

    if(crc_expect != crc_recv || cmd_len == 0)
    {
        return;
    }

    switch(app_cmd_buf[0])
    {
        case CMD_ID_INFO:
        {
            uint8_t mac[FRAME_DATA_LEN] = {0};
            GetMACAddress(mac);
            send_cmd_response(CMD_ID_INFO, SW_VERSION, mac, FRAME_DATA_LEN);
            break;
        }

        case CMD_ID_VERSION:
        {
            uint8_t resp[FRAME_DATA_LEN] = {0};
            fill_version_response(resp);
            send_cmd_response(CMD_ID_VERSION, SW_VERSION, resp, FRAME_DATA_LEN);
            break;
        }

        case CMD_ID_VBAT:
        {
            uint8_t resp[FRAME_DATA_LEN] = {0};
            fill_vbat_response(resp);
            send_cmd_response(CMD_ID_VBAT, CMD_STATUS_OK, resp, 4);
            break;
        }

        case CMD_ID_SCAN:
            handle_scan_command(app_cmd_buf, cmd_len);
            break;

        case CMD_ID_WMAC:
        case CMD_ID_BMAC:
            handle_mac_list_command(app_cmd_buf[0], app_cmd_buf, cmd_len);
            break;

        case CMD_ID_CLRM:
            memset(&white_list, 0, sizeof(white_list));
            memset(&black_list, 0, sizeof(black_list));
            send_cmd_response(CMD_ID_CLRM, MAC_MAX_SCAN_LIST, NULL, 0);
            break;

        case CMD_ID_GPIO:
            handle_gpio_command(app_cmd_buf, cmd_len);
            break;

        case CMD_ID_UART:
            handle_uart_command(app_cmd_buf, cmd_len);
            break;

        case CMD_ID_RFSDK:
            handle_rfsdk_command(app_cmd_buf, cmd_len);
            break;

        case CMD_ID_TXADV:
            handle_txadv_command(app_cmd_buf, cmd_len);
            break;

        case CMD_ID_CONN:
            handle_conn_command(app_cmd_buf, cmd_len);
            break;

        case CMD_ID_TXDATA:
            handle_txdata_command(app_cmd_buf, cmd_len);
            break;

        case CMD_ID_GPIOEVT:
            handle_gpioevt_command(app_cmd_buf, cmd_len);
            break;

        default:
            break;
    }
}

static bStatus_t start_service_discovery(void)
{
    s_conn.service_start_handle = INVALID_ATT_HANDLE;
    s_conn.service_end_handle = INVALID_ATT_HANDLE;
    s_conn.notify_handle = INVALID_ATT_HANDLE;
    s_conn.cccd_handle = INVALID_ATT_HANDLE;
    s_conn.notify_ready = FALSE;
    s_conn.discovery_state = CONN_DISC_SERVICE;

    return GATT_DiscPrimaryServiceByUUID(s_conn.conn_handle,
                                         (uint8_t *)s_remote_service_uuid,
                                         sizeof(s_remote_service_uuid),
                                         ObserverTaskId);
}

static bStatus_t start_notify_char_discovery(void)
{
    attReadByTypeReq_t req;

    memset(&req, 0, sizeof(req));
    req.startHandle = s_conn.service_start_handle;
    req.endHandle = s_conn.service_end_handle;
    req.type.len = sizeof(s_remote_notify_uuid);
    memcpy(req.type.uuid, s_remote_notify_uuid, sizeof(s_remote_notify_uuid));

    s_conn.discovery_state = CONN_DISC_NOTIFY_CHAR;
    return GATT_DiscCharsByUUID(s_conn.conn_handle, &req, ObserverTaskId);
}

static bStatus_t start_notify_cccd_discovery(void)
{
    if(s_conn.notify_handle == INVALID_ATT_HANDLE || s_conn.service_end_handle <= s_conn.notify_handle)
    {
        return INVALIDPARAMETER;
    }

    s_conn.discovery_state = CONN_DISC_NOTIFY_CCCD;
    return GATT_DiscAllCharDescs(s_conn.conn_handle,
                                 s_conn.notify_handle + 1,
                                 s_conn.service_end_handle,
                                 ObserverTaskId);
}

static bStatus_t enable_notify_cccd(void)
{
    attWriteReq_t req;

    if(s_conn.cccd_handle == INVALID_ATT_HANDLE)
    {
        return INVALIDPARAMETER;
    }

    memset(&req, 0, sizeof(req));
    req.handle = s_conn.cccd_handle;
    req.len = sizeof(s_cccd_enable_value);
    req.pValue = s_cccd_enable_value;
    req.sig = 0;
    req.cmd = 0;

    s_conn.discovery_state = CONN_DISC_ENABLE_CCCD;
    return GATT_WriteCharDesc(s_conn.conn_handle, &req, ObserverTaskId);
}

static void handle_gatt_msg(gattMsgEvent_t *pEvent)
{
    bool procedure_complete = (pEvent->hdr.status == bleProcedureComplete || pEvent->hdr.status == bleTimeout);

    if(pEvent->connHandle != s_conn.conn_handle)
    {
        return;
    }

    switch(pEvent->method)
    {
        case ATT_MTU_UPDATED_EVENT:
            break;

        case ATT_HANDLE_VALUE_NOTI:
            send_rxdata_response(ATT_HANDLE_VALUE_NOTI,
                                 pEvent->msg.handleValueNoti.handle,
                                 pEvent->msg.handleValueNoti.pValue,
                                 pEvent->msg.handleValueNoti.len);
            break;

        case ATT_HANDLE_VALUE_IND:
            send_rxdata_response(ATT_HANDLE_VALUE_IND,
                                 pEvent->msg.handleValueInd.handle,
                                 pEvent->msg.handleValueInd.pValue,
                                 pEvent->msg.handleValueInd.len);
            break;

        case ATT_FIND_BY_TYPE_VALUE_RSP:
            if(s_conn.discovery_state == CONN_DISC_SERVICE && pEvent->msg.findByTypeValueRsp.numInfo != 0)
            {
                const uint8_t *handles = pEvent->msg.findByTypeValueRsp.pHandlesInfo;

                s_conn.service_start_handle = (uint16_t)(handles[0] | (handles[1] << 8));
                s_conn.service_end_handle = (uint16_t)(handles[2] | (handles[3] << 8));
            }
            if(s_conn.discovery_state == CONN_DISC_SERVICE && procedure_complete)
            {
                if(s_conn.service_start_handle != INVALID_ATT_HANDLE && s_conn.service_end_handle != INVALID_ATT_HANDLE)
                {
                    if(start_notify_char_discovery() != SUCCESS)
                    {
                        s_conn.discovery_state = CONN_DISC_IDLE;
                    }
                }
                else
                {
                    s_conn.discovery_state = CONN_DISC_IDLE;
                }
            }
            break;

        case ATT_READ_BY_TYPE_RSP:
            if(s_conn.discovery_state == CONN_DISC_NOTIFY_CHAR &&
               pEvent->msg.readByTypeRsp.numPairs != 0 &&
               pEvent->msg.readByTypeRsp.len >= 7)
            {
                const uint8_t *data = pEvent->msg.readByTypeRsp.pDataList;
                s_conn.notify_handle = (uint16_t)(data[3] | (data[4] << 8));
            }
            if(s_conn.discovery_state == CONN_DISC_NOTIFY_CHAR && procedure_complete)
            {
                if(s_conn.notify_handle != INVALID_ATT_HANDLE)
                {
                    if(start_notify_cccd_discovery() != SUCCESS)
                    {
                        s_conn.discovery_state = CONN_DISC_IDLE;
                    }
                }
                else
                {
                    s_conn.discovery_state = CONN_DISC_IDLE;
                }
            }
            break;

        case ATT_FIND_INFO_RSP:
            if(s_conn.discovery_state == CONN_DISC_NOTIFY_CCCD && pEvent->msg.findInfoRsp.format == 0x01)
            {
                uint16_t index;
                const uint8_t *info = pEvent->msg.findInfoRsp.pInfo;

                for(index = 0; index < pEvent->msg.findInfoRsp.numInfo; index++)
                {
                    uint16_t handle = (uint16_t)(info[index * 4] | (info[index * 4 + 1] << 8));
                    uint16_t uuid = (uint16_t)(info[index * 4 + 2] | (info[index * 4 + 3] << 8));

                    if(uuid == 0x2902)
                    {
                        s_conn.cccd_handle = handle;
                        break;
                    }
                }
            }
            if(s_conn.discovery_state == CONN_DISC_NOTIFY_CCCD && procedure_complete)
            {
                if(s_conn.cccd_handle != INVALID_ATT_HANDLE)
                {
                    if(enable_notify_cccd() != SUCCESS)
                    {
                        s_conn.discovery_state = CONN_DISC_IDLE;
                    }
                }
                else
                {
                    s_conn.discovery_state = CONN_DISC_IDLE;
                }
            }
            break;

        case ATT_WRITE_RSP:
            if(s_conn.discovery_state == CONN_DISC_ENABLE_CCCD)
            {
                s_conn.notify_ready = TRUE;
                s_conn.discovery_state = CONN_DISC_IDLE;
            }
            break;

        case ATT_ERROR_RSP:
            if(s_conn.discovery_state == CONN_DISC_ENABLE_CCCD)
            {
                s_conn.notify_ready = FALSE;
            }
            s_conn.discovery_state = CONN_DISC_IDLE;
            break;

        default:
            break;
    }
}

static void free_gatt_payload(gattMsgEvent_t *pEvent)
{
    switch(pEvent->method)
    {
        case ATT_ERROR_RSP:
        case ATT_FIND_BY_TYPE_VALUE_RSP:
        case ATT_READ_BY_TYPE_RSP:
        case ATT_FIND_INFO_RSP:
        case ATT_HANDLE_VALUE_NOTI:
        case ATT_HANDLE_VALUE_IND:
            GATT_bm_free(&pEvent->msg, pEvent->method);
            break;

        default:
            break;
    }
}

uint16_t Observer_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & NEW_BLEDATA_EVT)
    {
        if(USBSendData() != SUCCESS)
        {
            tmos_start_task(ObserverTaskId, NEW_BLEDATA_EVT, 2);
        }
        else if(app_drv_fifo_length(&app_tx_fifo))
        {
            tmos_start_task(ObserverTaskId, NEW_BLEDATA_EVT, 1);
        }
        return (events ^ NEW_BLEDATA_EVT);
    }

    if(events & NEW_USBDATA_EVT)
    {
        if(app_cmd_len)
        {
            parse_command();
        }
        return (events ^ NEW_USBDATA_EVT);
    }

    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(task_id)) != NULL)
        {
            Observer_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
            tmos_msg_deallocate(pMsg);
        }

        return (events ^ SYS_EVENT_MSG);
    }

    if(events & START_DEVICE_EVT)
    {
        GAPRole_BroadcasterStartDevice((gapRolesBroadcasterCBs_t *)&BroadcasterRoleCB);
        GAPRole_ObserverStartDevice((gapRoleObserverCB_t *)&ObserverRoleCB);
        GAPRole_CentralStartDevice(ObserverTaskId, NULL, (gapCentralRoleCB_t *)&CentralRoleCB);
        return (events ^ START_DEVICE_EVT);
    }

    if(events & STOP_DISCOVERY_EVT)
    {
        stop_scan_now();
        return (events ^ STOP_DISCOVERY_EVT);
    }

    return 0;
}

static void Observer_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        case GATT_MSG_EVENT:
            handle_gatt_msg((gattMsgEvent_t *)pMsg);
            free_gatt_payload((gattMsgEvent_t *)pMsg);
            break;

        default:
            break;
    }
}

static void BroadcasterStateCB(gapRole_States_t newState)
{
    if((newState & GAPROLE_STATE_ADV_MASK) != GAPROLE_ADVERTISING)
    {
        uint8_t enabled = FALSE;
        GAPRole_GetParameter(GAPROLE_ADVERT_ENABLED, &enabled);
        if(!enabled)
        {
            s_txadv.running_phy = TXADV_RUNTIME_STOPPED;
        }
    }
}

static void ObserverEventCB(gapRoleEvent_t *pEvent)
{
    switch(pEvent->gap.opcode)
    {
        case GAP_DEVICE_INIT_DONE_EVENT:
            refresh_scan_params();
            break;

        case GAP_DEVICE_INFO_EVENT:
            if(should_accept_adv(pEvent->deviceInfo.addr, pEvent->deviceInfo.addrType))
            {
                send_adv_report(pEvent->deviceInfo.rssi,
                                pEvent->deviceInfo.eventType,
                                pEvent->deviceInfo.addrType & 0x0F,
                                GAP_PHY_VAL_LE_1M | (GAP_PHY_VAL_LE_1M << 4),
                                pEvent->deviceInfo.addr,
                                pEvent->deviceInfo.pEvtData,
                                pEvent->deviceInfo.dataLen);
            }
            break;

        case GAP_DIRECT_DEVICE_INFO_EVENT:
            if(should_accept_adv(pEvent->deviceDirectInfo.addr, pEvent->deviceDirectInfo.addrType))
            {
                uint8_t addr_type = (pEvent->deviceDirectInfo.addrType & 0x0F) |
                                    ((pEvent->deviceDirectInfo.directAddrType & 0x0F) << 4);
                send_adv_report(pEvent->deviceDirectInfo.rssi,
                                pEvent->deviceDirectInfo.eventType,
                                addr_type,
                                GAP_PHY_VAL_LE_1M | (GAP_PHY_VAL_LE_1M << 4),
                                pEvent->deviceDirectInfo.addr,
                                NULL,
                                0);
            }
            break;

        case GAP_EXT_ADV_DEVICE_INFO_EVENT:
            if(should_accept_adv(pEvent->deviceExtAdvInfo.addr, pEvent->deviceExtAdvInfo.addrType))
            {
                uint8_t addr_type = (pEvent->deviceExtAdvInfo.addrType & 0x0F) |
                                    ((pEvent->deviceExtAdvInfo.directAddressType & 0x0F) << 4);
                uint8_t phy = (pEvent->deviceExtAdvInfo.primaryPHY & 0x0F) |
                              ((pEvent->deviceExtAdvInfo.secondaryPHY & 0x0F) << 4);
                send_adv_report(pEvent->deviceExtAdvInfo.rssi,
                                pEvent->deviceExtAdvInfo.eventType,
                                addr_type,
                                phy,
                                pEvent->deviceExtAdvInfo.addr,
                                pEvent->deviceExtAdvInfo.pEvtData,
                                pEvent->deviceExtAdvInfo.dataLen);
            }
            break;

        case GAP_DEVICE_DISCOVERY_EVENT:
            s_scan_running = FALSE;
            if(s_conn.wait_for_scan_cancel)
            {
                bStatus_t ble_status;

                s_conn.wait_for_scan_cancel = FALSE;
                ble_status = begin_connection_request();
                if(ble_status != SUCCESS)
                {
                    clear_conn_runtime();
                    start_scan_if_enabled();
                    send_conn_response(CMD_STATUS_DENIED);
                }
                break;
            }
            start_scan_if_enabled();
            break;

        default:
            break;
    }
}

static void CentralEventCB(gapRoleEvent_t *pEvent)
{
    switch(pEvent->gap.opcode)
    {
        case GAP_LINK_ESTABLISHED_EVENT:
            if(pEvent->linkCmpl.hdr.status == SUCCESS)
            {
                s_conn.state = CONN_STATE_CONNECTED;
                s_conn.conn_handle = pEvent->linkCmpl.connectionHandle;
                s_conn.interval_units = pEvent->linkCmpl.connInterval;
                s_conn.peer_addr_type = pEvent->linkCmpl.devAddrType;
                memcpy(s_conn.peer_addr, pEvent->linkCmpl.devAddr, B_ADDR_LEN);

                send_conn_response(CMD_STATUS_OK);

                if(s_conn.pending_phy_bits == GAP_PHY_BIT_LE_CODED)
                {
                    GAPRole_UpdatePHY(s_conn.conn_handle,
                                      0,
                                      GAP_PHY_BIT_LE_CODED,
                                      GAP_PHY_BIT_LE_CODED,
                                      GAP_PHY_OPTIONS_S8);
                }

                if(start_service_discovery() != SUCCESS)
                {
                    s_conn.discovery_state = CONN_DISC_IDLE;
                }
            }
            else
            {
                clear_conn_runtime();
                start_scan_if_enabled();
                send_conn_response(CMD_STATUS_DENIED);
            }
            break;

        case GAP_LINK_PARAM_UPDATE_EVENT:
            if(s_conn.conn_handle == pEvent->linkUpdate.connectionHandle)
            {
                s_conn.interval_units = pEvent->linkUpdate.connInterval;
                send_conn_response(CMD_STATUS_OK);
            }
            break;

        case GAP_LINK_TERMINATED_EVENT:
            clear_conn_runtime();
            send_conn_response(CMD_STATUS_OK);
            start_scan_if_enabled();
            break;

        default:
            break;
    }
}

/*********************************************************************
*********************************************************************/