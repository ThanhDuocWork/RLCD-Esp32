#include "image_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display_service.h"
#include "esp_check.h"
#include "esp_log.h"
#include "storage_service.h"

static const char *TAG = "image_service";

typedef struct {
    uint32_t pixel_offset;
    int32_t width;
    int32_t height;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t colors_used;
    uint32_t dib_size;
} bmp_info_t;

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static int32_t read_le32s(const uint8_t *data)
{
    return (int32_t)read_le32(data);
}

static uint8_t clamp_u8(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return (uint8_t)value;
}

static void set_1bpp_pixel(uint8_t *bitmap, int width, int x, int y, bool on)
{
    const int row_stride = (width + 7) / 8;
    const int byte_index = y * row_stride + (x >> 3);
    const uint8_t mask = (uint8_t)(1U << (7 - (x & 0x07)));

    if (on) {
        bitmap[byte_index] |= mask;
    } else {
        bitmap[byte_index] &= (uint8_t)~mask;
    }
}

static esp_err_t read_bmp_info(FILE *file, bmp_info_t *info)
{
    uint8_t header[54];

    ESP_RETURN_ON_FALSE(fread(header, 1, sizeof(header), file) == sizeof(header),
                        ESP_ERR_INVALID_SIZE,
                        TAG,
                        "BMP header is too small");
    ESP_RETURN_ON_FALSE(header[0] == 'B' && header[1] == 'M', ESP_ERR_INVALID_ARG, TAG, "Not a BMP file");

    info->pixel_offset = read_le32(&header[10]);
    info->dib_size = read_le32(&header[14]);
    info->width = read_le32s(&header[18]);
    info->height = read_le32s(&header[22]);
    info->bits_per_pixel = read_le16(&header[28]);
    info->compression = read_le32(&header[30]);
    info->colors_used = read_le32(&header[46]);

    ESP_RETURN_ON_FALSE(info->dib_size >= 40, ESP_ERR_NOT_SUPPORTED, TAG, "Unsupported BMP DIB header");
    ESP_RETURN_ON_FALSE(read_le16(&header[26]) == 1, ESP_ERR_NOT_SUPPORTED, TAG, "Invalid BMP planes");
    ESP_RETURN_ON_FALSE(info->compression == 0, ESP_ERR_NOT_SUPPORTED, TAG, "Compressed BMP is not supported");
    ESP_RETURN_ON_FALSE(info->width > 0 && info->height != 0, ESP_ERR_INVALID_ARG, TAG, "Invalid BMP size");
    ESP_RETURN_ON_FALSE(info->bits_per_pixel == 8 || info->bits_per_pixel == 24 || info->bits_per_pixel == 32,
                        ESP_ERR_NOT_SUPPORTED,
                        TAG,
                        "Only 8/24/32-bit BMP is supported");

    return ESP_OK;
}

static esp_err_t read_bmp_palette(FILE *file, const bmp_info_t *info, uint8_t *palette_gray, size_t palette_len)
{
    const uint32_t palette_entries = (info->colors_used != 0) ? info->colors_used : 256;
    const long palette_offset = 14L + (long)info->dib_size;

    if (info->bits_per_pixel != 8) {
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(palette_entries <= palette_len, ESP_ERR_INVALID_SIZE, TAG, "BMP palette is too large");
    ESP_RETURN_ON_FALSE(fseek(file, palette_offset, SEEK_SET) == 0, ESP_ERR_INVALID_RESPONSE, TAG, "Seek BMP palette failed");

    for (uint32_t i = 0; i < palette_entries; ++i) {
        uint8_t entry[4];

        ESP_RETURN_ON_FALSE(fread(entry, 1, sizeof(entry), file) == sizeof(entry),
                            ESP_ERR_INVALID_SIZE,
                            TAG,
                            "Read BMP palette failed");
        palette_gray[i] = (uint8_t)(((uint16_t)entry[2] * 30U + (uint16_t)entry[1] * 59U + (uint16_t)entry[0] * 11U) / 100U);
    }

    return ESP_OK;
}

static uint8_t bmp_pixel_to_gray(const bmp_info_t *info, const uint8_t *row, int x, const uint8_t *palette_gray)
{
    if (info->bits_per_pixel == 8) {
        return palette_gray[row[x]];
    }

    const int bytes_per_pixel = info->bits_per_pixel / 8;
    const uint8_t *pixel = &row[x * bytes_per_pixel];
    return (uint8_t)(((uint16_t)pixel[2] * 30U + (uint16_t)pixel[1] * 59U + (uint16_t)pixel[0] * 11U) / 100U);
}

static esp_err_t convert_bmp_to_1bpp(FILE *file, const bmp_info_t *info, const image_service_mono_config_t *mono, uint8_t *bitmap)
{
    const int width = info->width;
    const int height = (info->height < 0) ? -info->height : info->height;
    const bool top_down = info->height < 0;
    const size_t file_row_stride = ((((size_t)width * info->bits_per_pixel) + 31U) / 32U) * 4U;
    uint8_t palette_gray[256] = { 0 };
    uint8_t *row = NULL;
    int16_t *err_curr = NULL;
    int16_t *err_next = NULL;

    ESP_RETURN_ON_ERROR(read_bmp_palette(file, info, palette_gray, sizeof(palette_gray)), TAG, "Read BMP palette failed");

    row = (uint8_t *)malloc(file_row_stride);
    ESP_RETURN_ON_FALSE(row != NULL, ESP_ERR_NO_MEM, TAG, "No memory for BMP row");

    if (mono->dither) {
        err_curr = (int16_t *)calloc((size_t)width + 2U, sizeof(int16_t));
        err_next = (int16_t *)calloc((size_t)width + 2U, sizeof(int16_t));
        if (err_curr == NULL || err_next == NULL) {
            free(row);
            free(err_curr);
            free(err_next);
            return ESP_ERR_NO_MEM;
        }
    }

    for (int y = 0; y < height; ++y) {
        const int file_y = top_down ? y : (height - 1 - y);
        const long row_offset = (long)info->pixel_offset + (long)file_y * (long)file_row_stride;

        if (fseek(file, row_offset, SEEK_SET) != 0 || fread(row, 1, file_row_stride, file) != file_row_stride) {
            free(row);
            free(err_curr);
            free(err_next);
            return ESP_ERR_INVALID_RESPONSE;
        }

        for (int x = 0; x < width; ++x) {
            int value = bmp_pixel_to_gray(info, row, x, palette_gray);

            if (mono->dither) {
                value = clamp_u8(value + (err_curr[x + 1] / 16));
            }

            bool on = value >= mono->threshold;
            const int quantized = on ? 255 : 0;

            if (mono->invert) {
                on = !on;
            }
            set_1bpp_pixel(bitmap, width, x, y, on);

            if (mono->dither) {
                const int error = value - quantized;

                err_curr[x + 2] += (int16_t)(error * 7);
                err_next[x] += (int16_t)(error * 3);
                err_next[x + 1] += (int16_t)(error * 5);
                err_next[x + 2] += (int16_t)error;
            }
        }

        if (mono->dither) {
            int16_t *tmp = err_curr;

            err_curr = err_next;
            err_next = tmp;
            memset(err_next, 0, ((size_t)width + 2U) * sizeof(int16_t));
        }
    }

    free(row);
    free(err_curr);
    free(err_next);
    return ESP_OK;
}

esp_err_t image_service_show_bmp_1bpp(const char *path, const image_service_mono_config_t *config)
{
    const image_service_mono_config_t default_config = {
        .threshold = 128,
        .dither = true,
        .invert = false,
    };
    const image_service_mono_config_t *mono = (config != NULL) ? config : &default_config;
    FILE *file = NULL;
    bmp_info_t info = { 0 };
    uint8_t *bitmap = NULL;
    esp_err_t ret;
    int width;
    int height;
    size_t bitmap_size;

    ESP_RETURN_ON_FALSE(path != NULL, ESP_ERR_INVALID_ARG, TAG, "Image path is null");
    ESP_RETURN_ON_FALSE(storage_service_is_ready(), ESP_ERR_INVALID_STATE, TAG, "Storage service is not ready");

    file = fopen(path, "rb");
    ESP_RETURN_ON_FALSE(file != NULL, ESP_ERR_NOT_FOUND, TAG, "Open BMP failed: %s", path);

    ret = read_bmp_info(file, &info);
    if (ret != ESP_OK) {
        fclose(file);
        return ret;
    }

    width = info.width;
    height = (info.height < 0) ? -info.height : info.height;
    bitmap_size = (((size_t)width + 7U) / 8U) * (size_t)height;
    bitmap = (uint8_t *)calloc(1, bitmap_size);
    if (bitmap == NULL) {
        fclose(file);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG,
             "Load BMP %s: %dx%d %ubpp threshold=%u dither=%d invert=%d",
             path,
             width,
             height,
             info.bits_per_pixel,
             mono->threshold,
             mono->dither,
             mono->invert);

    ret = convert_bmp_to_1bpp(file, &info, mono, bitmap);
    fclose(file);
    if (ret == ESP_OK) {
        ret = display_service_show_bitmap_1bpp(bitmap, width, height, false);
    }

    free(bitmap);
    return ret;
}
