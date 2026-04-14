idf_component_register(
    SRCS
        "src/st7305.c"
        "src/st7305_protocol.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
)
