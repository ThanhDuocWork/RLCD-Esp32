#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t display_service_init(void);
esp_err_t display_service_show_boot(void);
bool display_service_is_ready(void);
