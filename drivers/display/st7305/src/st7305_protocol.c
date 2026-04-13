#include "st7305_protocol.h"

#include <stdbool.h>

#include "esp_check.h"

static st7305_protocol_io_t s_io;
static bool s_is_ready;

static esp_err_t protocol_require_ready(void)
{
    return s_is_ready ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t st7305_protocol_init(const st7305_protocol_io_t *io)
{
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_ARG, "st7305_protocol", "io is null");
    ESP_RETURN_ON_FALSE(io->tx_cmd != NULL, ESP_ERR_INVALID_ARG, "st7305_protocol", "tx_cmd is null");
    ESP_RETURN_ON_FALSE(io->tx_data != NULL, ESP_ERR_INVALID_ARG, "st7305_protocol", "tx_data is null");
    ESP_RETURN_ON_FALSE(io->delay_ms != NULL, ESP_ERR_INVALID_ARG, "st7305_protocol", "delay_ms is null");

    s_io = *io;
    s_is_ready = true;
    return ESP_OK;
}

esp_err_t st7305_protocol_tx_cmd(uint8_t cmd)
{
    ESP_RETURN_ON_ERROR(protocol_require_ready(), "st7305_protocol", "protocol is not initialized");
    return s_io.tx_cmd(s_io.user_ctx, cmd);
}

esp_err_t st7305_protocol_tx_data(const void *data, size_t len)
{
    ESP_RETURN_ON_ERROR(protocol_require_ready(), "st7305_protocol", "protocol is not initialized");
    return s_io.tx_data(s_io.user_ctx, data, len);
}

esp_err_t st7305_protocol_run_init_table(const st7305_init_cmd_t *table, size_t count)
{
    ESP_RETURN_ON_FALSE(table != NULL, ESP_ERR_INVALID_ARG, "st7305_protocol", "table is null");
    ESP_RETURN_ON_ERROR(protocol_require_ready(), "st7305_protocol", "protocol is not initialized");

    for (size_t i = 0; i < count; ++i) {
        ESP_RETURN_ON_ERROR(s_io.tx_cmd(s_io.user_ctx, table[i].cmd), "st7305_protocol", "send init cmd failed");
        if ((table[i].data != NULL) && (table[i].data_len > 0)) {
            ESP_RETURN_ON_ERROR(s_io.tx_data(s_io.user_ctx, table[i].data, table[i].data_len),
                                "st7305_protocol",
                                "send init data failed");
        }
        if (table[i].delay_ms > 0) {
            s_io.delay_ms(table[i].delay_ms);
        }
    }

    return ESP_OK;
}

esp_err_t st7305_protocol_set_rotation(uint8_t madctl_value)
{
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_MADCTL), "st7305_protocol", "MADCTL cmd failed");
    return st7305_protocol_tx_data(&madctl_value, sizeof(madctl_value));
}

esp_err_t st7305_protocol_set_window(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye)
{
    uint8_t column_data[2] = {
        (uint8_t)(xs & 0x3F),
        (uint8_t)(xe & 0x3F),
    };
    uint8_t row_data[2] = {
        (uint8_t)(ys & 0xFF),
        (uint8_t)(ye & 0xFF),
    };

    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_CASET), "st7305_protocol", "CASET cmd failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_data(column_data, sizeof(column_data)),
                        "st7305_protocol",
                        "CASET data failed");
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_RASET), "st7305_protocol", "RASET cmd failed");
    return st7305_protocol_tx_data(row_data, sizeof(row_data));
}

esp_err_t st7305_protocol_memory_write(const void *data, size_t len)
{
    ESP_RETURN_ON_ERROR(st7305_protocol_tx_cmd(ST7305_CMD_RAMWR), "st7305_protocol", "RAMWR cmd failed");
    return st7305_protocol_tx_data(data, len);
}
