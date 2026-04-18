#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t display_service_init(void);
esp_err_t display_service_show_boot(void);
esp_err_t display_service_show_bitmap_1bpp(const uint8_t *bitmap, uint16_t width, uint16_t height, bool invert);
bool display_service_is_ready(void);
