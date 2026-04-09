#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BOARD_DISPLAY_BUS_UNKNOWN = 0,
    BOARD_DISPLAY_BUS_SPI,
    BOARD_DISPLAY_BUS_RGB,
} board_display_bus_type_t;

typedef struct {
    const char *controller_name;
    board_display_bus_type_t bus_type;
    int spi_host_id;
    int sclk_gpio;
    int mosi_gpio;
    int miso_gpio;
    int cs_gpio;
    int dc_gpio;
    int busy_gpio;
    int reset_gpio;
    int power_gpio;
    uint32_t spi_clock_hz;
    uint16_t h_res;
    uint16_t v_res;
    bool color_invert;
    bool supports_partial_refresh;
} board_lcd_config_t;

typedef struct {
    board_lcd_config_t lcd;
} board_config_t;

const board_config_t *board_config_get_default(void);
