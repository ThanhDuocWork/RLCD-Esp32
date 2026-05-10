#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef enum {
    INPUT_SERVICE_BUTTON_KEY = 1,
} input_service_button_t;

typedef enum {
    INPUT_SERVICE_KEY_ACTION_SHORT_PRESS = 1,
    INPUT_SERVICE_KEY_ACTION_LONG_PRESS,
} input_service_key_action_t;

typedef struct {
    input_service_button_t button;
    input_service_key_action_t action;
    uint32_t click_count;
    uint32_t press_ms;
} input_service_key_event_t;

esp_err_t input_service_init(void);
