#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct {
    i2c_master_dev_handle_t i2c_handle;
    uint32_t sample_rate_hz;
    uint32_t mclk_hz;
    uint8_t bits_per_sample;
    uint8_t default_volume;
    bool use_mic;
} es8311_config_t;

esp_err_t es8311_init(const es8311_config_t *config);
esp_err_t es8311_set_volume(uint8_t volume);
esp_err_t es8311_set_mute(bool mute);
esp_err_t es8311_read_reg(uint8_t reg, uint8_t *value);
