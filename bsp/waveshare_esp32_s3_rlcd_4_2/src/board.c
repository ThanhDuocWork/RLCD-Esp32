#include "board.h"

#include "board_features.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "waveshare_bsp";
static const board_config_t *s_board_config;

esp_err_t board_init(void)
{
    s_board_config = board_config_get_default();
    ESP_RETURN_ON_FALSE(s_board_config != NULL, ESP_ERR_INVALID_STATE, TAG, "Board config is not available");
    ESP_RETURN_ON_ERROR(bsp_display_init(), TAG, "bsp display init failed");

    ESP_LOGI(TAG,
             "Board ready: %s %ux%u",
             s_board_config->lcd.controller_name,
             s_board_config->lcd.h_res,
             s_board_config->lcd.v_res);
    ESP_LOGI(TAG, "Board display path initialized by BSP");
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

const board_audio_codec_config_t *board_get_audio_codec_config(void)
{
    return bsp_audio_get_codec_config();
}

board_display_info_t board_get_display_info(void)
{
    return bsp_display_get_info();
}

esp_err_t board_display_set_power(bool enable)
{
    return bsp_display_set_power(enable);
}

esp_err_t board_audio_set_pa_enabled(bool enable)
{
    return bsp_audio_set_pa_enabled(enable);
}
