#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"

typedef struct {
    spi_host_device_t host_id;
    int spi_mode;
    int sclk_gpio;
    int mosi_gpio;
    int miso_gpio;
    int cs_gpio;
    int dc_gpio;
    int busy_gpio;
    int reset_gpio;
    int width;
    int height;
    uint32_t pixel_clock_hz;
    uint8_t column_start;
    uint8_t column_end;
    uint8_t page_start;
    uint8_t page_end;
    bool color_invert;
} st7305_config_t;

esp_err_t st7305_init(const st7305_config_t *config);
esp_err_t st7305_fill_raw_pattern(uint8_t pattern_byte);
esp_err_t st7305_draw_test_pattern(void);
esp_err_t st7305_display_on(bool enable);
esp_err_t st7305_set_invert(bool enable);
