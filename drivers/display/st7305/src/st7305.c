#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7305.h"
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "st7305_protocol.h"

static const char *TAG = "st7305";

typedef struct {
    st7305_config_t config;
    bool ready;
} st7305_runtime_t;

static st7305_runtime_t s_runtime;

static const uint8_t s_nvm_load_ctrl[] = { 0x17, 0x02 };
static const uint8_t s_booster_enable[] = { 0x01 };
static const uint8_t s_gate_voltage_setting[] = { 0x11, 0x04 };
static const uint8_t s_vshp_setting[] = { 0x41, 0x41, 0x41, 0x41 };
static const uint8_t s_vshn_setting[] = { 0x19, 0x19, 0x19, 0x19 };
static const uint8_t s_vslp_setting[] = { 0x41, 0x41, 0x41, 0x41 };
static const uint8_t s_vsln_setting[] = { 0x19, 0x19, 0x19, 0x19 };
static const uint8_t s_vgh_vgl_ctrl[] = { 0xA6, 0xE9 };
static const uint8_t s_display_mode_ctrl[] = { 0x05 };
static const uint8_t s_source_gate_scan_ctrl[] = { 0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45 };
static const uint8_t s_gate_eq_ctrl[] = { 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45 };
static const uint8_t s_vcom_ctrl[] = { 0x32, 0x03, 0x1F };
static const uint8_t s_lv_detect_ctrl[] = { 0x13 };
static const uint8_t s_osc_ctrl[] = { 0x64 };
static const uint8_t s_vcom_output_enable[] = { 0x00 };
static const uint8_t s_madctl_default[] = { 0x48 };
static const uint8_t s_interface_pixel_format[] = { 0x11 };
static const uint8_t s_cabc_ctrl[] = { 0x20 };
static const uint8_t s_frame_rate_ctrl[] = { 0x29 };
static const uint8_t s_te_on[] = { 0x00 };
static const uint8_t s_oscillator_ctrl[] = { 0xFF };

#define ST7305_TX_CHUNK_BYTES 384U

static esp_err_t st7305_tx_cmd(void *user_ctx, uint8_t cmd)
{
    st7305_runtime_t *runtime = (st7305_runtime_t *)user_ctx;
    spi_transaction_t trans = { 0 };

    gpio_set_level(runtime->config.dc_gpio, 0);
    trans.length = 8;
    trans.tx_buffer = &cmd;
    return spi_device_polling_transmit(runtime->config.spi_dev, &trans);
}

static esp_err_t st7305_tx_data(void *user_ctx, const void *data, size_t len)
{
    st7305_runtime_t *runtime = (st7305_runtime_t *)user_ctx;
    spi_transaction_t trans = { 0 };

    if ((data == NULL) || (len == 0)) {
        return ESP_OK;
    }

    gpio_set_level(runtime->config.dc_gpio, 1);
    trans.length = len * 8;
    trans.tx_buffer = data;
    return spi_device_polling_transmit(runtime->config.spi_dev, &trans);
}

static void st7305_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static esp_err_t st7305_tx_data_chunked(const uint8_t *data, size_t len)
{
    while (len > 0) {
        const size_t burst = (len > ST7305_TX_CHUNK_BYTES) ? ST7305_TX_CHUNK_BYTES : len;
        ESP_RETURN_ON_ERROR(st7305_protocol_tx_data(data, burst), TAG, "chunked tx failed");
        data += burst;
        len -= burst;
    }

    return ESP_OK;
}

static esp_err_t st7305_require_ready(void)
{
    return s_runtime.ready ? ESP_OK : ESP_ERR_INVALID_STATE;
}

static size_t st7305_calc_buffer_size(const st7305_config_t *config)
{
    return ((size_t)config->width * (size_t)config->height) / 8U;
}

static esp_err_t st7305_push_buffer(const uint8_t *buffer, size_t len)
{
    uint8_t column_window[2] = { s_runtime.config.column_start, s_runtime.config.column_end };
    uint8_t page_window[2] = { s_runtime.config.page_start, s_runtime.config.page_end };

    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");
    ESP_RETURN_ON_FALSE(len == st7305_calc_buffer_size(&s_runtime.config), ESP_ERR_INVALID_ARG, TAG, "buffer size mismatch");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_CASET), TAG, "CASET failed");
    ESP_RETURN_ON_ERROR(st7305_tx_data_chunked(column_window, sizeof(column_window)), TAG, "CASET data failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_RASET), TAG, "RASET failed");
    ESP_RETURN_ON_ERROR(st7305_tx_data_chunked(page_window, sizeof(page_window)), TAG, "RASET data failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_RAMWR), TAG, "RAMWR failed");
    return st7305_tx_data_chunked(buffer, len);
}

static void st7305_set_pixel_portrait(uint8_t *buffer, int width, int x, int y, bool on)
{
    const int bytes_per_row_group = width / 4;
    const int byte_y = y >> 1;
    const int byte_x = x >> 2;
    const int index = byte_y * bytes_per_row_group + byte_x;
    const uint8_t bit = (uint8_t)(7 - (((x & 0x03) << 1) | (y & 0x01)));
    const uint8_t mask = (uint8_t)(1U << bit);

    if (on) {
        buffer[index] |= mask;
    } else {
        buffer[index] &= (uint8_t)~mask;
    }
}

static esp_err_t st7305_fill_solid_portrait(bool on)
{
    uint8_t *buffer;
    const int width = s_runtime.config.width;
    const int height = s_runtime.config.height;
    const size_t buf_size = st7305_calc_buffer_size(&s_runtime.config);

    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");

    buffer = (uint8_t *)calloc(1, buf_size);
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG, "no mem for solid buffer");

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            st7305_set_pixel_portrait(buffer, width, x, y, on);
        }
    }

    ESP_RETURN_ON_ERROR(st7305_push_buffer(buffer, buf_size), TAG, "push solid buffer failed");
    free(buffer);
    return ESP_OK;
}

static esp_err_t st7305_run_panel_init(void)
{
    const st7305_init_cmd_t init_table[] = {
        { 0xD6, s_nvm_load_ctrl, sizeof(s_nvm_load_ctrl), 0 },
        { 0xD1, s_booster_enable, sizeof(s_booster_enable), 0 },
        { 0xC0, s_gate_voltage_setting, sizeof(s_gate_voltage_setting), 0 },
        { 0xC1, s_vshp_setting, sizeof(s_vshp_setting), 0 },
        { 0xC2, s_vshn_setting, sizeof(s_vshn_setting), 0 },
        { 0xC4, s_vslp_setting, sizeof(s_vslp_setting), 0 },
        { 0xC5, s_vsln_setting, sizeof(s_vsln_setting), 0 },
        { 0xD8, s_vgh_vgl_ctrl, sizeof(s_vgh_vgl_ctrl), 0 },
        { 0xB2, s_display_mode_ctrl, sizeof(s_display_mode_ctrl), 0 },
        { 0xB3, s_source_gate_scan_ctrl, sizeof(s_source_gate_scan_ctrl), 0 },
        { 0xB4, s_gate_eq_ctrl, sizeof(s_gate_eq_ctrl), 0 },
        { 0x62, s_vcom_ctrl, sizeof(s_vcom_ctrl), 0 },
        { 0xB7, s_lv_detect_ctrl, sizeof(s_lv_detect_ctrl), 0 },
        { 0xB0, s_osc_ctrl, sizeof(s_osc_ctrl), 0 },
        { 0x11, NULL, 0, 200 },
        { 0xC9, s_vcom_output_enable, sizeof(s_vcom_output_enable), 0 },
        { 0x36, s_madctl_default, sizeof(s_madctl_default), 0 },
        { 0x3A, s_interface_pixel_format, sizeof(s_interface_pixel_format), 0 },
        { 0xB9, s_cabc_ctrl, sizeof(s_cabc_ctrl), 0 },
        { 0xB8, s_frame_rate_ctrl, sizeof(s_frame_rate_ctrl), 0 },
        { 0x20, NULL, 0, 0 },
        { 0x2A, (const uint8_t[]){ 0x12, 0x2A }, 2, 0 },
        { 0x2B, (const uint8_t[]){ 0x00, 0xC7 }, 2, 0 },
        { 0x35, s_te_on, sizeof(s_te_on), 0 },
        { 0xD0, s_oscillator_ctrl, sizeof(s_oscillator_ctrl), 0 },
        { 0x38, NULL, 0, 0 },
        { 0x29, NULL, 0, 0 },
    };

    return st7305_protocol_run_init_table(init_table, sizeof(init_table) / sizeof(init_table[0]));
}

esp_err_t st7305_init(const st7305_config_t *config)
{
    const st7305_protocol_io_t proto_io = {
        .tx_cmd = st7305_tx_cmd,
        .tx_data = st7305_tx_data,
        .set_window = NULL,
        .delay_ms = st7305_delay_ms,
        .user_ctx = &s_runtime,
    };

    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is null");
    ESP_RETURN_ON_FALSE(config->spi_dev != NULL, ESP_ERR_INVALID_ARG, TAG, "spi device is null");
    ESP_RETURN_ON_FALSE(config->dc_gpio >= 0, ESP_ERR_INVALID_ARG, TAG, "dc gpio is not configured");

    s_runtime.config = *config;

    ESP_RETURN_ON_ERROR(st7305_protocol_init(&proto_io), TAG, "Protocol init failed");
    ESP_RETURN_ON_ERROR(st7305_run_panel_init(), TAG, "Panel init sequence failed");

    s_runtime.ready = true;

    ESP_LOGI(TAG,
             "ST7305 initialized: dc=%d busy=%d size=%dx%d col=%u..%u page=%u..%u",
             config->dc_gpio,
             config->busy_gpio,
             config->width,
             config->height,
             config->column_start,
             config->column_end,
             config->page_start,
             config->page_end);

    return ESP_OK;
}

esp_err_t st7305_fill_raw_pattern(uint8_t pattern_byte)
{
    uint8_t chunk[192];
    size_t total_bytes;

    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");

    if (pattern_byte == 0x00) {
        return st7305_fill_solid_portrait(false);
    }
    if (pattern_byte == 0xFF) {
        return st7305_fill_solid_portrait(true);
    }

    memset(chunk, pattern_byte, sizeof(chunk));
    total_bytes = st7305_calc_buffer_size(&s_runtime.config);
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_CASET), TAG, "CASET failed");
    ESP_RETURN_ON_ERROR(st7305_tx_data_chunked((const uint8_t[]){ 0x12, 0x2A }, 2), TAG, "CASET data failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_RASET), TAG, "RASET failed");
    ESP_RETURN_ON_ERROR(st7305_tx_data_chunked((const uint8_t[]){ 0x00, 0xC7 }, 2), TAG, "RASET data failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_RAMWR), TAG, "RAMWR failed");

    while (total_bytes > 0) {
        const size_t burst = (total_bytes > sizeof(chunk)) ? sizeof(chunk) : total_bytes;
        ESP_RETURN_ON_ERROR(st7305_protocol_tx_data(chunk, burst), TAG, "raw pattern write failed");
        total_bytes -= burst;
    }

    return ESP_OK;
}

esp_err_t st7305_draw_test_pattern(void)
{
    uint8_t *buffer;
    const int width = s_runtime.config.width;
    const int height = s_runtime.config.height;
    const size_t buf_size = st7305_calc_buffer_size(&s_runtime.config);

    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");

    buffer = (uint8_t *)calloc(1, buf_size);
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG, "no mem for test buffer");

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool on = false;

            if (y < height / 4) {
                on = (x < width / 2);
            } else if (y < height / 2) {
                on = ((x / 12) & 1) == 0;
            } else if (y < (height * 3) / 4) {
                on = ((y / 8) & 1) == 0;
            } else {
                on = (((x / 8) + (y / 8)) & 1) == 0;
            }

            st7305_set_pixel_portrait(buffer, width, x, y, on);
        }
    }

    ESP_RETURN_ON_ERROR(st7305_push_buffer(buffer, buf_size), TAG, "push test buffer failed");
    free(buffer);
    return ESP_OK;
}

esp_err_t st7305_draw_bitmap_1bpp(const uint8_t *bitmap, uint16_t width, uint16_t height, bool invert)
{
    uint8_t *buffer;
    const int panel_width = s_runtime.config.width;
    const int panel_height = s_runtime.config.height;
    const size_t row_stride = ((size_t)width + 7U) / 8U;
    const size_t buf_size = st7305_calc_buffer_size(&s_runtime.config);

    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");
    ESP_RETURN_ON_FALSE(bitmap != NULL, ESP_ERR_INVALID_ARG, TAG, "bitmap is null");
    ESP_RETURN_ON_FALSE(width == panel_width, ESP_ERR_INVALID_ARG, TAG, "bitmap width mismatch");
    ESP_RETURN_ON_FALSE(height == panel_height, ESP_ERR_INVALID_ARG, TAG, "bitmap height mismatch");

    buffer = (uint8_t *)calloc(1, buf_size);
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_NO_MEM, TAG, "no mem for bitmap buffer");

    for (int y = 0; y < panel_height; ++y) {
        for (int x = 0; x < panel_width; ++x) {
            const size_t src_index = ((size_t)y * row_stride) + ((size_t)x >> 3);
            const uint8_t src_mask = (uint8_t)(1U << (7 - (x & 0x07)));
            bool on = (bitmap[src_index] & src_mask) != 0;

            if (invert) {
                on = !on;
            }

            st7305_set_pixel_portrait(buffer, panel_width, x, y, on);
        }
    }

    esp_err_t ret = st7305_push_buffer(buffer, buf_size);
    free(buffer);
    ESP_RETURN_ON_ERROR(ret, TAG, "push bitmap buffer failed");
    return ret;
}

esp_err_t st7305_display_on(bool enable)
{
    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");
    return st7305_protocol_tx_cmd(enable ? ST7305_CMD_DISPON : ST7305_CMD_DISPOFF);
}

esp_err_t st7305_set_invert(bool enable)
{
    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");
    return st7305_protocol_tx_cmd(enable ? ST7305_CMD_INVON : ST7305_CMD_INVOFF);
}

