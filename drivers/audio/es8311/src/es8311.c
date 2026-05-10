#include "es8311.h"

#include "esp_bit_defs.h"
#include "esp_check.h"
#include "esp_log.h"
#include "i2c_master_driver.h"

static const char *TAG = "es8311";

#define ES8311_I2C_TIMEOUT_MS 100

#define ES8311_REG00_RESET 0x00
#define ES8311_REG01_CLK_MANAGER 0x01
#define ES8311_REG02_CLK_MANAGER 0x02
#define ES8311_REG03_CLK_MANAGER 0x03
#define ES8311_REG04_CLK_MANAGER 0x04
#define ES8311_REG05_CLK_MANAGER 0x05
#define ES8311_REG06_CLK_MANAGER 0x06
#define ES8311_REG07_CLK_MANAGER 0x07
#define ES8311_REG08_CLK_MANAGER 0x08
#define ES8311_REG09_SDPIN 0x09
#define ES8311_REG0A_SDPOUT 0x0A
#define ES8311_REG0D_SYSTEM 0x0D
#define ES8311_REG0E_SYSTEM 0x0E
#define ES8311_REG12_SYSTEM 0x12
#define ES8311_REG13_SYSTEM 0x13
#define ES8311_REG14_SYSTEM 0x14
#define ES8311_REG16_ADC 0x16
#define ES8311_REG17_ADC 0x17
#define ES8311_REG1C_ADC 0x1C
#define ES8311_REG31_DAC 0x31
#define ES8311_REG32_DAC 0x32
#define ES8311_REG37_DAC 0x37

typedef struct {
    uint32_t mclk_hz;
    uint32_t sample_rate_hz;
    uint8_t pre_div;
    uint8_t pre_mult;
    uint8_t adc_div;
    uint8_t dac_div;
    uint8_t fs_mode;
    uint8_t lrck_h;
    uint8_t lrck_l;
    uint8_t bclk_div;
    uint8_t adc_osr;
    uint8_t dac_osr;
} es8311_clock_coeff_t;

/*
 * Fixed playback coefficient for 16 kHz with 4.096 MHz MCLK (256 * fs).
 * Values aligned with common ES8311 open-source driver tables.
 */
static const es8311_clock_coeff_t s_coeff_16k_4m096 = {
    .mclk_hz = 4096000,
    .sample_rate_hz = 16000,
    .pre_div = 0x01,
    .pre_mult = 0x00,
    .adc_div = 0x01,
    .dac_div = 0x01,
    .fs_mode = 0x00,
    .lrck_h = 0x00,
    .lrck_l = 0xFF,
    .bclk_div = 0x04,
    .adc_osr = 0x10,
    .dac_osr = 0x10,
};

static i2c_master_dev_handle_t s_i2c_handle;
static bool s_ready;

static esp_err_t es8311_write_reg(uint8_t reg, uint8_t value)
{
    ESP_RETURN_ON_FALSE(s_i2c_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "I2C handle is not ready");
    return i2c_master_driver_write_reg8(s_i2c_handle, reg, value, ES8311_I2C_TIMEOUT_MS);
}

esp_err_t es8311_read_reg(uint8_t reg, uint8_t *value)
{
    ESP_RETURN_ON_FALSE(s_i2c_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "I2C handle is not ready");
    return i2c_master_driver_read_reg8(s_i2c_handle, reg, value, ES8311_I2C_TIMEOUT_MS);
}

static uint8_t es8311_bits_to_reg_value(uint8_t bits_per_sample)
{
    switch (bits_per_sample) {
    case 16:
        return (3U << 2);
    case 18:
        return (2U << 2);
    case 20:
        return (1U << 2);
    case 24:
        return (0U << 2);
    case 32:
        return (4U << 2);
    default:
        return (3U << 2);
    }
}

static esp_err_t es8311_configure_clock(const es8311_config_t *config)
{
    const es8311_clock_coeff_t *coeff = &s_coeff_16k_4m096;
    uint8_t reg_val = 0;

    ESP_RETURN_ON_FALSE(config->sample_rate_hz == coeff->sample_rate_hz && config->mclk_hz == coeff->mclk_hz,
                        ESP_ERR_NOT_SUPPORTED,
                        TAG,
                        "Only 16kHz/4.096MHz clock profile is supported in current driver");

    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG01_CLK_MANAGER, 0x3F), TAG, "Write clk reg01 failed");

    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_REG02_CLK_MANAGER, &reg_val), TAG, "Read clk reg02 failed");
    reg_val &= 0x07;
    reg_val |= (uint8_t)((coeff->pre_div - 1U) << 5);
    reg_val |= (uint8_t)(coeff->pre_mult << 3);
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG02_CLK_MANAGER, reg_val), TAG, "Write clk reg02 failed");

    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG03_CLK_MANAGER, (uint8_t)((coeff->fs_mode << 6) | coeff->adc_osr)),
                        TAG,
                        "Write clk reg03 failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG04_CLK_MANAGER, coeff->dac_osr), TAG, "Write clk reg04 failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG05_CLK_MANAGER,
                                         (uint8_t)(((coeff->adc_div - 1U) << 4) | (coeff->dac_div - 1U))),
                        TAG,
                        "Write clk reg05 failed");

    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_REG06_CLK_MANAGER, &reg_val), TAG, "Read clk reg06 failed");
    reg_val &= 0xE0;
    reg_val |= (uint8_t)(coeff->bclk_div - 1U);
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG06_CLK_MANAGER, reg_val), TAG, "Write clk reg06 failed");

    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_REG07_CLK_MANAGER, &reg_val), TAG, "Read clk reg07 failed");
    reg_val &= 0xC0;
    reg_val |= coeff->lrck_h;
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG07_CLK_MANAGER, reg_val), TAG, "Write clk reg07 failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG08_CLK_MANAGER, coeff->lrck_l), TAG, "Write clk reg08 failed");
    return ESP_OK;
}

static esp_err_t es8311_configure_format(const es8311_config_t *config)
{
    uint8_t reg00 = 0;
    const uint8_t fmt = es8311_bits_to_reg_value(config->bits_per_sample);

    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_REG00_RESET, &reg00), TAG, "Read reset reg failed");
    reg00 &= 0xBF;
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG00_RESET, reg00), TAG, "Write reset reg failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG09_SDPIN, fmt), TAG, "Write sdpin failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG0A_SDPOUT, fmt), TAG, "Write sdpout failed");
    return ESP_OK;
}

static esp_err_t es8311_configure_playback_path(const es8311_config_t *config)
{
    (void)config;

    /*
     * This project is in playback-first mode:
     * - keep the DAC/output path enabled
     * - keep the ADC/mic path quiet to avoid bringing up unused blocks
     * - leave EQ/processing bypassed for the first hardware validation stage
     */
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG14_SYSTEM, 0x10), TAG, "Route playback path failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG16_ADC, 0x00), TAG, "Disable ADC scale failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG17_ADC, 0x00), TAG, "Disable ADC gain failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG1C_ADC, 0x6A), TAG, "ADC path bypass failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG37_DAC, 0x08), TAG, "DAC path bypass failed");
    return ESP_OK;
}

esp_err_t es8311_init(const es8311_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL && config->i2c_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid es8311 config");

    s_i2c_handle = config->i2c_handle;

    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG00_RESET, 0x1F), TAG, "Reset stage 1 failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG00_RESET, 0x00), TAG, "Reset stage 2 failed");
    ESP_RETURN_ON_ERROR(es8311_configure_clock(config), TAG, "Configure clock failed");
    ESP_RETURN_ON_ERROR(es8311_configure_format(config), TAG, "Configure format failed");
    ESP_RETURN_ON_ERROR(es8311_configure_playback_path(config), TAG, "Configure playback path failed");
    ESP_RETURN_ON_ERROR(es8311_set_volume(config->default_volume), TAG, "Set default volume failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG0D_SYSTEM, 0x01), TAG, "Power analog failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG0E_SYSTEM, 0x02), TAG, "Enable PGA failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG12_SYSTEM, 0x00), TAG, "Power DAC failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG13_SYSTEM, 0x10), TAG, "Enable output failed");
    ESP_RETURN_ON_ERROR(es8311_set_mute(true), TAG, "Mute during bring-up failed");
    ESP_RETURN_ON_ERROR(es8311_write_reg(ES8311_REG00_RESET, 0x80), TAG, "Power on failed");

    s_ready = true;
    ESP_LOGI(TAG,
             "ES8311 ready: sample=%u mclk=%u bits=%u volume=0x%02x",
             (unsigned)config->sample_rate_hz,
             (unsigned)config->mclk_hz,
             config->bits_per_sample,
             config->default_volume);
    return ESP_OK;
}

esp_err_t es8311_set_volume(uint8_t volume)
{
    ESP_RETURN_ON_FALSE(s_i2c_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "ES8311 is not ready");
    return es8311_write_reg(ES8311_REG32_DAC, volume);
}

esp_err_t es8311_set_mute(bool mute)
{
    uint8_t reg31 = 0;

    ESP_RETURN_ON_FALSE(s_i2c_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "ES8311 is not ready");
    ESP_RETURN_ON_ERROR(es8311_read_reg(ES8311_REG31_DAC, &reg31), TAG, "Read mute reg failed");

    if (mute) {
        reg31 |= BIT(6) | BIT(5);
    } else {
        reg31 &= (uint8_t)~(BIT(6) | BIT(5));
    }

    return es8311_write_reg(ES8311_REG31_DAC, reg31);
}
