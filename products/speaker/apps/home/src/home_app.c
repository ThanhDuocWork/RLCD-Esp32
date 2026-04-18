#include "home_app.h"

#include "display_service.h"
#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"
#include "image_service.h"
#include "input_service.h"

static const char *TAG = "home_app";

typedef struct {
    const char *name;
    const char *path;
} home_app_image_entry_t;

static const home_app_image_entry_t s_ui_images[] = {
    { .name = "home", .path = "/sdcard/images/home.bmp" },
    { .name = "settings", .path = "/sdcard/images/settings.bmp" },
};

static const image_service_mono_config_t s_image_config = {
    .threshold = 128,
    .dither = true,
    .invert = false,
};

static size_t s_current_image_index;
static bool s_started;

static esp_err_t home_app_show_image(size_t index)
{
    const home_app_image_entry_t *image;

    ESP_RETURN_ON_FALSE(index < (sizeof(s_ui_images) / sizeof(s_ui_images[0])),
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "Image index out of range");

    image = &s_ui_images[index];
    ESP_LOGI(TAG, "Show UI image: %s (%s)", image->name, image->path);
    return image_service_show_bmp_1bpp(image->path, &s_image_config);
}

static esp_err_t home_app_show_first_available_image(void)
{
    esp_err_t last_error = ESP_FAIL;

    for (size_t i = 0; i < (sizeof(s_ui_images) / sizeof(s_ui_images[0])); ++i) {
        last_error = home_app_show_image(i);
        if (last_error == ESP_OK) {
            s_current_image_index = i;
            return ESP_OK;
        }

        ESP_LOGW(TAG, "UI image unavailable: %s (%s)", s_ui_images[i].name, esp_err_to_name(last_error));
    }

    ESP_LOGW(TAG, "UI images unavailable, try legacy /sdcard/cat.bmp");
    last_error = image_service_show_bmp_1bpp("/sdcard/cat.bmp", &s_image_config);
    if (last_error == ESP_OK) {
        return ESP_OK;
    }

    ESP_LOGW(TAG, "Boot BMP unavailable, fallback to display test loop: %s", esp_err_to_name(last_error));
    return display_service_show_boot();
}

static void home_app_input_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const size_t image_count = sizeof(s_ui_images) / sizeof(s_ui_images[0]);
    const input_service_key_event_t *key_event = (const input_service_key_event_t *)event_data;
    esp_err_t ret;

    (void)handler_arg;
    (void)event_base;

    if (!s_started || event_id != APP_EVENT_INPUT_KEY || key_event == NULL) {
        return;
    }

    if (key_event->button != INPUT_SERVICE_BUTTON_KEY) {
        return;
    }

    s_current_image_index = (s_current_image_index + 1U) % image_count;
    ret = home_app_show_image(s_current_image_index);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG,
                 "Failed to show UI image %s: %s",
                 s_ui_images[s_current_image_index].name,
                 esp_err_to_name(ret));
    }
}

esp_err_t home_app_start(void)
{
    ESP_LOGI(TAG, "Start home app");
    ESP_RETURN_ON_ERROR(event_hub_register(APP_EVENT_INPUT_KEY, home_app_input_handler, NULL),
                        TAG,
                        "Register input handler failed");
    s_started = true;

    ESP_RETURN_ON_ERROR(home_app_show_first_available_image(), TAG, "Boot screen failed");
    ESP_RETURN_ON_ERROR(event_hub_post(APP_EVENT_DISPLAY_BOOT_SCREEN, NULL, 0, 0),
                        TAG,
                        "Post boot event failed");
    return ESP_OK;
}
