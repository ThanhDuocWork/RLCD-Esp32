idf_component_register(
    SRCS "src/product_speaker.c"
    INCLUDE_DIRS "include"
    REQUIRES system_manager display audio input sensor storage home
)
