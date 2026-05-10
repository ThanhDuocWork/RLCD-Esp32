#include "i2c_master_driver.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "i2c_master_drv";
static i2c_master_bus_handle_t s_bus_handle;
static i2c_port_num_t s_port_id = -1;
static bool s_ready;

esp_err_t i2c_master_driver_init(const bus_i2c_master_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is null");

    if (s_ready) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_cfg = {
        .i2c_port = config->port_id,
        .sda_io_num = config->sda_gpio,
        .scl_io_num = config->scl_gpio,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = config->enable_internal_pullup ? 1U : 0U,
            .allow_pd = 0U,
        },
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus_handle), TAG, "Create I2C master bus failed");
    s_port_id = config->port_id;
    s_ready = true;
    ESP_LOGI(TAG,
             "I2C bus ready: port=%d sda=%d scl=%d hz=%u",
             (int)config->port_id,
             config->sda_gpio,
             config->scl_gpio,
             (unsigned)config->clock_speed_hz);
    return ESP_OK;
}

esp_err_t i2c_master_driver_add_device(const bus_i2c_master_device_config_t *config, i2c_master_dev_handle_t *out_handle)
{
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->device_address,
        .scl_speed_hz = config->scl_speed_hz,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = 0U,
        },
    };

    ESP_RETURN_ON_FALSE(s_ready, ESP_ERR_INVALID_STATE, TAG, "I2C bus is not ready");
    ESP_RETURN_ON_FALSE(config != NULL && out_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid add-device args");
    return i2c_master_bus_add_device(s_bus_handle, &dev_cfg, out_handle);
}

esp_err_t i2c_master_driver_probe(uint16_t device_address, int timeout_ms)
{
    ESP_RETURN_ON_FALSE(s_ready, ESP_ERR_INVALID_STATE, TAG, "I2C bus is not ready");
    return i2c_master_probe(s_bus_handle, device_address, timeout_ms);
}

esp_err_t i2c_master_driver_write_reg8(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t value, int timeout_ms)
{
    uint8_t payload[2] = { reg, value };

    ESP_RETURN_ON_FALSE(handle != NULL, ESP_ERR_INVALID_ARG, TAG, "I2C device handle is null");
    return i2c_master_transmit(handle, payload, sizeof(payload), timeout_ms);
}

esp_err_t i2c_master_driver_write(i2c_master_dev_handle_t handle, const uint8_t *data, size_t data_size, int timeout_ms)
{
    ESP_RETURN_ON_FALSE(handle != NULL && data != NULL && data_size > 0, ESP_ERR_INVALID_ARG, TAG, "invalid write args");
    return i2c_master_transmit(handle, data, data_size, timeout_ms);
}

esp_err_t i2c_master_driver_read_reg8(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t *value, int timeout_ms)
{
    ESP_RETURN_ON_FALSE(handle != NULL && value != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid read args");
    return i2c_master_transmit_receive(handle, &reg, sizeof(reg), value, 1, timeout_ms);
}
