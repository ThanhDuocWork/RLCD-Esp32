#include "input_service.h"

#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "input_port.h"

static const char *TAG = "input_service";

#define INPUT_SERVICE_POLL_MS 20U
#define INPUT_SERVICE_DEBOUNCE_TICKS 2U
#define INPUT_SERVICE_LONG_PRESS_MS 900U
#define INPUT_SERVICE_TASK_STACK 4096U

static TaskHandle_t s_input_task;
static uint32_t s_key_click_count;

static void input_service_post_key(input_service_key_action_t action, uint32_t press_ms)
{
    const input_service_key_event_t event = {
        .button = INPUT_SERVICE_BUTTON_KEY,
        .action = action,
        .click_count = ++s_key_click_count,
        .press_ms = press_ms,
    };

    ESP_LOGI(TAG,
             "KEY %s %lu (%lu ms)",
             action == INPUT_SERVICE_KEY_ACTION_LONG_PRESS ? "long" : "short",
             (unsigned long)event.click_count,
             (unsigned long)event.press_ms);
    if (event_hub_post(APP_EVENT_INPUT_KEY, &event, sizeof(event), 0) != ESP_OK) {
        ESP_LOGW(TAG, "Post KEY event failed");
    }
}

static void input_service_task(void *arg)
{
    bool last_pressed = false;
    bool stable_pressed = false;
    uint8_t debounce_ticks = 0;
    TickType_t press_start_tick = 0;

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

        if (debounce_ticks >= INPUT_SERVICE_DEBOUNCE_TICKS && pressed != stable_pressed) {
            stable_pressed = pressed;
            if (stable_pressed) {
                press_start_tick = xTaskGetTickCount();
            } else {
                const TickType_t elapsed_ticks = xTaskGetTickCount() - press_start_tick;
                const uint32_t press_ms = elapsed_ticks * portTICK_PERIOD_MS;
                const input_service_key_action_t action = (press_ms >= INPUT_SERVICE_LONG_PRESS_MS)
                                                              ? INPUT_SERVICE_KEY_ACTION_LONG_PRESS
                                                              : INPUT_SERVICE_KEY_ACTION_SHORT_PRESS;

                input_service_post_key(action, press_ms);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(INPUT_SERVICE_POLL_MS));
    }
}

esp_err_t input_service_init(void)
{
    ESP_LOGI(TAG, "Initialize input service");
    ESP_RETURN_ON_ERROR(input_port_init(), TAG, "Input port init failed");

    if (s_input_task == NULL) {
        BaseType_t ok = xTaskCreate(input_service_task,
                                    "input_service",
                                    INPUT_SERVICE_TASK_STACK,
                                    NULL,
                                    tskIDLE_PRIORITY + 2,
                                    &s_input_task);
        ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "Create input task failed");
    }

    return ESP_OK;
}
