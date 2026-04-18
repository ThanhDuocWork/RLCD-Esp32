#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t storage_service_init(void);
bool storage_service_is_ready(void);
const char *storage_service_get_mount_point(void);
