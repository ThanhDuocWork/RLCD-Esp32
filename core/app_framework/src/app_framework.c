#include "app_framework.h"

#include "esp_log.h"

static const char *TAG = "app_framework";

esp_err_t app_framework_init(void)
{
    ESP_LOGI(TAG, "Initialize app framework");
    return ESP_OK;
}
