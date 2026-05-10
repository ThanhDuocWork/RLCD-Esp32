#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    HOME_UI_MENU_ITEM_HOME = 0,
    HOME_UI_MENU_ITEM_SETTINGS,
    HOME_UI_MENU_ITEM_AUDIO,
    HOME_UI_MENU_ITEM_COUNT,
} home_ui_menu_item_t;

esp_err_t home_ui_init(void);
esp_err_t home_ui_show_menu(home_ui_menu_item_t selected);
esp_err_t home_ui_show_page(home_ui_menu_item_t page);
esp_err_t home_ui_show_audio_page(bool is_playing);
