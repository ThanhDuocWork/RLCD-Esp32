#include "bsp_audio.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "waveshare_bsp_audio";

static const bsp_audio_codec_config_t s_audio_config = {
    .codec_name = "ES8311",
    .i2s_port_id = 0,
    .i2c_port_id = 0,
    .i2c_sda_gpio = 13,
    .i2c_scl_gpio = 14,
    .i2c_clock_hz = 100000,
    .codec_i2c_addr = 0x18,
    .mclk_gpio = 16,
    .bclk_gpio = 9,
    .ws_gpio = 45,
    .dout_gpio = 8,
    .din_gpio = 10,
    .pa_enable_gpio = 46,
    .sample_rate_hz = 16000,
};

static bool s_ready;

esp_err_t bsp_audio_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    if (s_audio_config.pa_enable_gpio >= 0) {
        const gpio_config_t io_cfg = {
            .pin_bit_mask = 1ULL << s_audio_config.pa_enable_gpio,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };

        ESP_RETURN_ON_ERROR(gpio_config(&io_cfg), TAG, "Configure PA enable GPIO failed");
        ESP_RETURN_ON_ERROR(gpio_set_level(s_audio_config.pa_enable_gpio, 0), TAG, "Default PA off failed");
    }

    s_ready = true;
    ESP_LOGI(TAG,
             "Audio path ready: codec=%s i2c=%d sda=%d scl=%d addr=0x%02x i2s=%d mclk=%d bclk=%d ws=%d dout=%d pa=%d",
             s_audio_config.codec_name,
             s_audio_config.i2c_port_id,
             s_audio_config.i2c_sda_gpio,
             s_audio_config.i2c_scl_gpio,
             s_audio_config.codec_i2c_addr,
             s_audio_config.i2s_port_id,
             s_audio_config.mclk_gpio,
             s_audio_config.bclk_gpio,
             s_audio_config.ws_gpio,
             s_audio_config.dout_gpio,
             s_audio_config.pa_enable_gpio);
    return ESP_OK;
}

const bsp_audio_codec_config_t *bsp_audio_get_codec_config(void)
{
    return &s_audio_config;
}

esp_err_t bsp_audio_set_pa_enabled(bool enable)
{
    ESP_RETURN_ON_ERROR(bsp_audio_init(), TAG, "Audio init failed");
    if (s_audio_config.pa_enable_gpio < 0) {
        return ESP_OK;
    }

    return gpio_set_level(s_audio_config.pa_enable_gpio, enable ? 1 : 0);
}
