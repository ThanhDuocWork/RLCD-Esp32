idf_component_register(
    SRCS
        "src/board.c"
        "src/board_config.c"
        "src/board_features.c"
        "src/bsp_display.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_driver_spi spi_master
)
