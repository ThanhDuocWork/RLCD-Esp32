#include "bsp_storage.h"

static const bsp_storage_sdcard_config_t s_sdcard_config = {
    .mount_point = "/sdcard",
    .clk_gpio = 38,
    .cmd_gpio = 21,
    .d0_gpio = 39,
    .use_internal_pullups = true,
    .max_files = 5,
    .format_if_mount_failed = false,
};

const bsp_storage_sdcard_config_t *bsp_storage_get_sdcard_config(void)
{
    return &s_sdcard_config;
}
