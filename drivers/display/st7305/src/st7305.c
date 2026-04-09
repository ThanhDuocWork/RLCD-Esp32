#include "st7305.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spi_master_driver.h"
#include "st7305_protocol.h"

static const char *TAG = "st7305";

typedef struct {
    spi_device_handle_t spi_dev;
    st7305_config_t config;
    bool ready;
} st7305_runtime_t;

static st7305_runtime_t s_runtime;

static const uint8_t s_colmod_12bit = 0x03;
static const uint8_t s_madctl_default = ST7305_MADCTL_MX | ST7305_MADCTL_MY | ST7305_MADCTL_RGB;
static const uint8_t s_gate_line_setting[] = { 0x13 };
static const uint8_t s_first_gate_setting[] = { 0x00 };
static const uint8_t s_frame_rate_setting[] = { 0x00 };
static const uint8_t s_panel_setting[] = { 0x29 };
static const uint8_t s_gate_voltage_setting[] = { 0x12, 0x12 };
static const uint8_t s_vshp_setting[] = { 0x3C };
static const uint8_t s_vslp_setting[] = { 0x3C };
static const uint8_t s_vshn_setting[] = { 0x3C };
static const uint8_t s_vsln_setting[] = { 0x3C };
static const uint8_t s_osc_setting[] = { 0x2A };
static const uint8_t s_booster_enable[] = { 0x14 };

static const st7305_init_cmd_t s_init_table[] = {
    { ST7305_CMD_SWRESET, NULL, 0, 120 },
    { 0xB0, s_gate_line_setting, sizeof(s_gate_line_setting), 0 },
    { 0xB1, s_first_gate_setting, sizeof(s_first_gate_setting), 0 },
    { 0xB2, s_frame_rate_setting, sizeof(s_frame_rate_setting), 0 },
    { 0xB8, s_panel_setting, sizeof(s_panel_setting), 0 },
    { 0xC0, s_gate_voltage_setting, sizeof(s_gate_voltage_setting), 0 },
    { 0xC1, s_vshp_setting, sizeof(s_vshp_setting), 0 },
    { 0xC2, s_vslp_setting, sizeof(s_vslp_setting), 0 },
    { 0xC4, s_vshn_setting, sizeof(s_vshn_setting), 0 },
    { 0xC5, s_vsln_setting, sizeof(s_vsln_setting), 0 },
    { 0xD1, s_booster_enable, sizeof(s_booster_enable), 10 },
    { 0xD8, s_osc_setting, sizeof(s_osc_setting), 0 },
    { ST7305_CMD_SLPOUT, NULL, 0, 120 },
    { ST7305_CMD_MADCTL, &s_madctl_default, 1, 0 },
    { ST7305_CMD_COLMOD, &s_colmod_12bit, 1, 0 },
    { ST7305_CMD_DISPON, NULL, 0, 20 },
};

static void st7305_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static esp_err_t st7305_gpio_init(const st7305_config_t *config)
{
    uint64_t pin_mask = 0;

    if (config->dc_gpio >= 0) {
        pin_mask |= (1ULL << config->dc_gpio);
    }
    if (config->reset_gpio >= 0) {
        pin_mask |= (1ULL << config->reset_gpio);
    }

    if (pin_mask == 0) {
        return ESP_OK;
    }

    const gpio_config_t io_cfg = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
    };

    return gpio_config(&io_cfg);
}

static esp_err_t st7305_spi_attach_device(const st7305_config_t *config, spi_device_handle_t *out_dev)
{
    const spi_device_interface_config_t dev_config = {
        .clock_speed_hz = (int)config->pixel_clock_hz,
        .mode = 0,
        .spics_io_num = config->cs_gpio,
        .queue_size = 4,
    };

    return spi_bus_add_device(config->host_id, &dev_config, out_dev);
}

static esp_err_t st7305_tx_cmd(void *user_ctx, uint8_t cmd)
{
    st7305_runtime_t *runtime = (st7305_runtime_t *)user_ctx;
    spi_transaction_t trans = { 0 };

    gpio_set_level(runtime->config.dc_gpio, 0);
    trans.length = 8;
    trans.tx_buffer = &cmd;
    return spi_device_polling_transmit(runtime->spi_dev, &trans);
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
    return spi_device_polling_transmit(runtime->spi_dev, &trans);
}

static esp_err_t st7305_hw_reset(const st7305_config_t *config)
{
    if (config->reset_gpio < 0) {
        return ESP_OK;
    }

    gpio_set_level(config->reset_gpio, 1);
    st7305_delay_ms(10);
    gpio_set_level(config->reset_gpio, 0);
    st7305_delay_ms(10);
    gpio_set_level(config->reset_gpio, 1);
    st7305_delay_ms(120);
    return ESP_OK;
}

static esp_err_t st7305_require_ready(void)
{
    return s_runtime.ready ? ESP_OK : ESP_ERR_INVALID_STATE;
}

static void st7305_pack_two_pixels(uint16_t rgb444_color, uint8_t packed[3])
{
    packed[0] = (uint8_t)(rgb444_color >> 4);
    packed[1] = (uint8_t)(((rgb444_color & 0x0F) << 4) | ((rgb444_color >> 8) & 0x0F));
    packed[2] = (uint8_t)(rgb444_color & 0xFF);
}

esp_err_t st7305_init(const st7305_config_t *config)
{
    const bus_spi_master_config_t spi_config = {
        .host_id = config->host_id,
        .sclk_gpio = config->sclk_gpio,
        .mosi_gpio = config->mosi_gpio,
        .miso_gpio = config->miso_gpio,
        .max_transfer_sz = config->width * 32,
    };
    const st7305_protocol_io_t proto_io = {
        .tx_cmd = st7305_tx_cmd,
        .tx_data = st7305_tx_data,
        .set_window = NULL,
        .delay_ms = st7305_delay_ms,
        .user_ctx = &s_runtime,
    };

    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is null");
    ESP_RETURN_ON_FALSE(config->cs_gpio >= 0, ESP_ERR_INVALID_ARG, TAG, "cs gpio is not configured");
    ESP_RETURN_ON_FALSE(config->dc_gpio >= 0, ESP_ERR_INVALID_ARG, TAG, "dc gpio is not configured");
    ESP_RETURN_ON_ERROR(spi_master_driver_init(&spi_config), TAG, "SPI driver init failed");
    ESP_RETURN_ON_ERROR(st7305_gpio_init(config), TAG, "GPIO init failed");
    ESP_RETURN_ON_ERROR(st7305_spi_attach_device(config, &s_runtime.spi_dev), TAG, "SPI attach failed");

    s_runtime.config = *config;

    ESP_RETURN_ON_ERROR(st7305_protocol_init(&proto_io), TAG, "Protocol init failed");
    ESP_RETURN_ON_ERROR(st7305_hw_reset(config), TAG, "Hardware reset failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_run_init_table(s_init_table,
                                                       sizeof(s_init_table) / sizeof(s_init_table[0])),
                        TAG,
                        "Panel init sequence failed");

    s_runtime.ready = true;

    ESP_LOGI(TAG,
             "ST7305 initialized: host=%d cs=%d dc=%d reset=%d busy=%d size=%dx%d clk=%lu",
             config->host_id,
             config->cs_gpio,
             config->dc_gpio,
             config->reset_gpio,
             config->busy_gpio,
             config->width,
             config->height,
             (unsigned long)config->pixel_clock_hz);

    return ESP_OK;
}

esp_err_t st7305_fill_color(uint16_t rgb444_color)
{
    uint8_t chunk[96];
    const size_t total_pixels = (size_t)s_runtime.config.width * (size_t)s_runtime.config.height;
    const size_t total_pairs = (total_pixels + 1U) / 2U;
    uint8_t packed[3];

    ESP_RETURN_ON_ERROR(st7305_require_ready(), TAG, "panel is not ready");
    ESP_RETURN_ON_ERROR(st7305_protocol_set_window(0,
                                                   (uint16_t)(s_runtime.config.width - 1),
                                                   0,
                                                   (uint16_t)(s_runtime.config.height - 1)),
                        TAG,
                        "set window failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_RAMWR), TAG, "RAMWR failed");

    st7305_pack_two_pixels(rgb444_color, packed);
    for (size_t i = 0; i < sizeof(chunk); i += sizeof(packed)) {
        memcpy(&chunk[i], packed, sizeof(packed));
    }

    for (size_t pair_index = 0; pair_index < total_pairs;) {
        size_t pairs_this_round = sizeof(chunk) / sizeof(packed);
        size_t bytes_this_round = sizeof(chunk);

        if ((pair_index + pairs_this_round) > total_pairs) {
            pairs_this_round = total_pairs - pair_index;
            bytes_this_round = pairs_this_round * sizeof(packed);
        }

        ESP_RETURN_ON_ERROR(st7305_protocol_tx_data(chunk, bytes_this_round), TAG, "fill data failed");
        pair_index += pairs_this_round;
    }

    return ESP_OK;
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
