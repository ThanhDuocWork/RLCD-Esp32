#include "display_service.h"

#include <stdbool.h>

#include "display_port.h"
#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display_service";
static bool s_display_ready;
static TaskHandle_t s_display_test_task;

static void display_service_test_task(void *arg)
{
    bool white = false;
    (void)arg;

    while (true) {
        const uint16_t color = white ? 0x0FFF : 0x0000;
        ESP_LOGI(TAG, "Refresh solid %s", white ? "white" : "black");
        if (display_port_fill_screen(color) != ESP_OK) {
            ESP_LOGE(TAG, "Display fill failed during refresh loop");
        }
        white = !white;
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

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
    ESP_LOGI(TAG, "Start display refresh loop");
    ESP_RETURN_ON_ERROR(display_port_set_power(true), TAG, "Display power failed");

    if (s_display_test_task == NULL) {
        BaseType_t ok = xTaskCreate(display_service_test_task,
                                    "display_test",
                                    3072,
                                    NULL,
                                    tskIDLE_PRIORITY + 1,
                                    &s_display_test_task);
        ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "Create display test task failed");
    }

    return ESP_OK;
}

bool display_service_is_ready(void)
{
    return s_display_ready;
}
