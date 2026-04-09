#pragma once

#include <stddef.h>

#include "esp_err.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"

ESP_EVENT_DECLARE_BASE(APP_EVENT_BASE);

typedef enum {
    APP_EVENT_DISPLAY_READY = 1,
    APP_EVENT_DISPLAY_BOOT_SCREEN,
    APP_EVENT_INPUT_KEY,
    APP_EVENT_SENSOR_UPDATED,
    APP_EVENT_AUDIO_STATE_CHANGED,
} app_event_id_t;

esp_err_t event_hub_init(void);
esp_err_t event_hub_post(app_event_id_t event_id, const void *event_data, size_t event_data_size, TickType_t ticks_to_wait);
esp_err_t event_hub_register(app_event_id_t event_id, esp_event_handler_t handler, void *handler_arg);
