#include "storage_port.h"

#include "bsp_storage.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

static const char *TAG = "storage_port";

static sdmmc_card_t *s_card;
static bool s_mounted;

esp_err_t storage_port_mount(void)
{
    const bsp_storage_sdcard_config_t *cfg = bsp_storage_get_sdcard_config();

    ESP_RETURN_ON_FALSE(cfg != NULL, ESP_ERR_INVALID_STATE, TAG, "No SD card BSP config");

    if (s_mounted) {
        return ESP_OK;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = cfg->format_if_mount_failed,
        .max_files = cfg->max_files,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 1;
    slot_config.clk = cfg->clk_gpio;
    slot_config.cmd = cfg->cmd_gpio;
    slot_config.d0 = cfg->d0_gpio;
    if (cfg->use_internal_pullups) {
        slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    }

    ESP_LOGI(TAG,
             "Mount TF card: mount=%s clk=%d cmd=%d d0=%d",
             cfg->mount_point,
             cfg->clk_gpio,
             cfg->cmd_gpio,
             cfg->d0_gpio);

    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdmmc_mount(cfg->mount_point, &host, &slot_config, &mount_config, &s_card),
                        TAG,
                        "Mount TF card failed");

    s_mounted = true;
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

esp_err_t storage_port_unmount(void)
{
    const bsp_storage_sdcard_config_t *cfg = bsp_storage_get_sdcard_config();

    if (!s_mounted) {
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(cfg != NULL, ESP_ERR_INVALID_STATE, TAG, "No SD card BSP config");
    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdcard_unmount(cfg->mount_point, s_card), TAG, "Unmount TF card failed");
    s_card = NULL;
    s_mounted = false;
    return ESP_OK;
}

bool storage_port_is_mounted(void)
{
    return s_mounted;
}

const char *storage_port_get_mount_point(void)
{
    const bsp_storage_sdcard_config_t *cfg = bsp_storage_get_sdcard_config();

    return (cfg != NULL) ? cfg->mount_point : "";
}
