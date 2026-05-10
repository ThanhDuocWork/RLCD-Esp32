idf_component_register(
    SRCS "src/audio_service.c"
    INCLUDE_DIRS "include"
    REQUIRES event_hub audio_port storage
)
