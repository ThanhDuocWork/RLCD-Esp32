#include "display_port.h"

#include "bsp_display.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7305.h"

static const char *TAG = "display_port";
static bool s_port_ready;
static display_port_size_t s_size;

esp_err_t display_port_init(void)
{
    const bsp_display_panel_config_t *panel_cfg = bsp_display_get_panel_config();
    const bsp_display_info_t display_info = bsp_display_get_info();
    spi_device_handle_t spi_dev = NULL;

    ESP_RETURN_ON_FALSE(panel_cfg != NULL, ESP_ERR_INVALID_STATE, TAG, "BSP display config is not available");
    ESP_RETURN_ON_FALSE(display_info.width > 0, ESP_ERR_INVALID_STATE, TAG, "Display width is invalid");
    ESP_RETURN_ON_ERROR(bsp_display_init(), TAG, "BSP display init failed");
    ESP_RETURN_ON_ERROR(bsp_display_new_spi_device(&spi_dev), TAG, "Create SPI device failed");
    ESP_RETURN_ON_ERROR(bsp_display_reset_panel(), TAG, "Panel reset failed");

    const st7305_config_t panel_config = {
        .spi_dev = spi_dev,
        .dc_gpio = panel_cfg->dc_gpio,
        .busy_gpio = panel_cfg->busy_gpio,
        .width = panel_cfg->width,
        .height = panel_cfg->height,
        .column_start = panel_cfg->column_start,
        .column_end = panel_cfg->column_end,
        .page_start = panel_cfg->page_start,
        .page_end = panel_cfg->page_end,
        .color_invert = panel_cfg->color_invert,
    };

    ESP_LOGI(TAG, "Initialize display port for %s", display_info.controller_name);
    ESP_RETURN_ON_ERROR(st7305_init(&panel_config), TAG, "ST7305 init failed");
    s_size.width = display_info.width;
    s_size.height = display_info.height;
    s_port_ready = true;
    return ESP_OK;
}

esp_err_t display_port_set_power(bool enable)
{
    return bsp_display_set_power(enable);
}

esp_err_t display_port_fill_screen(uint16_t rgb444_color)
{
    uint8_t pattern = (rgb444_color == 0U) ? 0x00 : 0xFF;

    ESP_RETURN_ON_FALSE(s_port_ready, ESP_ERR_INVALID_STATE, TAG, "Display port not ready");
    return st7305_fill_raw_pattern(pattern);
}

esp_err_t display_port_draw_test_pattern(void)
{
    ESP_RETURN_ON_FALSE(s_port_ready, ESP_ERR_INVALID_STATE, TAG, "Display port not ready");

    ESP_LOGI(TAG, "Draw structured ST7305 test pattern");
    ESP_RETURN_ON_ERROR(st7305_fill_raw_pattern(0x00), TAG, "Fill dark failed");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_RETURN_ON_ERROR(st7305_fill_raw_pattern(0xFF), TAG, "Fill bright failed");
    vTaskDelay(pdMS_TO_TICKS(500));
    return st7305_draw_test_pattern();
}

display_port_size_t display_port_get_size(void)
{
    return s_size;
}

bool display_port_is_ready(void)
{
    return s_port_ready;
}
