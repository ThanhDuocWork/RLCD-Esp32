idf_component_register(
    SRCS "src/audio_port.c"
    INCLUDE_DIRS "include"
    REQUIRES waveshare_esp32_s3_rlcd_4_2 esp_driver_i2s i2c_master es8311
)
