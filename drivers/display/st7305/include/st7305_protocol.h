#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    esp_err_t (*tx_cmd)(void *user_ctx, uint8_t cmd);
    esp_err_t (*tx_data)(void *user_ctx, const void *data, size_t len);
    esp_err_t (*set_window)(void *user_ctx, uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye);
    void (*delay_ms)(uint32_t ms);
    void *user_ctx;
} st7305_protocol_io_t;

typedef struct {
    uint8_t cmd;
    const uint8_t *data;
    uint8_t data_len;
    uint16_t delay_ms;
} st7305_init_cmd_t;

#define ST7305_CMD_CASET    0x2A
#define ST7305_CMD_RASET    0x2B
#define ST7305_CMD_RAMWR    0x2C
#define ST7305_CMD_MADCTL   0x36
#define ST7305_CMD_SWRESET  0x01
#define ST7305_CMD_SLPOUT   0x11
#define ST7305_CMD_DISPON   0x29
#define ST7305_CMD_DISPOFF  0x28
#define ST7305_CMD_INVON    0x21
#define ST7305_CMD_INVOFF   0x20
#define ST7305_CMD_COLMOD   0x3A

#define ST7305_MADCTL_MY    0x80
#define ST7305_MADCTL_MX    0x40
#define ST7305_MADCTL_MV    0x20
#define ST7305_MADCTL_ML    0x10
#define ST7305_MADCTL_RGB   0x00

esp_err_t st7305_protocol_init(const st7305_protocol_io_t *io);
esp_err_t st7305_protocol_tx_cmd(uint8_t cmd);
esp_err_t st7305_protocol_tx_data(const void *data, size_t len);
esp_err_t st7305_protocol_run_init_table(const st7305_init_cmd_t *table, size_t count);
esp_err_t st7305_protocol_set_rotation(uint8_t madctl_value);
esp_err_t st7305_protocol_set_window(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye);
esp_err_t st7305_protocol_memory_write(const void *data, size_t len);
