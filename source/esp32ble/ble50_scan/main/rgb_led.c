/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal WS2812 (RGB LED) driver using RMT.
 * Adapted from ESP-IDF led_strip example.
 */

#include "rgb_led.h"
#include "esp_check.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "rgb_led";

#define RMT_RESOLUTION_HZ       10000000  // 10 MHz, 1 tick = 0.1 µs
#define RMT_TRANS_QUEUE_DEPTH   4
#define RMT_MEM_BLOCK_SYMBOLS   64

static rmt_channel_handle_t s_led_chan = NULL;
static rmt_encoder_handle_t s_led_encoder = NULL;
static uint8_t s_pixel_data[3];   // G, R, B order for WS2812

/* ── WS2812 encoder (adapted from ESP-IDF led_strip_encoder.c) ──────────── */

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

RMT_ENCODER_FUNC_ATTR
static size_t rmt_encode_led_strip(
    rmt_encoder_t *encoder, rmt_channel_handle_t channel,
    const void *primary_data, size_t data_size,
    rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t rmt_new_led_strip_encoder(uint32_t resolution, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;

    led_encoder = rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
    ESP_RETURN_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, TAG, "no mem for led strip encoder");

    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;

    // WS2812 timing (0.1 µs ticks)
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 3,   // T0H = 0.3 µs
            .level1 = 0,
            .duration1 = 9,   // T0L = 0.9 µs
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 9,   // T1H = 0.9 µs
            .level1 = 0,
            .duration1 = 3,   // T1L = 0.3 µs
        },
        .flags.msb_first = 1, // WS2812: G7..G0 R7..R0 B7..B0
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder),
                      err, TAG, "create bytes encoder failed");

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder),
                      err, TAG, "create copy encoder failed");

    uint32_t reset_ticks = resolution / 1000000 * 50 / 2; // 50 µs reset
    led_encoder->reset_code = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };

    *ret_encoder = &led_encoder->base;
    return ESP_OK;

err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) rmt_del_encoder(led_encoder->bytes_encoder);
        if (led_encoder->copy_encoder) rmt_del_encoder(led_encoder->copy_encoder);
        free(led_encoder);
    }
    return ret;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

esp_err_t rgb_led_init(gpio_num_t gpio)
{
    esp_err_t ret;

    // RMT TX channel
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio,
        .mem_block_symbols = RMT_MEM_BLOCK_SYMBOLS,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = RMT_TRANS_QUEUE_DEPTH,
    };
    ret = rmt_new_tx_channel(&tx_chan_config, &s_led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "create RMT TX channel on GPIO%d failed: %s", gpio, esp_err_to_name(ret));
        return ret;
    }
    ret = rmt_enable(s_led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "enable RMT channel on GPIO%d failed: %s", gpio, esp_err_to_name(ret));
        return ret;
    }

    // LED strip encoder
    ret = rmt_new_led_strip_encoder(RMT_RESOLUTION_HZ, &s_led_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "create LED encoder on GPIO%d failed: %s", gpio, esp_err_to_name(ret));
        return ret;
    }

    // Start with LED off
    return rgb_led_off();
}

esp_err_t rgb_led_set(uint8_t red, uint8_t green, uint8_t blue)
{
    // WS2812 uses GRB order
    s_pixel_data[0] = green;
    s_pixel_data[1] = red;
    s_pixel_data[2] = blue;

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    esp_err_t err = rmt_transmit(s_led_chan, s_led_encoder, s_pixel_data, sizeof(s_pixel_data), &tx_config);
    if (err != ESP_OK) {
        return err;
    }
    // Timeout: 500 ms – more than enough for a 30 µs WS2812 frame, but prevents
    // a permanent hang if the RMT peripheral stalls for any reason.
    err = rmt_tx_wait_all_done(s_led_chan, pdMS_TO_TICKS(500));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "RMT tx wait timeout – recovering");
        rmt_disable(s_led_chan);
        rmt_enable(s_led_chan);
    }
    return ESP_OK;
}

esp_err_t rgb_led_off(void)
{
    return rgb_led_set(0, 0, 0);
}
