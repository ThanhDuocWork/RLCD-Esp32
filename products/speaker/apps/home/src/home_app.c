#include "home_app.h"

#include "display_service.h"
#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"

static const char *TAG = "home_app";

esp_err_t home_app_start(void)
{
    ESP_LOGI(TAG, "Start home app");
    ESP_RETURN_ON_ERROR(display_service_show_boot(), TAG, "Boot screen failed");
    ESP_RETURN_ON_ERROR(event_hub_post(APP_EVENT_DISPLAY_BOOT_SCREEN, NULL, 0, 0),
                        TAG,
                        "Post boot event failed");
    return ESP_OK;
}
