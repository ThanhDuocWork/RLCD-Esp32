#include "audio_service.h"

#include <dirent.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_port.h"
#include "event_hub.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "storage_service.h"

static const char *TAG = "audio_service";

#define AUDIO_SERVICE_PLAYBACK_TASK_STACK 4096
#define AUDIO_SERVICE_PLAYBACK_TASK_PRIO 4
#define AUDIO_SERVICE_WAV_DIR_PATH "/sdcard/audio"
#define AUDIO_SERVICE_WAV_IO_BUFFER_BYTES 512
#define AUDIO_SERVICE_OUTPUT_SAMPLE_RATE 16000

typedef struct {
    uint16_t audio_format;
    uint16_t channel_count;
    uint32_t sample_rate_hz;
    uint16_t bits_per_sample;
    uint32_t data_size;
} audio_service_wav_info_t;

static audio_service_state_t s_state = AUDIO_SERVICE_STATE_IDLE;
static esp_err_t s_last_error = ESP_OK;
static TaskHandle_t s_playback_task;
static volatile bool s_stop_requested;
static char s_playback_path[160];

static esp_err_t audio_service_publish_state(audio_service_state_t state, esp_err_t last_error)
{
    const audio_service_event_t event = {
        .state = state,
        .last_error = last_error,
    };

    s_state = state;
    s_last_error = last_error;
    return event_hub_post(APP_EVENT_AUDIO_STATE_CHANGED, &event, sizeof(event), 0);
}

static uint16_t audio_service_read_le16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t audio_service_read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static int16_t audio_service_get_input_mono_sample(const int16_t *src, uint16_t channel_count, size_t frame_index)
{
    if (channel_count == 1) {
        return src[frame_index];
    }

    return src[frame_index * 2];
}

static bool audio_service_has_wav_extension(const char *name)
{
    size_t len;

    if (name == NULL) {
        return false;
    }

    len = strlen(name);
    if (len < 4) {
        return false;
    }

    return tolower((unsigned char)name[len - 4]) == '.' &&
           tolower((unsigned char)name[len - 3]) == 'w' &&
           tolower((unsigned char)name[len - 2]) == 'a' &&
           tolower((unsigned char)name[len - 1]) == 'v';
}

static esp_err_t audio_service_find_first_wav(char *out_path, size_t out_path_size)
{
    DIR *dir;
    struct dirent *entry;

    ESP_RETURN_ON_FALSE(out_path != NULL && out_path_size > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid output path buffer");

    dir = opendir(AUDIO_SERVICE_WAV_DIR_PATH);
    if (dir == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    while ((entry = readdir(dir)) != NULL) {
        int written;

        if (entry->d_name[0] == '.') {
            continue;
        }

        if (!audio_service_has_wav_extension(entry->d_name)) {
            continue;
        }

        written = snprintf(out_path, out_path_size, "%s/%s", AUDIO_SERVICE_WAV_DIR_PATH, entry->d_name);
        if (written <= 0 || (size_t)written >= out_path_size) {
            closedir(dir);
            return ESP_ERR_INVALID_SIZE;
        }

        closedir(dir);
        return ESP_OK;
    }

    closedir(dir);
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t audio_service_parse_wav(FILE *file, audio_service_wav_info_t *info)
{
    uint8_t riff_header[12];
    bool fmt_found = false;
    bool data_found = false;

    ESP_RETURN_ON_FALSE(file != NULL && info != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid WAV parser args");

    if (fread(riff_header, 1, sizeof(riff_header), file) != sizeof(riff_header)) {
        return ESP_FAIL;
    }

    if (memcmp(riff_header, "RIFF", 4) != 0 || memcmp(&riff_header[8], "WAVE", 4) != 0) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    memset(info, 0, sizeof(*info));

    while (!data_found) {
        uint8_t chunk_header[8];
        uint32_t chunk_size;
        long next_chunk_pos;

        if (fread(chunk_header, 1, sizeof(chunk_header), file) != sizeof(chunk_header)) {
            return ESP_FAIL;
        }

        chunk_size = audio_service_read_le32(&chunk_header[4]);
        next_chunk_pos = ftell(file) + (long)chunk_size + (chunk_size & 1U);

        if (memcmp(chunk_header, "fmt ", 4) == 0) {
            uint8_t fmt_data[16];

            if (chunk_size < sizeof(fmt_data) || fread(fmt_data, 1, sizeof(fmt_data), file) != sizeof(fmt_data)) {
                return ESP_FAIL;
            }

            info->audio_format = audio_service_read_le16(&fmt_data[0]);
            info->channel_count = audio_service_read_le16(&fmt_data[2]);
            info->sample_rate_hz = audio_service_read_le32(&fmt_data[4]);
            info->bits_per_sample = audio_service_read_le16(&fmt_data[14]);
            fmt_found = true;
        } else if (memcmp(chunk_header, "data", 4) == 0) {
            info->data_size = chunk_size;
            data_found = true;
            break;
        }

        if (fseek(file, next_chunk_pos, SEEK_SET) != 0) {
            return ESP_FAIL;
        }
    }

    ESP_RETURN_ON_FALSE(fmt_found && data_found, ESP_ERR_INVALID_RESPONSE, TAG, "Incomplete WAV file");
    ESP_RETURN_ON_FALSE(info->audio_format == 1, ESP_ERR_NOT_SUPPORTED, TAG, "Only PCM WAV is supported");
    ESP_RETURN_ON_FALSE(info->bits_per_sample == 16, ESP_ERR_NOT_SUPPORTED, TAG, "Only 16-bit WAV is supported");
    ESP_RETURN_ON_FALSE(info->channel_count == 1 || info->channel_count == 2,
                        ESP_ERR_NOT_SUPPORTED,
                        TAG,
                        "Only mono/stereo WAV is supported");
    ESP_RETURN_ON_FALSE(info->sample_rate_hz >= AUDIO_SERVICE_OUTPUT_SAMPLE_RATE,
                        ESP_ERR_NOT_SUPPORTED,
                        TAG,
                        "Only WAV sample rates >= 16kHz are supported");
    return ESP_OK;
}

static esp_err_t audio_service_play_wav_file(const char *path)
{
    FILE *file = NULL;
    audio_service_wav_info_t info;
    uint8_t io_buffer[AUDIO_SERVICE_WAV_IO_BUFFER_BYTES];
    int16_t output_samples[AUDIO_SERVICE_WAV_IO_BUFFER_BYTES / 2];
    uint32_t resample_pos_q16 = 0;
    uint32_t resample_step_q16;
    esp_err_t ret;

    file = fopen(path, "rb");
    ESP_RETURN_ON_FALSE(file != NULL, ESP_ERR_NOT_FOUND, TAG, "Open WAV failed: %s", path);

    ret = audio_service_parse_wav(file, &info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG,
                 "Unsupported WAV %s: fmt=%u ch=%u rate=%u bits=%u",
                 path,
                 info.audio_format,
                 info.channel_count,
                 (unsigned)info.sample_rate_hz,
                 info.bits_per_sample);
        fclose(file);
        return ret;
    }

    ESP_LOGI(TAG,
             "Play WAV %s: ch=%u rate=%u bits=%u data=%u -> out=%u mono",
             path,
             info.channel_count,
             (unsigned)info.sample_rate_hz,
             info.bits_per_sample,
             (unsigned)info.data_size,
             AUDIO_SERVICE_OUTPUT_SAMPLE_RATE);

    ret = audio_port_start_pcm_stream();
    if (ret != ESP_OK) {
        fclose(file);
        return ret;
    }

    resample_step_q16 = (uint32_t)(((uint64_t)info.sample_rate_hz << 16) / AUDIO_SERVICE_OUTPUT_SAMPLE_RATE);

    while (!s_stop_requested) {
        size_t bytes_read = fread(io_buffer, 1, sizeof(io_buffer), file);
        size_t frame_bytes = info.channel_count * sizeof(int16_t);
        if (bytes_read == 0) {
            break;
        }

        bytes_read -= (bytes_read % frame_bytes);
        if (bytes_read == 0) {
            continue;
        }

        {
            const int16_t *src = (const int16_t *)io_buffer;
            size_t input_frames = bytes_read / frame_bytes;
            size_t output_count = 0;

            while ((resample_pos_q16 >> 16) < input_frames && output_count < (sizeof(output_samples) / sizeof(output_samples[0]))) {
                size_t src_frame = resample_pos_q16 >> 16;
                output_samples[output_count++] = audio_service_get_input_mono_sample(src, info.channel_count, src_frame);
                resample_pos_q16 += resample_step_q16;
            }

            if (output_count > 0) {
                ret = audio_port_write_pcm(output_samples, output_count, portMAX_DELAY);
            } else {
                ret = ESP_OK;
            }

            if ((resample_pos_q16 >> 16) >= input_frames) {
                resample_pos_q16 -= ((uint32_t)input_frames << 16);
            } else {
                resample_pos_q16 = 0;
            }
        }

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "PCM write failed: %s", esp_err_to_name(ret));
            fclose(file);
            audio_port_stop();
            return ret;
        }
    }

    fclose(file);
    return audio_port_stop();
}

static void audio_service_playback_task(void *arg)
{
    const char *path = (const char *)arg;
    esp_err_t ret = audio_service_play_wav_file(path);

    if (ret != ESP_OK && !s_stop_requested) {
        ESP_LOGE(TAG, "WAV playback failed: %s", esp_err_to_name(ret));
        audio_service_publish_state(AUDIO_SERVICE_STATE_ERROR, ret);
    } else {
        audio_service_publish_state(AUDIO_SERVICE_STATE_IDLE, ESP_OK);
    }

    s_stop_requested = false;
    s_playback_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t audio_service_init(void)
{
    ESP_LOGI(TAG, "Initialize audio service");
    ESP_RETURN_ON_ERROR(audio_port_init(), TAG, "Audio port init failed");
    ESP_RETURN_ON_ERROR(audio_service_publish_state(AUDIO_SERVICE_STATE_IDLE, ESP_OK),
                        TAG,
                        "Publish initial audio state failed");
    return ESP_OK;
}

esp_err_t audio_service_start_test_tone(void)
{
    esp_err_t ret = audio_port_start_test_tone();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Start test tone failed: %s", esp_err_to_name(ret));
        audio_service_publish_state(AUDIO_SERVICE_STATE_ERROR, ret);
        return ret;
    }

    ESP_LOGI(TAG, "Audio test tone started");
    ESP_RETURN_ON_ERROR(audio_service_publish_state(AUDIO_SERVICE_STATE_PLAYING, ESP_OK),
                        TAG,
                        "Publish playing state failed");
    return ESP_OK;
}

esp_err_t audio_service_start_wav(const char *path)
{
    BaseType_t task_ok;

    ESP_RETURN_ON_FALSE(path != NULL && path[0] != '\0', ESP_ERR_INVALID_ARG, TAG, "Invalid WAV path");
    ESP_RETURN_ON_FALSE(!audio_service_is_playing(), ESP_ERR_INVALID_STATE, TAG, "Audio is already playing");

    strncpy(s_playback_path, path, sizeof(s_playback_path) - 1U);
    s_playback_path[sizeof(s_playback_path) - 1U] = '\0';
    s_stop_requested = false;

    task_ok = xTaskCreate(audio_service_playback_task,
                          "audio_playback",
                          AUDIO_SERVICE_PLAYBACK_TASK_STACK,
                          s_playback_path,
                          AUDIO_SERVICE_PLAYBACK_TASK_PRIO,
                          &s_playback_task);
    ESP_RETURN_ON_FALSE(task_ok == pdPASS, ESP_ERR_NO_MEM, TAG, "Create playback task failed");

    ESP_LOGI(TAG, "Start WAV playback: %s", s_playback_path);
    ESP_RETURN_ON_ERROR(audio_service_publish_state(AUDIO_SERVICE_STATE_PLAYING, ESP_OK),
                        TAG,
                        "Publish playing state failed");
    return ESP_OK;
}

esp_err_t audio_service_start_default_playback(void)
{
    esp_err_t ret;

    if (storage_service_is_ready()) {
        ret = audio_service_find_first_wav(s_playback_path, sizeof(s_playback_path));
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Detected WAV file: %s", s_playback_path);
            return audio_service_start_wav(s_playback_path);
        }

        ESP_LOGW(TAG, "No WAV file found in %s, fallback to melody", AUDIO_SERVICE_WAV_DIR_PATH);
    } else {
        ESP_LOGW(TAG, "Storage not ready, fallback to melody");
    }

    return audio_service_start_test_tone();
}

esp_err_t audio_service_stop(void)
{
    esp_err_t ret;

    s_stop_requested = true;
    ret = audio_port_stop();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Stop audio failed: %s", esp_err_to_name(ret));
        audio_service_publish_state(AUDIO_SERVICE_STATE_ERROR, ret);
        return ret;
    }

    ESP_LOGI(TAG, "Audio stopped");
    ESP_RETURN_ON_ERROR(audio_service_publish_state(AUDIO_SERVICE_STATE_IDLE, ESP_OK),
                        TAG,
                        "Publish idle state failed");
    return ESP_OK;
}

esp_err_t audio_service_toggle_test_tone(void)
{
    return audio_service_is_playing() ? audio_service_stop() : audio_service_start_test_tone();
}

esp_err_t audio_service_toggle_default_playback(void)
{
    return audio_service_is_playing() ? audio_service_stop() : audio_service_start_default_playback();
}

audio_service_state_t audio_service_get_state(void)
{
    return s_state;
}

bool audio_service_is_playing(void)
{
    return s_state == AUDIO_SERVICE_STATE_PLAYING;
}
