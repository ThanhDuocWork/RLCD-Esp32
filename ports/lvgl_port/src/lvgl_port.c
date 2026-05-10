#include "lvgl_port.h"

#include <stdlib.h>

#include "display_port.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "lvgl_port";

#define LVGL_PORT_TICK_MS 2
#define LVGL_PORT_TASK_STACK 12288
#define LVGL_PORT_TASK_PRIO 2

static lv_display_t *s_display;
static uint8_t *s_draw_buffer_raw;
static uint8_t *s_draw_buffer;
static SemaphoreHandle_t s_lvgl_mutex;
static TaskHandle_t s_lvgl_task;
static esp_timer_handle_t s_tick_timer;
static bool s_ready;

static void lvgl_port_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_PORT_TICK_MS);
}

static void lvgl_port_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    display_port_size_t size = display_port_get_size();
    const uint8_t *bitmap = px_map;

    (void)display;

    if (area->x1 != 0 || area->y1 != 0 || area->x2 != (int32_t)size.width - 1 || area->y2 != (int32_t)size.height - 1) {
        ESP_LOGW(TAG, "Only full-screen LVGL flush is supported for now");
        lv_display_flush_ready(display);
        return;
    }

    /*
     * LVGL I1 buffers reserve the first 8 bytes for a palette. The actual
     * 1bpp pixel payload starts after that palette block.
     */
    bitmap += 8;
    if (display_port_draw_bitmap_1bpp(bitmap, size.width, size.height, false) != ESP_OK) {
        ESP_LOGE(TAG, "LVGL flush failed");
    }

    lv_display_flush_ready(display);
}

static void lvgl_port_task(void *arg)
{
    uint32_t loop_count = 0;

    (void)arg;

    while (true) {
        if (lvgl_port_lock() == ESP_OK) {
            lv_timer_handler();
            lvgl_port_unlock();
        }
        loop_count++;
        if ((loop_count % 500U) == 0U) {
            ESP_LOGD(TAG, "LVGL stack watermark=%u", (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t lvgl_port_init(void)
{
    display_port_size_t size;
    size_t draw_size;
    size_t alloc_size;
    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_port_tick_cb,
        .name = "lvgl_tick",
    };

    if (s_ready) {
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(display_port_is_ready(), ESP_ERR_INVALID_STATE, TAG, "Display port is not ready");

    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    ESP_RETURN_ON_FALSE(s_lvgl_mutex != NULL, ESP_ERR_NO_MEM, TAG, "No memory for LVGL mutex");

    lv_init();

    size = display_port_get_size();
    draw_size = (size_t)lv_draw_buf_width_to_stride(size.width, LV_COLOR_FORMAT_I1) * (size_t)size.height + 8U;
    alloc_size = draw_size + LV_DRAW_BUF_ALIGN;
    s_draw_buffer_raw = (uint8_t *)heap_caps_malloc(alloc_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(s_draw_buffer_raw != NULL, ESP_ERR_NO_MEM, TAG, "No memory for LVGL draw buffer");
    s_draw_buffer = (uint8_t *)lv_draw_buf_align(s_draw_buffer_raw, LV_COLOR_FORMAT_I1);
    ESP_RETURN_ON_FALSE(s_draw_buffer != NULL, ESP_ERR_INVALID_STATE, TAG, "Align LVGL draw buffer failed");

    s_display = lv_display_create(size.width, size.height);
    ESP_RETURN_ON_FALSE(s_display != NULL, ESP_ERR_NO_MEM, TAG, "Create LVGL display failed");
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_I1);
    lv_display_set_flush_cb(s_display, lvgl_port_flush_cb);
    lv_display_set_buffers(s_display, s_draw_buffer, NULL, draw_size, LV_DISPLAY_RENDER_MODE_FULL);

    ESP_RETURN_ON_ERROR(esp_timer_create(&tick_timer_args, &s_tick_timer), TAG, "Create LVGL tick timer failed");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_tick_timer, LVGL_PORT_TICK_MS * 1000), TAG, "Start LVGL tick failed");

    BaseType_t ok = xTaskCreate(lvgl_port_task,
                                "lvgl_port",
                                LVGL_PORT_TASK_STACK,
                                NULL,
                                LVGL_PORT_TASK_PRIO,
                                &s_lvgl_task);
    ESP_RETURN_ON_FALSE(ok == pdPASS, ESP_ERR_NO_MEM, TAG, "Create LVGL task failed");

    s_ready = true;
    ESP_LOGI(TAG, "LVGL ready: %ux%u I1 draw=%u alloc=%u", size.width, size.height, (unsigned)draw_size, (unsigned)alloc_size);
    return ESP_OK;
}

bool lvgl_port_is_ready(void)
{
    return s_ready;
}

lv_display_t *lvgl_port_get_display(void)
{
    return s_display;
}

esp_err_t lvgl_port_lock(void)
{
    ESP_RETURN_ON_FALSE(s_lvgl_mutex != NULL, ESP_ERR_INVALID_STATE, TAG, "LVGL mutex is not ready");
    return xSemaphoreTakeRecursive(s_lvgl_mutex, portMAX_DELAY) == pdTRUE ? ESP_OK : ESP_FAIL;
}

void lvgl_port_unlock(void)
{
    if (s_lvgl_mutex != NULL) {
        xSemaphoreGiveRecursive(s_lvgl_mutex);
    }
}
