#include "input_port.h"

#include "bsp_input.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "input_port";
static const bsp_input_button_config_t *s_key_config;
static bool s_ready;

esp_err_t input_port_init(void)
{
    s_key_config = bsp_input_get_key_button_config();
    ESP_RETURN_ON_FALSE(s_key_config != NULL, ESP_ERR_INVALID_STATE, TAG, "No KEY config");
    ESP_RETURN_ON_FALSE(s_key_config->gpio >= 0, ESP_ERR_INVALID_STATE, TAG, "KEY GPIO is invalid");

    const gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << s_key_config->gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = s_key_config->enable_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&io_config), TAG, "Configure KEY GPIO failed");
    s_ready = true;
    ESP_LOGI(TAG, "%s ready: gpio=%d active_low=%d", s_key_config->name, s_key_config->gpio, s_key_config->active_low);
    return ESP_OK;
}

bool input_port_is_key_pressed(void)
{
    int level;

    if (!s_ready || s_key_config == NULL) {
        return false;
    }

    level = gpio_get_level(s_key_config->gpio);
    return s_key_config->active_low ? (level == 0) : (level != 0);
}
