#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t audio_port_init(void);
esp_err_t audio_port_start_test_tone(void);
esp_err_t audio_port_start_pcm_stream(void);
esp_err_t audio_port_write_pcm(const int16_t *samples, size_t sample_count, TickType_t ticks_to_wait);
esp_err_t audio_port_stop(void);
bool audio_port_is_streaming(void);
