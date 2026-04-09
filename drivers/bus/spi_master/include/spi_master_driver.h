#pragma once

#include <stdbool.h>

#include "driver/spi_master.h"
#include "esp_err.h"

typedef struct {
    spi_host_device_t host_id;
    int sclk_gpio;
    int mosi_gpio;
    int miso_gpio;
    int max_transfer_sz;
} bus_spi_master_config_t;

esp_err_t spi_master_driver_init(const bus_spi_master_config_t *config);
bool spi_master_driver_is_ready(spi_host_device_t host_id);
