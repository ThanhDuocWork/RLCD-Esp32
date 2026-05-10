#pragma once

#include "esp_err.h"
#include "lvgl.h"

esp_err_t lvgl_port_init(void);
bool lvgl_port_is_ready(void);
lv_display_t *lvgl_port_get_display(void);
esp_err_t lvgl_port_lock(void);
void lvgl_port_unlock(void);
