/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the WS2812 RGB LED on the given GPIO.
 *
 * Sets up the RMT TX channel and encoder for the WS2812 LED on the
 * specified GPIO pin. Must be called once before rgb_led_set().
 *
 * @param gpio GPIO number for the RGB LED data line.
 * @return ESP_OK on success.
 */
esp_err_t rgb_led_init(gpio_num_t gpio);

/**
 * @brief Set the RGB LED colour.
 *
 * @param red   Red component (0-255)
 * @param green Green component (0-255)
 * @param blue  Blue component (0-255)
 * @return ESP_OK on success
 */
esp_err_t rgb_led_set(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Turn the RGB LED off.
 */
esp_err_t rgb_led_off(void);

#ifdef __cplusplus
}
#endif
