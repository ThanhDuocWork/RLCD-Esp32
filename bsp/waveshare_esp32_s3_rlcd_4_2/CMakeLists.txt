idf_component_register(
    SRCS
        "src/board.c"
        "src/board_config.c"
        "src/board_features.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_driver_spi
)
