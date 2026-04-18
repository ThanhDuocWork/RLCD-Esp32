#pragma once

#include <stdbool.h>

typedef struct {
    const char *mount_point;
    int clk_gpio;
    int cmd_gpio;
    int d0_gpio;
    bool use_internal_pullups;
    int max_files;
    bool format_if_mount_failed;
} bsp_storage_sdcard_config_t;

const bsp_storage_sdcard_config_t *bsp_storage_get_sdcard_config(void);
