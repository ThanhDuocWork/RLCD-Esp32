#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef enum {
    INPUT_SERVICE_BUTTON_KEY = 1,
} input_service_button_t;

typedef struct {
    input_service_button_t button;
    uint32_t click_count;
} input_service_key_event_t;

esp_err_t input_service_init(void);
