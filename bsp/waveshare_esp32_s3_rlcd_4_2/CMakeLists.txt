idf_component_register(
    SRCS
        "src/board.c"
        "src/board_config.c"
        "src/board_features.c"
        "src/bsp_display.c"
        "src/bsp_audio.c"
        "src/bsp_input.c"
        "src/bsp_storage.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_driver_spi esp_driver_i2s spi_master
)
