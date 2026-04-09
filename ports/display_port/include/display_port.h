#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    uint16_t width;
    uint16_t height;
} display_port_size_t;

esp_err_t display_port_init(void);
esp_err_t display_port_set_power(bool enable);
esp_err_t display_port_fill_screen(uint16_t rgb444_color);
esp_err_t display_port_draw_test_pattern(void);
display_port_size_t display_port_get_size(void);
bool display_port_is_ready(void);
