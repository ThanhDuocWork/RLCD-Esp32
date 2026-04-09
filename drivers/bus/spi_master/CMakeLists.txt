idf_component_register(
    SRCS "src/spi_master_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_spi
)
