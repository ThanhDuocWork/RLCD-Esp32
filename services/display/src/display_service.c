#include "display_service.h"

#include "display_port.h"
#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"

static const char *TAG = "display_service";
static bool s_display_ready;

esp_err_t display_service_init(void)
{
    ESP_LOGI(TAG, "Initialize display service");
    ESP_RETURN_ON_ERROR(display_port_init(), TAG, "Display port init failed");
    ESP_RETURN_ON_ERROR(event_hub_post(APP_EVENT_DISPLAY_READY, NULL, 0, 0), TAG, "Post display ready failed");
    s_display_ready = true;
    return ESP_OK;
}

esp_err_t display_service_show_boot(void)
{
    ESP_RETURN_ON_FALSE(s_display_ready, ESP_ERR_INVALID_STATE, TAG, "Display is not initialized");
    ESP_LOGI(TAG, "Show boot screen test pattern");
    ESP_RETURN_ON_ERROR(display_port_set_power(true), TAG, "Display power failed");
    return display_port_draw_test_pattern();
}

bool display_service_is_ready(void)
{
    return s_display_ready;
}
