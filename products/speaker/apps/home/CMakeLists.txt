idf_component_register(
    SRCS "src/home_app.c"
    INCLUDE_DIRS "include" "../../resources/images"
    REQUIRES display event_hub image input
)
