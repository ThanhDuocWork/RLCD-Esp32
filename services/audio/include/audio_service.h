#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    AUDIO_SERVICE_STATE_IDLE = 0,
    AUDIO_SERVICE_STATE_PLAYING,
    AUDIO_SERVICE_STATE_ERROR,
} audio_service_state_t;

typedef struct {
    audio_service_state_t state;
    esp_err_t last_error;
} audio_service_event_t;

esp_err_t audio_service_init(void);
esp_err_t audio_service_start_test_tone(void);
esp_err_t audio_service_start_wav(const char *path);
esp_err_t audio_service_start_default_playback(void);
esp_err_t audio_service_stop(void);
esp_err_t audio_service_toggle_test_tone(void);
esp_err_t audio_service_toggle_default_playback(void);
audio_service_state_t audio_service_get_state(void);
bool audio_service_is_playing(void);
