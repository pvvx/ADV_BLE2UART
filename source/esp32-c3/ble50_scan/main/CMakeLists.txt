idf_component_register(SRCS "ble50_scan.c"
                    "crc.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES bt nvs_flash esp_adc esp_driver_gpio esp_driver_ledc esp_driver_usb_serial_jtag)
