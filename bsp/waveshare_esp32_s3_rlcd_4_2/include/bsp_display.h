#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

typedef struct {
    const char *controller_name;
    spi_host_device_t spi_host_id;
    int spi_mode;
    int sclk_gpio;
    int mosi_gpio;
    int miso_gpio;
    int cs_gpio;
    int dc_gpio;
    int busy_gpio;
    int reset_gpio;
    int power_gpio;
    uint32_t spi_clock_hz;
    uint16_t width;
    uint16_t height;
    uint8_t column_start;
    uint8_t column_end;
    uint8_t page_start;
    uint8_t page_end;
    bool color_invert;
} bsp_display_panel_config_t;

typedef struct {
    const char *controller_name;
    uint16_t width;
    uint16_t height;
    bool has_power_gate;
} bsp_display_info_t;

esp_err_t bsp_display_init(void);
const bsp_display_panel_config_t *bsp_display_get_panel_config(void);
bsp_display_info_t bsp_display_get_info(void);
esp_err_t bsp_display_new_spi_device(spi_device_handle_t *out_dev);
esp_err_t bsp_display_reset_panel(void);
esp_err_t bsp_display_set_power(bool enable);
