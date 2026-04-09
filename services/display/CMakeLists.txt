idf_component_register(
    SRCS "src/display_service.c"
    INCLUDE_DIRS "include"
    REQUIRES display_port event_hub
)
