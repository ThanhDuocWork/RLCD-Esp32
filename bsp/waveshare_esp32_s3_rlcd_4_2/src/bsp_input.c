#include "bsp_input.h"

static const bsp_input_button_config_t s_key_button_config = {
    .name = "KEY",
    .gpio = 18,
    .active_low = true,
    .enable_pullup = true,
};

const bsp_input_button_config_t *bsp_input_get_key_button_config(void)
{
    return &s_key_button_config;
}
