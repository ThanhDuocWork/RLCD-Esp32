#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct {
    i2c_port_num_t port_id;
    int sda_gpio;
    int scl_gpio;
    uint32_t clock_speed_hz;
    bool enable_internal_pullup;
} bus_i2c_master_config_t;

typedef struct {
    uint16_t device_address;
    uint32_t scl_speed_hz;
} bus_i2c_master_device_config_t;

esp_err_t i2c_master_driver_init(const bus_i2c_master_config_t *config);
esp_err_t i2c_master_driver_add_device(const bus_i2c_master_device_config_t *config, i2c_master_dev_handle_t *out_handle);
esp_err_t i2c_master_driver_probe(uint16_t device_address, int timeout_ms);
esp_err_t i2c_master_driver_write_reg8(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t value, int timeout_ms);
esp_err_t i2c_master_driver_write(i2c_master_dev_handle_t handle, const uint8_t *data, size_t data_size, int timeout_ms);
esp_err_t i2c_master_driver_read_reg8(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t *value, int timeout_ms);
