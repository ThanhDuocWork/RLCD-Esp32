#include "home_app.h"

#include "audio_service.h"
#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"
#include "home_ui.h"
#include "input_service.h"

static const char *TAG = "home_app";

static home_ui_menu_item_t s_selected_item = HOME_UI_MENU_ITEM_HOME;
static bool s_in_menu = true;
static bool s_started;

static home_ui_menu_item_t home_app_next_item(home_ui_menu_item_t item)
{
    return (home_ui_menu_item_t)(((uint8_t)item + 1U) % HOME_UI_MENU_ITEM_COUNT);
}

static esp_err_t home_app_show_current_page(void)
{
    if (s_selected_item == HOME_UI_MENU_ITEM_AUDIO) {
        return home_ui_show_audio_page(audio_service_is_playing());
    }

    return home_ui_show_page(s_selected_item);
}

static void home_app_handle_short_press(void)
{
    esp_err_t ret;

    if (!s_in_menu) {
        if (s_selected_item == HOME_UI_MENU_ITEM_AUDIO) {
            ret = audio_service_toggle_default_playback();
            if (ret == ESP_OK) {
                ret = home_app_show_current_page();
            }
        } else {
            s_in_menu = true;
            ret = home_ui_show_menu(s_selected_item);
        }

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to handle short press in page: %s", esp_err_to_name(ret));
        }
        return;
    }

    s_selected_item = home_app_next_item(s_selected_item);
    ret = home_ui_show_menu(s_selected_item);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to select menu item: %s", esp_err_to_name(ret));
    }
}

static void home_app_handle_long_press(void)
{
    esp_err_t ret;

    if (s_in_menu) {
        s_in_menu = false;
        ESP_LOGI(TAG, "Enter UI: %d", (int)s_selected_item);
        ret = home_app_show_current_page();
    } else {
        s_in_menu = true;
        ret = home_ui_show_menu(s_selected_item);
    }

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to handle long press: %s", esp_err_to_name(ret));
    }
}

static void home_app_input_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const input_service_key_event_t *key_event = (const input_service_key_event_t *)event_data;

    (void)handler_arg;
    (void)event_base;

    if (!s_started || event_id != APP_EVENT_INPUT_KEY || key_event == NULL) {
        return;
    }

    if (key_event->button != INPUT_SERVICE_BUTTON_KEY) {
        return;
    }

    if (key_event->action == INPUT_SERVICE_KEY_ACTION_LONG_PRESS) {
        home_app_handle_long_press();
    } else {
        home_app_handle_short_press();
    }
}

static void home_app_audio_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const audio_service_event_t *audio_event = (const audio_service_event_t *)event_data;

    (void)handler_arg;
    (void)event_base;

    if (!s_started || event_id != APP_EVENT_AUDIO_STATE_CHANGED || audio_event == NULL) {
        return;
    }

    if (!s_in_menu && s_selected_item == HOME_UI_MENU_ITEM_AUDIO) {
        esp_err_t ret = home_ui_show_audio_page(audio_event->state == AUDIO_SERVICE_STATE_PLAYING);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Refresh audio page failed: %s", esp_err_to_name(ret));
        }
    }
}

esp_err_t home_app_start(void)
{
    ESP_LOGI(TAG, "Start home app");
    ESP_RETURN_ON_ERROR(event_hub_register(APP_EVENT_INPUT_KEY, home_app_input_handler, NULL),
                        TAG,
                        "Register input handler failed");
    ESP_RETURN_ON_ERROR(event_hub_register(APP_EVENT_AUDIO_STATE_CHANGED, home_app_audio_handler, NULL),
                        TAG,
                        "Register audio handler failed");
    ESP_RETURN_ON_ERROR(home_ui_init(), TAG, "Home LVGL UI init failed");
    ESP_RETURN_ON_ERROR(home_ui_show_menu(s_selected_item), TAG, "Show main menu failed");
    s_started = true;

    ESP_RETURN_ON_ERROR(event_hub_post(APP_EVENT_DISPLAY_BOOT_SCREEN, NULL, 0, 0),
                        TAG,
                        "Post boot event failed");
    return ESP_OK;
}
