#pragma once

#include <stdbool.h>

typedef struct {
    const char *name;
    int gpio;
    bool active_low;
    bool enable_pullup;
} bsp_input_button_config_t;

const bsp_input_button_config_t *bsp_input_get_key_button_config(void);
