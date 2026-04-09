#include "system_manager.h"

#include "app_framework.h"
#include "board.h"
#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"

static const char *TAG = "system_manager";

esp_err_t system_manager_init(void)
{
    ESP_LOGI(TAG, "Initialize system manager");
    ESP_RETURN_ON_ERROR(board_init(), TAG, "Board init failed");
    ESP_RETURN_ON_ERROR(event_hub_init(), TAG, "Event hub init failed");
    ESP_RETURN_ON_ERROR(app_framework_init(), TAG, "App framework init failed");
    return ESP_OK;
}
