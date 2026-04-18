idf_component_register(
    SRCS "src/input_service.c"
    INCLUDE_DIRS "include"
    REQUIRES event_hub input_port
)
