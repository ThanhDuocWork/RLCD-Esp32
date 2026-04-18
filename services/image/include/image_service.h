#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    uint8_t threshold;
    bool dither;
    bool invert;
} image_service_mono_config_t;

esp_err_t image_service_show_bmp_1bpp(const char *path, const image_service_mono_config_t *config);
