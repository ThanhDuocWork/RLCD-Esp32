#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "board_config.h"
#include "board_features.h"
#include "esp_err.h"

typedef struct {
    const char *controller_name;
    uint16_t width;
    uint16_t height;
    bool has_power_gate;
} board_display_info_t;

esp_err_t board_init(void);
const board_config_t *board_get_config(void);
const board_features_t *board_get_features(void);
board_display_info_t board_get_display_info(void);
esp_err_t board_display_set_power(bool enable);
