#include "board.h"

#include "board_features.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "waveshare_bsp";
static const board_config_t *s_board_config;

esp_err_t board_init(void)
{
    s_board_config = board_config_get_default();
    ESP_RETURN_ON_FALSE(s_board_config != NULL, ESP_ERR_INVALID_STATE, TAG, "Board config is not available");

    ESP_LOGI(TAG,
             "Board ready: %s %ux%u",
             s_board_config->lcd.controller_name,
             s_board_config->lcd.h_res,
             s_board_config->lcd.v_res);
    ESP_LOGI(TAG, "Fill real GPIO mapping in bsp/waveshare_esp32_s3_rlcd_4_2/src/board_config.c");
    return ESP_OK;
}

const board_config_t *board_get_config(void)
{
    return board_config_get_default();
}

const board_features_t *board_get_features(void)
{
    return board_features_get();
}

board_display_info_t board_get_display_info(void)
{
    const board_config_t *cfg = board_config_get_default();

    return (board_display_info_t) {
        .controller_name = (cfg != NULL) ? cfg->lcd.controller_name : "unknown",
        .width = (cfg != NULL) ? cfg->lcd.h_res : 0,
        .height = (cfg != NULL) ? cfg->lcd.v_res : 0,
        .has_power_gate = (cfg != NULL) ? (cfg->lcd.power_gpio >= 0) : false,
    };
}

esp_err_t board_display_set_power(bool enable)
{
    const board_config_t *cfg = board_get_config();

    if ((cfg == NULL) || (cfg->lcd.power_gpio < 0)) {
        return ESP_OK;
    }

    return gpio_set_level(cfg->lcd.power_gpio, enable ? 1 : 0);
}
