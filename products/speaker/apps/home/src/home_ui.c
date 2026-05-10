#include "home_ui.h"

#include "esp_check.h"
#include "esp_log.h"
#include "lvgl_port.h"

static const char *TAG = "home_ui";

static lv_obj_t *s_root;
static bool s_ready;

static const char *home_ui_item_name(home_ui_menu_item_t item)
{
    switch (item) {
    case HOME_UI_MENU_ITEM_SETTINGS:
        return "Settings";
    case HOME_UI_MENU_ITEM_AUDIO:
        return "Play Audio";
    case HOME_UI_MENU_ITEM_HOME:
    default:
        return "Home";
    }
}

static const char *home_ui_item_hint(home_ui_menu_item_t item)
{
    switch (item) {
    case HOME_UI_MENU_ITEM_SETTINGS:
        return "device options";
    case HOME_UI_MENU_ITEM_AUDIO:
        return "speaker test";
    case HOME_UI_MENU_ITEM_HOME:
    default:
        return "dashboard";
    }
}

static void home_ui_clear(void)
{
    if (s_root != NULL) {
        lv_obj_clean(s_root);
    }
}

static lv_obj_t *home_ui_make_label(lv_obj_t *parent, const char *text, lv_align_t align, int32_t x, int32_t y)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_align(label, align, x, y);
    return label;
}

static void home_ui_make_line(lv_obj_t *parent, const lv_point_precise_t *points, uint32_t point_count, uint8_t width)
{
    lv_obj_t *line = lv_line_create(parent);

    lv_line_set_points(line, points, point_count);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
    lv_obj_set_style_line_rounded(line, true, 0);
}

static void home_ui_make_hline(int32_t y, int32_t x1, int32_t x2, uint8_t width)
{
    lv_obj_t *line = lv_obj_create(s_root);

    lv_obj_set_size(line, x2 - x1, width);
    lv_obj_set_pos(line, x1, y);
    lv_obj_set_style_bg_color(line, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *home_ui_make_clean_box(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *box = lv_obj_create(parent);

    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

static void home_ui_draw_home_icon(lv_obj_t *parent, int32_t x, int32_t y, uint8_t scale)
{
    const int32_t s = scale;
    static const lv_point_precise_t roof_scale_1[] = {
        { 0, 24 },
        { 28, 0 },
        { 56, 24 },
    };
    static const lv_point_precise_t roof_scale_2[] = {
        { 0, 48 },
        { 56, 0 },
        { 112, 48 },
    };
    const lv_point_precise_t *roof = (scale == 2) ? roof_scale_2 : roof_scale_1;
    lv_obj_t *icon = home_ui_make_clean_box(parent, x, y, 64 * s, 64 * s);

    home_ui_make_line(icon, roof, 3, 3 * s);

    lv_obj_t *body = lv_obj_create(icon);
    lv_obj_set_size(body, 40 * s, 30 * s);
    lv_obj_set_pos(body, 8 * s, 25 * s);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 3 * s, 0);
    lv_obj_set_style_border_color(body, lv_color_black(), 0);
    lv_obj_set_style_radius(body, 0, 0);
}

static void home_ui_draw_settings_icon(lv_obj_t *parent, int32_t x, int32_t y, uint8_t scale)
{
    const int32_t s = scale;
    lv_obj_t *outer = lv_obj_create(parent);
    lv_obj_set_size(outer, 44 * s, 44 * s);
    lv_obj_set_pos(outer, x + 8 * s, y + 8 * s);
    lv_obj_set_style_bg_opa(outer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(outer, 4 * s, 0);
    lv_obj_set_style_border_color(outer, lv_color_black(), 0);
    lv_obj_set_style_radius(outer, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *inner = lv_obj_create(parent);
    lv_obj_set_size(inner, 16 * s, 16 * s);
    lv_obj_set_pos(inner, x + 22 * s, y + 22 * s);
    lv_obj_set_style_bg_opa(inner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(inner, 3 * s, 0);
    lv_obj_set_style_border_color(inner, lv_color_black(), 0);
    lv_obj_set_style_radius(inner, LV_RADIUS_CIRCLE, 0);
}

static void home_ui_draw_audio_icon(lv_obj_t *parent, int32_t x, int32_t y, uint8_t scale)
{
    const int32_t s = scale;
    static const lv_point_precise_t triangle_scale_1[] = {
        { 0, 0 },
        { 0, 42 },
        { 38, 21 },
        { 0, 0 },
    };
    static const lv_point_precise_t triangle_scale_2[] = {
        { 0, 0 },
        { 0, 84 },
        { 76, 42 },
        { 0, 0 },
    };
    const lv_point_precise_t *triangle = (scale == 2) ? triangle_scale_2 : triangle_scale_1;
    lv_obj_t *icon = home_ui_make_clean_box(parent, x + 12 * s, y + 8 * s, 56 * s, 56 * s);

    home_ui_make_line(icon, triangle, 4, 4 * s);
}

static void home_ui_draw_icon(lv_obj_t *parent, home_ui_menu_item_t item, int32_t x, int32_t y, uint8_t scale)
{
    switch (item) {
    case HOME_UI_MENU_ITEM_SETTINGS:
        home_ui_draw_settings_icon(parent, x, y, scale);
        break;
    case HOME_UI_MENU_ITEM_AUDIO:
        home_ui_draw_audio_icon(parent, x, y, scale);
        break;
    case HOME_UI_MENU_ITEM_HOME:
    default:
        home_ui_draw_home_icon(parent, x, y, scale);
        break;
    }
}

static void home_ui_make_header(const char *title, const char *subtitle)
{
    lv_obj_t *label = home_ui_make_label(s_root, title, LV_ALIGN_TOP_LEFT, 18, 16);
    lv_obj_set_style_text_letter_space(label, 1, 0);
    home_ui_make_label(s_root, subtitle, LV_ALIGN_TOP_RIGHT, -18, 18);
    home_ui_make_hline(54, 18, 282, 3);
}

static void home_ui_make_footer(const char *text)
{
    home_ui_make_hline(354, 18, 282, 2);
    home_ui_make_label(s_root, text, LV_ALIGN_BOTTOM_MID, 0, -18);
}

static void home_ui_make_menu_item(home_ui_menu_item_t item, uint8_t row, bool selected)
{
    const int32_t y = 76 + row * 86;
    lv_obj_t *box = lv_obj_create(s_root);
    lv_obj_t *label;
    lv_obj_t *hint;

    lv_obj_set_size(box, 264, 72);
    lv_obj_set_pos(box, 18, y);
    lv_obj_set_style_border_width(box, selected ? 5 : 2, 0);
    lv_obj_set_style_border_color(box, lv_color_black(), 0);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_set_style_bg_color(box, selected ? lv_color_black() : lv_color_white(), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    if (selected) {
        label = lv_label_create(box);
        lv_label_set_text(label, "> ");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);
    } else {
        home_ui_draw_icon(box, item, 14, 7, 1);
    }

    label = lv_label_create(box);
    lv_label_set_text(label, home_ui_item_name(item));
    lv_obj_set_style_text_color(label, selected ? lv_color_white() : lv_color_black(), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, selected ? 48 : 84, -10);

    hint = lv_label_create(box);
    lv_label_set_text(hint, home_ui_item_hint(item));
    lv_obj_set_style_text_color(hint, selected ? lv_color_white() : lv_color_black(), 0);
    lv_obj_align(hint, LV_ALIGN_LEFT_MID, selected ? 48 : 84, 16);
}

esp_err_t home_ui_init(void)
{
    ESP_RETURN_ON_ERROR(lvgl_port_init(), TAG, "LVGL port init failed");
    ESP_RETURN_ON_ERROR(lvgl_port_lock(), TAG, "LVGL lock failed");

    s_root = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_root, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_root, 0, 0);
    lv_obj_set_style_pad_all(s_root, 0, 0);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_screen_load(s_root);

    s_ready = true;
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Home LVGL UI ready");
    return home_ui_show_menu(HOME_UI_MENU_ITEM_HOME);
}

esp_err_t home_ui_show_menu(home_ui_menu_item_t selected)
{
    ESP_RETURN_ON_FALSE(s_ready, ESP_ERR_INVALID_STATE, TAG, "Home UI is not ready");
    ESP_RETURN_ON_FALSE(selected < HOME_UI_MENU_ITEM_COUNT, ESP_ERR_INVALID_ARG, TAG, "Invalid menu item");
    ESP_RETURN_ON_ERROR(lvgl_port_lock(), TAG, "LVGL lock failed");

    home_ui_clear();
    home_ui_make_header("RLCD Speaker", "Menu");
    home_ui_make_menu_item(HOME_UI_MENU_ITEM_HOME, 0, selected == HOME_UI_MENU_ITEM_HOME);
    home_ui_make_menu_item(HOME_UI_MENU_ITEM_SETTINGS, 1, selected == HOME_UI_MENU_ITEM_SETTINGS);
    home_ui_make_menu_item(HOME_UI_MENU_ITEM_AUDIO, 2, selected == HOME_UI_MENU_ITEM_AUDIO);
    home_ui_make_footer("click: move     hold: open");

    lv_obj_invalidate(s_root);
    lvgl_port_unlock();
    return ESP_OK;
}

esp_err_t home_ui_show_page(home_ui_menu_item_t page)
{
    ESP_RETURN_ON_FALSE(s_ready, ESP_ERR_INVALID_STATE, TAG, "Home UI is not ready");
    ESP_RETURN_ON_FALSE(page < HOME_UI_MENU_ITEM_COUNT, ESP_ERR_INVALID_ARG, TAG, "Invalid page");
    ESP_RETURN_ON_ERROR(lvgl_port_lock(), TAG, "LVGL lock failed");

    home_ui_clear();
    home_ui_make_header(home_ui_item_name(page), "App");
    home_ui_draw_icon(s_root, page, 98, 94, 2);

    switch (page) {
    case HOME_UI_MENU_ITEM_SETTINGS:
        home_ui_make_label(s_root, "Device settings", LV_ALIGN_CENTER, 0, 70);
        home_ui_make_label(s_root, "WiFi / Display / Power", LV_ALIGN_CENTER, 0, 98);
        break;
    case HOME_UI_MENU_ITEM_AUDIO:
        home_ui_make_label(s_root, "Audio player", LV_ALIGN_CENTER, 0, 70);
        home_ui_make_label(s_root, "I2S test tone next", LV_ALIGN_CENTER, 0, 98);
        break;
    case HOME_UI_MENU_ITEM_HOME:
    default:
        home_ui_make_label(s_root, "Product dashboard", LV_ALIGN_CENTER, 0, 70);
        home_ui_make_label(s_root, "RTC / Sensor / Status", LV_ALIGN_CENTER, 0, 98);
        break;
    }

    home_ui_make_footer("click: menu     hold: menu");
    lv_obj_invalidate(s_root);
    lvgl_port_unlock();
    return ESP_OK;
}

esp_err_t home_ui_show_audio_page(bool is_playing)
{
    ESP_RETURN_ON_FALSE(s_ready, ESP_ERR_INVALID_STATE, TAG, "Home UI is not ready");
    ESP_RETURN_ON_ERROR(lvgl_port_lock(), TAG, "LVGL lock failed");

    home_ui_clear();
    home_ui_make_header("Play Audio", "App");
    home_ui_draw_icon(s_root, HOME_UI_MENU_ITEM_AUDIO, 98, 94, 2);

    if (is_playing) {
        home_ui_make_label(s_root, "Audio test tone", LV_ALIGN_CENTER, 0, 70);
        home_ui_make_label(s_root, "status: playing", LV_ALIGN_CENTER, 0, 98);
        home_ui_make_footer("click: stop     hold: menu");
    } else {
        home_ui_make_label(s_root, "Audio test tone", LV_ALIGN_CENTER, 0, 70);
        home_ui_make_label(s_root, "status: idle", LV_ALIGN_CENTER, 0, 98);
        home_ui_make_footer("click: play     hold: menu");
    }

    lv_obj_invalidate(s_root);
    lvgl_port_unlock();
    return ESP_OK;
}
