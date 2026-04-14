#include "board_config.h"

#include "driver/spi_master.h"

static const board_config_t s_board_config = {
    .lcd = {
        .controller_name = "ST7305",
        .bus_type = BOARD_DISPLAY_BUS_SPI,
        .spi_host_id = SPI2_HOST,
        .spi_mode = 0,
        .sclk_gpio = 11,
        .mosi_gpio = 12,
        .miso_gpio = -1,
        .cs_gpio = 40,
        .dc_gpio = 5,
        .busy_gpio = 6,
        .reset_gpio = 41,
        .power_gpio = -1,
        .spi_clock_hz = 10 * 1000 * 1000,
        .h_res = 300,
        .v_res = 400,
        .column_start = 0x12,
        .column_end = 0x2A,
        .page_start = 0x00,
        .page_end = 0xC7,
        .color_invert = false,
        .supports_partial_refresh = true,
    },
};

const board_config_t *board_config_get_default(void)
{
    return &s_board_config;
}
