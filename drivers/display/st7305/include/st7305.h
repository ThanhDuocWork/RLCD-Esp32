#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"

typedef struct {
    spi_host_device_t host_id;
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
    bool color_invert;
} st7305_config_t;

esp_err_t st7305_init(const st7305_config_t *config);
esp_err_t st7305_fill_color(uint16_t rgb444_color);
esp_err_t st7305_display_on(bool enable);
esp_err_t st7305_set_invert(bool enable);
