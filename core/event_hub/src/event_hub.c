#include "event_hub.h"

#include "esp_check.h"
#include "esp_log.h"

ESP_EVENT_DEFINE_BASE(APP_EVENT_BASE);

static const char *TAG = "event_hub";
static bool s_event_loop_ready;

esp_err_t event_hub_init(void)
{
    if (s_event_loop_ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Create default event loop failed");
    s_event_loop_ready = true;
    ESP_LOGI(TAG, "Event hub ready");
    return ESP_OK;
}

esp_err_t event_hub_post(app_event_id_t event_id,
                         const void *event_data,
                         size_t event_data_size,
                         TickType_t ticks_to_wait)
{
    ESP_RETURN_ON_FALSE(s_event_loop_ready, ESP_ERR_INVALID_STATE, TAG, "Event hub not initialized");
    return esp_event_post(APP_EVENT_BASE, event_id, event_data, event_data_size, ticks_to_wait);
}

esp_err_t event_hub_register(app_event_id_t event_id, esp_event_handler_t handler, void *handler_arg)
{
    ESP_RETURN_ON_FALSE(s_event_loop_ready, ESP_ERR_INVALID_STATE, TAG, "Event hub not initialized");
    return esp_event_handler_register(APP_EVENT_BASE, event_id, handler, handler_arg);
}
