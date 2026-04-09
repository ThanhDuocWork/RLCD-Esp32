#pragma once

#include <stdbool.h>

typedef struct {
    const char *name;
    const char *description;
    bool present;
} board_feature_entry_t;

typedef struct {
    board_feature_entry_t audio_adc_es7210;
    board_feature_entry_t audio_codec_es8311;
    board_feature_entry_t boot_button;
    board_feature_entry_t power_button;
    board_feature_entry_t key_button;
    board_feature_entry_t shtc3_sensor;
    board_feature_entry_t rtc_pcf85063;
    board_feature_entry_t speaker_header;
    board_feature_entry_t rtc_battery_header;
    board_feature_entry_t dual_header_2x8;
    board_feature_entry_t battery_holder_18650;
    board_feature_entry_t dual_microphone_array;
    board_feature_entry_t charge_indicator;
    board_feature_entry_t warning_indicator;
    board_feature_entry_t usb_type_c;
    board_feature_entry_t tf_card_slot;
} board_features_t;

const board_features_t *board_features_get(void);
