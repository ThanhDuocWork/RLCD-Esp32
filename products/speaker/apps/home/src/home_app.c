#include "home_app.h"

#include "display_service.h"
#include "esp_check.h"
#include "esp_log.h"
#include "event_hub.h"
#include "image_service.h"

static const char *TAG = "home_app";

esp_err_t home_app_start(void)
{
    const image_service_mono_config_t boot_image_config = {
        .threshold = 128,
        .dither = true,
        .invert = false,
    };
    esp_err_t image_ret;

    ESP_LOGI(TAG, "Start home app");
    image_ret = image_service_show_bmp_1bpp("/sdcard/boot.bmp", &boot_image_config);
    if (image_ret != ESP_OK) {
        ESP_LOGW(TAG, "Boot BMP unavailable, fallback to display test loop: %s", esp_err_to_name(image_ret));
        ESP_RETURN_ON_ERROR(display_service_show_boot(), TAG, "Boot screen failed");
    }
    ESP_RETURN_ON_ERROR(event_hub_post(APP_EVENT_DISPLAY_BOOT_SCREEN, NULL, 0, 0),
                        TAG,
                        "Post boot event failed");
    return ESP_OK;
}
