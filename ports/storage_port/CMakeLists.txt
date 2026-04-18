idf_component_register(
    SRCS "src/storage_port.c"
    INCLUDE_DIRS "include"
    REQUIRES fatfs sdmmc esp_driver_sdmmc waveshare_esp32_s3_rlcd_4_2
)
