#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "board_config.h"
#include "board_features.h"
#include "bsp_audio.h"
#include "bsp_display.h"
#include "esp_err.h"

typedef bsp_display_info_t board_display_info_t;
typedef bsp_audio_codec_config_t board_audio_codec_config_t;

esp_err_t board_init(void);
const board_config_t *board_get_config(void);
const board_features_t *board_get_features(void);
board_display_info_t board_get_display_info(void);
esp_err_t board_display_set_power(bool enable);
const board_audio_codec_config_t *board_get_audio_codec_config(void);
esp_err_t board_audio_set_pa_enabled(bool enable);
