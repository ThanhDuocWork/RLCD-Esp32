#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t storage_port_mount(void);
esp_err_t storage_port_unmount(void);
bool storage_port_is_mounted(void);
const char *storage_port_get_mount_point(void);
