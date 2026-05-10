idf_component_register(
    SRCS "src/home_app.c" "src/home_ui.c"
    INCLUDE_DIRS "include" "../../resources/images"
    REQUIRES audio display event_hub image input lvgl_port
)
