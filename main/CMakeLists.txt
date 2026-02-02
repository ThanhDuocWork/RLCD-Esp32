set(srcs
   
   
    )

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_spi esp_driver_gpio
                    EMBED_FILES image.jpg)
