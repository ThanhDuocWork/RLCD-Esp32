#include "board_features.h"

static const board_features_t s_board_features = {
    .audio_adc_es7210 = { .name = "ES7210 ADC", .description = "ADC audio chip with echo-cancellation microphone front-end", .present = true },
    .audio_codec_es8311 = { .name = "ES8311 Codec", .description = "Low-power audio codec IC", .present = true },
    .boot_button = { .name = "BOOT Button", .description = "Hold during power-on to enter download mode", .present = true },
    .power_button = { .name = "PWR Button", .description = "Long press to power off, short press to power on", .present = true },
    .key_button = { .name = "KEY Button", .description = "User-definable function button", .present = true },
    .shtc3_sensor = { .name = "SHTC3", .description = "Ambient temperature and humidity sensor", .present = true },
    .rtc_pcf85063 = { .name = "PCF85063 RTC", .description = "RTC chip with time-keeping support", .present = true },
    .speaker_header = { .name = "MX1.25 2-pin Speaker", .description = "External speaker output header", .present = true },
    .rtc_battery_header = { .name = "RTC Power Header", .description = "PH1.0 rechargeable battery connector for RTC backup", .present = true },
    .dual_header_2x8 = { .name = "2x8 Female Header", .description = "Two 8-pin 2.54 mm female headers", .present = true },
    .battery_holder_18650 = { .name = "18650 Battery Holder", .description = "Single-cell 18650 battery holder", .present = true },
    .dual_microphone_array = { .name = "Dual Microphones", .description = "Dual microphone array used with ES7210", .present = true },
    .charge_indicator = { .name = "CHG LED", .description = "Charging indicator LED, turns off when full", .present = true },
    .warning_indicator = { .name = "WRN LED", .description = "Warning LED for reverse battery connection", .present = true },
    .usb_type_c = { .name = "USB Type-C", .description = "Used for flashing firmware and serial logs", .present = true },
    .tf_card_slot = { .name = "TF Card Slot", .description = "FAT32 microSD expansion slot", .present = true },
};

const board_features_t *board_features_get(void)
{
    return &s_board_features;
}
