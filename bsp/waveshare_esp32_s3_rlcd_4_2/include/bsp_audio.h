#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    const char *codec_name;
    int i2s_port_id;
    int i2c_port_id;
    int i2c_sda_gpio;
    int i2c_scl_gpio;
    uint32_t i2c_clock_hz;
    uint8_t codec_i2c_addr;
    int mclk_gpio;
    int bclk_gpio;
    int ws_gpio;
    int dout_gpio;
    int din_gpio;
    int pa_enable_gpio;
    uint32_t sample_rate_hz;
} bsp_audio_codec_config_t;

esp_err_t bsp_audio_init(void);
const bsp_audio_codec_config_t *bsp_audio_get_codec_config(void);
esp_err_t bsp_audio_set_pa_enabled(bool enable);
