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
    ESP_LOGI(TAG, "Boot speaker product");

    ESP_ERROR_CHECK(system_manager_init());
    ESP_ERROR_CHECK(display_service_init());
    ESP_ERROR_CHECK(audio_service_init());
    ESP_ERROR_CHECK(input_service_init());
    ESP_ERROR_CHECK(sensor_service_init());
    ESP_ERROR_CHECK(storage_service_init());
    ESP_ERROR_CHECK(home_app_start());

    ESP_LOGI(TAG, "Speaker product ready");
}
