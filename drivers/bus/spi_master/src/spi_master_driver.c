#include "spi_master_driver.h"

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "spi_master_drv";
static bool s_spi2_ready;
static bool s_spi3_ready;

static bool *host_flag(spi_host_device_t host_id)
{
    switch (host_id) {
    case SPI2_HOST:
        return &s_spi2_ready;
    case SPI3_HOST:
        return &s_spi3_ready;
    default:
        return NULL;
    }
}

esp_err_t spi_master_driver_init(const bus_spi_master_config_t *config)
{
    bool *ready_flag = NULL;

    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is null");
    ESP_RETURN_ON_FALSE(config->sclk_gpio >= 0, ESP_ERR_INVALID_ARG, TAG, "sclk gpio is not configured");
    ESP_RETURN_ON_FALSE(config->mosi_gpio >= 0, ESP_ERR_INVALID_ARG, TAG, "mosi gpio is not configured");

    ready_flag = host_flag(config->host_id);
    ESP_RETURN_ON_FALSE(ready_flag != NULL, ESP_ERR_NOT_SUPPORTED, TAG, "unsupported host");

    if (*ready_flag) {
        return ESP_OK;
    }

    const spi_bus_config_t bus_config = {
        .sclk_io_num = config->sclk_gpio,
        .mosi_io_num = config->mosi_gpio,
        .miso_io_num = config->miso_gpio,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = config->max_transfer_sz,
    };

    ESP_RETURN_ON_ERROR(spi_bus_initialize(config->host_id, &bus_config, SPI_DMA_CH_AUTO),
                        TAG,
                        "spi_bus_initialize failed");
    *ready_flag = true;
    ESP_LOGI(TAG, "SPI host %d ready", config->host_id);
    return ESP_OK;
}

bool spi_master_driver_is_ready(spi_host_device_t host_id)
{
    bool *ready_flag = host_flag(host_id);
    return (ready_flag != NULL) ? *ready_flag : false;
}
