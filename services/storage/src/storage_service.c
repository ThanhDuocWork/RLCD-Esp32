#include "storage_service.h"

#include "esp_log.h"
#include "storage_port.h"

static const char *TAG = "storage_service";
static bool s_storage_ready;

esp_err_t storage_service_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initialize storage service");
    ret = storage_port_mount();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "TF card is not ready yet: %s", esp_err_to_name(ret));
        s_storage_ready = false;
        return ESP_OK;
    }

    s_storage_ready = true;
    return ESP_OK;
}

bool storage_service_is_ready(void)
{
    return s_storage_ready;
}

const char *storage_service_get_mount_point(void)
{
    return storage_port_get_mount_point();
}
