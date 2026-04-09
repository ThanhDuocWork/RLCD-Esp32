#include "display_port.h"

#include "board.h"
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
    const board_config_t *board_config = board_get_config();
    const board_display_info_t display_info = board_get_display_info();

    ESP_RETURN_ON_FALSE(board_config != NULL, ESP_ERR_INVALID_STATE, TAG, "Board config is not available");
    ESP_RETURN_ON_FALSE(display_info.width > 0, ESP_ERR_INVALID_STATE, TAG, "Display width is invalid");

    const st7305_config_t panel_config = {
        .host_id = (spi_host_device_t)board_config->lcd.spi_host_id,
        .sclk_gpio = board_config->lcd.sclk_gpio,
        .mosi_gpio = board_config->lcd.mosi_gpio,
        .miso_gpio = board_config->lcd.miso_gpio,
        .cs_gpio = board_config->lcd.cs_gpio,
        .dc_gpio = board_config->lcd.dc_gpio,
        .busy_gpio = board_config->lcd.busy_gpio,
        .reset_gpio = board_config->lcd.reset_gpio,
        .width = board_config->lcd.h_res,
        .height = board_config->lcd.v_res,
        .pixel_clock_hz = board_config->lcd.spi_clock_hz,
        .color_invert = board_config->lcd.color_invert,
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
    return board_display_set_power(enable);
}

esp_err_t display_port_fill_screen(uint16_t rgb444_color)
{
    ESP_RETURN_ON_FALSE(s_port_ready, ESP_ERR_INVALID_STATE, TAG, "Display port not ready");
    return st7305_fill_color(rgb444_color);
}

esp_err_t display_port_draw_test_pattern(void)
{
    ESP_RETURN_ON_FALSE(s_port_ready, ESP_ERR_INVALID_STATE, TAG, "Display port not ready");

    ESP_LOGI(TAG, "Draw simple test pattern");
    ESP_RETURN_ON_ERROR(st7305_fill_color(0xFFF), TAG, "Fill white failed");
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_RETURN_ON_ERROR(st7305_fill_color(0x000), TAG, "Fill black failed");
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_RETURN_ON_ERROR(st7305_fill_color(0xF00), TAG, "Fill red failed");
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_RETURN_ON_ERROR(st7305_fill_color(0x0F0), TAG, "Fill green failed");
    vTaskDelay(pdMS_TO_TICKS(300));
    return st7305_fill_color(0x00F);
}

display_port_size_t display_port_get_size(void)
{
    return s_size;
}

bool display_port_is_ready(void)
{
    return s_port_ready;
}
