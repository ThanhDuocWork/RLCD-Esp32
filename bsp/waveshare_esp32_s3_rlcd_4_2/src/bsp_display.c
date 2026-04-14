#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_display.h"
#include "board_config.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "spi_master_driver.h"

static const char *TAG = "waveshare_bsp_display";
static const bsp_display_panel_config_t s_panel_config = {
    .controller_name = "ST7305",
    .spi_host_id = SPI2_HOST,
    .spi_mode = 0,
    .sclk_gpio = 11,
    .mosi_gpio = 12,
    .miso_gpio = -1,
    .cs_gpio = 40,
    .dc_gpio = 5,
    .busy_gpio = 6,
    .reset_gpio = 41,
    .power_gpio = -1,
    .spi_clock_hz = 10 * 1000 * 1000,
    .width = 300,
    .height = 400,
    .column_start = 0x12,
    .column_end = 0x2A,
    .page_start = 0x00,
    .page_end = 0xC7,
    .color_invert = false,
};
static bool s_bsp_display_ready;

static esp_err_t bsp_display_gpio_init(void)
{
    uint64_t pin_mask = 0;

    if (s_panel_config.dc_gpio >= 0) {
        pin_mask |= (1ULL << s_panel_config.dc_gpio);
    }
    if (s_panel_config.reset_gpio >= 0) {
        pin_mask |= (1ULL << s_panel_config.reset_gpio);
    }
    if (s_panel_config.power_gpio >= 0) {
        pin_mask |= (1ULL << s_panel_config.power_gpio);
    }

    if (pin_mask == 0) {
        return ESP_OK;
    }

    const gpio_config_t io_cfg = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
    };

    return gpio_config(&io_cfg);
}

esp_err_t bsp_display_init(void)
{
    const bus_spi_master_config_t spi_config = {
        .host_id = s_panel_config.spi_host_id,
        .sclk_gpio = s_panel_config.sclk_gpio,
        .mosi_gpio = s_panel_config.mosi_gpio,
        .miso_gpio = s_panel_config.miso_gpio,
        .max_transfer_sz = 384,
    };

    if (s_bsp_display_ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(spi_master_driver_init(&spi_config), TAG, "SPI bus init failed");
    ESP_RETURN_ON_ERROR(bsp_display_gpio_init(), TAG, "display gpio init failed");
    s_bsp_display_ready = true;
    return ESP_OK;
}

const bsp_display_panel_config_t *bsp_display_get_panel_config(void)
{
    return &s_panel_config;
}

bsp_display_info_t bsp_display_get_info(void)
{
    return (bsp_display_info_t) {
        .controller_name = s_panel_config.controller_name,
        .width = s_panel_config.width,
        .height = s_panel_config.height,
        .has_power_gate = s_panel_config.power_gpio >= 0,
    };
}

esp_err_t bsp_display_new_spi_device(spi_device_handle_t *out_dev)
{
    const spi_device_interface_config_t dev_config = {
        .clock_speed_hz = (int)s_panel_config.spi_clock_hz,
        .mode = s_panel_config.spi_mode,
        .spics_io_num = s_panel_config.cs_gpio,
        .queue_size = 4,
    };

    ESP_RETURN_ON_FALSE(out_dev != NULL, ESP_ERR_INVALID_ARG, TAG, "out_dev is null");
    ESP_RETURN_ON_ERROR(bsp_display_init(), TAG, "bsp display init failed");
    return spi_bus_add_device(s_panel_config.spi_host_id, &dev_config, out_dev);
}

esp_err_t bsp_display_reset_panel(void)
{
    if (s_panel_config.reset_gpio < 0) {
        return ESP_OK;
    }

    gpio_set_level(s_panel_config.reset_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(s_panel_config.reset_gpio, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(s_panel_config.reset_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    return ESP_OK;
}

esp_err_t bsp_display_set_power(bool enable)
{
    if (s_panel_config.power_gpio < 0) {
        return ESP_OK;
    }

    return gpio_set_level(s_panel_config.power_gpio, enable ? 1 : 0);
}

