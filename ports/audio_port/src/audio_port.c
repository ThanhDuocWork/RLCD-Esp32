#include "audio_port.h"

#include <string.h>

#include "bsp_audio.h"
#include "driver/i2s_std.h"
#include "es8311.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_master_driver.h"

static const char *TAG = "audio_port";

#define AUDIO_PORT_TASK_STACK 4096
#define AUDIO_PORT_TASK_PRIO 4
#define AUDIO_PORT_SAMPLE_RATE 16000
#define AUDIO_PORT_CHUNK_FRAMES 256
#define AUDIO_PORT_AMPLITUDE 6000

typedef struct {
    uint16_t freq_hz;
    uint16_t duration_ms;
} audio_port_note_t;

typedef enum {
    AUDIO_PORT_MODE_NONE = 0,
    AUDIO_PORT_MODE_MELODY,
    AUDIO_PORT_MODE_PCM_STREAM,
} audio_port_mode_t;

static const audio_port_note_t s_test_melody[] = {
    {262, 220}, /* C4 */
    {294, 220}, /* D4 */
    {330, 220}, /* E4 */
    {392, 320}, /* G4 */
    {0,   120}, /* rest */
    {392, 220}, /* G4 */
    {330, 220}, /* E4 */
    {294, 220}, /* D4 */
    {262, 360}, /* C4 */
    {0,   220}, /* rest */
};

static i2s_chan_handle_t s_tx_handle;
static i2c_master_dev_handle_t s_codec_i2c_handle;
static TaskHandle_t s_audio_task;
static bool s_ready;
static bool s_streaming;
static audio_port_mode_t s_mode;
static int16_t s_pcm_chunk[AUDIO_PORT_CHUNK_FRAMES];
static uint32_t s_tone_phase;
static uint32_t s_tone_step;
static size_t s_melody_index;
static uint32_t s_note_samples_left;

static void audio_port_log_probe_result(const char *name, uint16_t address, esp_err_t ret)
{
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "I2C probe ok: %s addr=0x%02x", name, address);
    } else {
        ESP_LOGW(TAG, "I2C probe failed: %s addr=0x%02x (%s)", name, address, esp_err_to_name(ret));
    }
}

static void audio_port_select_note(size_t index)
{
    const audio_port_note_t *note = &s_test_melody[index];

    s_melody_index = index;
    s_tone_phase = 0;
    s_tone_step = (note->freq_hz == 0)
                      ? 0
                      : (uint32_t)(((uint64_t)note->freq_hz << 32) / AUDIO_PORT_SAMPLE_RATE);
    s_note_samples_left = ((uint32_t)note->duration_ms * AUDIO_PORT_SAMPLE_RATE) / 1000U;
    if (s_note_samples_left == 0) {
        s_note_samples_left = 1;
    }
}

static void audio_port_advance_note(void)
{
    size_t next_index = s_melody_index + 1U;

    if (next_index >= (sizeof(s_test_melody) / sizeof(s_test_melody[0]))) {
        next_index = 0;
    }

    audio_port_select_note(next_index);
}

static void audio_port_fill_test_tone(void)
{
    for (uint32_t i = 0; i < AUDIO_PORT_CHUNK_FRAMES; ++i) {
        if (s_note_samples_left == 0) {
            audio_port_advance_note();
        }

        const int16_t sample = (s_tone_step == 0)
                                   ? 0
                                   : ((s_tone_phase < 0x80000000U)
                                          ? AUDIO_PORT_AMPLITUDE
                                          : -AUDIO_PORT_AMPLITUDE);

        s_pcm_chunk[i] = sample;
        s_tone_phase += s_tone_step;
        s_note_samples_left--;
    }
}

static esp_err_t audio_port_start_common(size_t *bytes_loaded)
{
    ESP_RETURN_ON_ERROR(audio_port_init(), TAG, "Audio port init failed");

    if (s_streaming) {
        if (bytes_loaded != NULL) {
            *bytes_loaded = 0;
        }
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(i2s_channel_preload_data(s_tx_handle, s_pcm_chunk, sizeof(s_pcm_chunk), bytes_loaded),
                        TAG,
                        "Preload I2S TX failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx_handle), TAG, "Enable I2S TX failed");
    ESP_RETURN_ON_ERROR(es8311_set_mute(false), TAG, "Unmute ES8311 failed");
    ESP_RETURN_ON_ERROR(bsp_audio_set_pa_enabled(true), TAG, "Enable PA failed");
    s_streaming = true;
    return ESP_OK;
}

static void audio_port_task(void *arg)
{
    (void)arg;

    while (true) {
        if (!s_streaming || s_tx_handle == NULL || s_mode != AUDIO_PORT_MODE_MELODY) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        audio_port_fill_test_tone();

        const uint8_t *write_ptr = (const uint8_t *)s_pcm_chunk;
        size_t remaining = sizeof(s_pcm_chunk);

        while (s_streaming && remaining > 0) {
            size_t bytes_written = 0;
            esp_err_t ret = i2s_channel_write(s_tx_handle,
                                              write_ptr,
                                              remaining,
                                              &bytes_written,
                                              portMAX_DELAY);

            if (bytes_written > 0) {
                write_ptr += bytes_written;
                remaining -= bytes_written;
            }

            if (ret != ESP_OK && bytes_written == 0) {
                ESP_LOGW(TAG, "I2S stalled: %s (remaining=%u)",
                         esp_err_to_name(ret),
                         (unsigned)remaining);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
}

esp_err_t audio_port_init(void)
{
    const bsp_audio_codec_config_t *cfg;

    if (s_ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(bsp_audio_init(), TAG, "BSP audio init failed");
    cfg = bsp_audio_get_codec_config();
    ESP_RETURN_ON_FALSE(cfg != NULL, ESP_ERR_INVALID_STATE, TAG, "No audio config");

    const bus_i2c_master_config_t i2c_cfg = {
        .port_id = cfg->i2c_port_id,
        .sda_gpio = cfg->i2c_sda_gpio,
        .scl_gpio = cfg->i2c_scl_gpio,
        .clock_speed_hz = cfg->i2c_clock_hz,
        .enable_internal_pullup = true,
    };
    const bus_i2c_master_device_config_t codec_i2c_cfg = {
        .device_address = cfg->codec_i2c_addr,
        .scl_speed_hz = cfg->i2c_clock_hz,
    };
    es8311_config_t codec_cfg = {
        .i2c_handle = NULL,
        .sample_rate_hz = cfg->sample_rate_hz,
        .mclk_hz = cfg->sample_rate_hz * 256U,
        .bits_per_sample = 16,
        .default_volume = 0xBF,
        .use_mic = false,
    };

    ESP_RETURN_ON_ERROR(i2c_master_driver_init(&i2c_cfg), TAG, "I2C bus init failed");
    ESP_RETURN_ON_ERROR(bsp_audio_set_pa_enabled(true), TAG, "Enable PA before codec probe failed");
    vTaskDelay(pdMS_TO_TICKS(20));

    audio_port_log_probe_result("SHTC3", 0x70, i2c_master_driver_probe(0x70, 100));
    audio_port_log_probe_result("PCF85063", 0x51, i2c_master_driver_probe(0x51, 100));

    esp_err_t codec_probe_ret = i2c_master_driver_probe(cfg->codec_i2c_addr, 100);
    audio_port_log_probe_result(cfg->codec_name, cfg->codec_i2c_addr, codec_probe_ret);
    ESP_RETURN_ON_ERROR(codec_probe_ret, TAG, "ES8311 probe failed");
    ESP_RETURN_ON_ERROR(i2c_master_driver_add_device(&codec_i2c_cfg, &s_codec_i2c_handle), TAG, "Add ES8311 I2C device failed");

    const i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)cfg->i2s_port_id, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx_handle, NULL), TAG, "Create I2S TX channel failed");

    i2s_std_slot_config_t slot_cfg =
        I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(cfg->sample_rate_hz),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .mclk = cfg->mclk_gpio,
            .bclk = cfg->bclk_gpio,
            .ws = cfg->ws_gpio,
            .dout = cfg->dout_gpio,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx_handle, &std_cfg), TAG, "Init I2S std mode failed");
    codec_cfg.i2c_handle = s_codec_i2c_handle;
    ESP_RETURN_ON_ERROR(es8311_init(&codec_cfg), TAG, "ES8311 init failed");
    audio_port_select_note(0);
    audio_port_fill_test_tone();

    if (s_audio_task == NULL) {
        BaseType_t ok = xTaskCreate(audio_port_task,
                                    "audio_port",
                                    AUDIO_PORT_TASK_STACK,
                                    NULL,
                                    AUDIO_PORT_TASK_PRIO,
                                    &s_audio_task);
        ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "Create audio task failed");
    }

    s_ready = true;
    ESP_LOGI(TAG,
             "Audio port ready: sr=%u fmt=philips mono-left bclk=%d ws=%d dout=%d pa=%d",
             (unsigned)cfg->sample_rate_hz,
             cfg->bclk_gpio,
             cfg->ws_gpio,
             cfg->dout_gpio,
             cfg->pa_enable_gpio);
    return ESP_OK;
}

esp_err_t audio_port_start_test_tone(void)
{
    size_t bytes_loaded = 0;

    audio_port_select_note(0);
    audio_port_fill_test_tone();
    ESP_RETURN_ON_ERROR(audio_port_start_common(&bytes_loaded), TAG, "Start audio path failed");
    s_mode = AUDIO_PORT_MODE_MELODY;
    ESP_LOGI(TAG, "I2S preload loaded=%u bytes", (unsigned)bytes_loaded);
    ESP_LOGI(TAG, "Start audio test melody");
    return ESP_OK;
}

esp_err_t audio_port_start_pcm_stream(void)
{
    size_t bytes_loaded = 0;

    memset(s_pcm_chunk, 0, sizeof(s_pcm_chunk));
    ESP_RETURN_ON_ERROR(audio_port_start_common(&bytes_loaded), TAG, "Start PCM stream failed");
    s_mode = AUDIO_PORT_MODE_PCM_STREAM;
    ESP_LOGI(TAG, "I2S preload loaded=%u bytes", (unsigned)bytes_loaded);
    ESP_LOGI(TAG, "Start PCM stream");
    return ESP_OK;
}

esp_err_t audio_port_write_pcm(const int16_t *samples, size_t sample_count, TickType_t ticks_to_wait)
{
    const uint8_t *write_ptr = (const uint8_t *)samples;
    size_t remaining;
    int retry_count = 0;

    ESP_RETURN_ON_FALSE(samples != NULL && sample_count > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid PCM buffer");
    ESP_RETURN_ON_FALSE(s_streaming && s_mode == AUDIO_PORT_MODE_PCM_STREAM,
                        ESP_ERR_INVALID_STATE,
                        TAG,
                        "PCM stream is not active");

    remaining = sample_count * sizeof(int16_t);

    while (remaining > 0 && s_streaming && s_mode == AUDIO_PORT_MODE_PCM_STREAM) {
        size_t bytes_written = 0;
        esp_err_t ret = i2s_channel_write(s_tx_handle, write_ptr, remaining, &bytes_written, ticks_to_wait);

        if (bytes_written > 0) {
            write_ptr += bytes_written;
            remaining -= bytes_written;
            retry_count = 0;
        }

        if (ret != ESP_OK) {
            if (bytes_written == 0) {
                if (++retry_count >= 8) {
                    ESP_LOGE(TAG, "PCM stream stalled: %s (remaining=%u)",
                             esp_err_to_name(ret),
                             (unsigned)remaining);
                    return ret;
                }
                vTaskDelay(pdMS_TO_TICKS(5));
            } else {
                ESP_LOGW(TAG, "PCM partial write: %s (remaining=%u)",
                         esp_err_to_name(ret),
                         (unsigned)remaining);
            }
        }
    }

    return remaining == 0 ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t audio_port_stop(void)
{
    if (!s_ready) {
        return ESP_OK;
    }

    if (!s_streaming) {
        if (s_codec_i2c_handle != NULL) {
            ESP_RETURN_ON_ERROR(es8311_set_mute(true), TAG, "Mute ES8311 failed");
        }
        return bsp_audio_set_pa_enabled(false);
    }

    s_streaming = false;
    s_mode = AUDIO_PORT_MODE_NONE;
    if (s_tx_handle != NULL) {
        ESP_RETURN_ON_ERROR(i2s_channel_disable(s_tx_handle), TAG, "Disable I2S TX failed");
    }
    ESP_RETURN_ON_ERROR(es8311_set_mute(true), TAG, "Mute ES8311 failed");
    ESP_RETURN_ON_ERROR(bsp_audio_set_pa_enabled(false), TAG, "Disable PA failed");
    ESP_LOGI(TAG, "Stop audio output");
    return ESP_OK;
}

bool audio_port_is_streaming(void)
{
    return s_streaming;
}
