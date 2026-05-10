#include "product_speaker.h"

#include "audio_service.h"
#include "display_service.h"
#include "esp_check.h"
#include "esp_log.h"
#include "home_app.h"
#include "input_service.h"
#include "sensor_service.h"
#include "storage_service.h"
#include "system_manager.h"

static const char *TAG = "product_speaker";

void product_speaker_start(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Boot speaker product");

    ESP_ERROR_CHECK(system_manager_init());
    ESP_ERROR_CHECK(display_service_init());
    ret = audio_service_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Audio service init failed, continue without audio: %s", esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(input_service_init());
    ESP_ERROR_CHECK(sensor_service_init());
    ESP_ERROR_CHECK(storage_service_init());
    ESP_ERROR_CHECK(home_app_start());

    ESP_LOGI(TAG, "Speaker product ready");
}
