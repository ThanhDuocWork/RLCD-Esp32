#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t input_port_init(void);
bool input_port_is_key_pressed(void);
