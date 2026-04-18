#include "input_service.h"

#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_port.h"

static const char *TAG = "input_service";

static TaskHandle_t s_input_task;
static uint32_t s_key_click_count;

static void input_service_task(void *arg)
{
    bool last_pressed = false;
    bool stable_pressed = false;
    uint8_t debounce_ticks = 0;

    (void)arg;

    while (true) {
        const bool pressed = input_port_is_key_pressed();

        if (pressed == last_pressed) {
            if (debounce_ticks < 3) {
                debounce_ticks++;
            }
        } else {
            debounce_ticks = 0;
            last_pressed = pressed;
        }

        if (debounce_ticks >= 2 && pressed != stable_pressed) {
            stable_pressed = pressed;
            if (stable_pressed) {
                const input_service_key_event_t event = {
                    .button = INPUT_SERVICE_BUTTON_KEY,
                    .click_count = ++s_key_click_count,
                };

                ESP_LOGI(TAG, "KEY click %lu", (unsigned long)event.click_count);
                if (event_hub_post(APP_EVENT_INPUT_KEY, &event, sizeof(event), 0) != ESP_OK) {
                    ESP_LOGW(TAG, "Post KEY event failed");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t input_service_init(void)
{
    ESP_LOGI(TAG, "Initialize input service");
    ESP_RETURN_ON_ERROR(input_port_init(), TAG, "Input port init failed");

    if (s_input_task == NULL) {
        BaseType_t ok = xTaskCreate(input_service_task,
                                    "input_service",
                                    2048,
                                    NULL,
                                    tskIDLE_PRIORITY + 2,
                                    &s_input_task);
        ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "Create input task failed");
    }

    return ESP_OK;
}
