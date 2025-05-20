/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PRESETS_H_
#define _PRESETS_H_

/**
 * @defgroup auraconfig_presets nRF Auraconfig presets
 * @brief nRF Auraconfig presets
 *
 * @{
 */

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

#define PRESET_NAME_MAX 8
#define LANGUAGE_LEN	3

/* Low latency settings */
static struct bt_bap_lc3_preset lc3_preset_8_1_1 =
	BT_BAP_LC3_BROADCAST_PRESET_8_1_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_8_2_1 =
	BT_BAP_LC3_BROADCAST_PRESET_8_2_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_16_1_1 =
	BT_BAP_LC3_BROADCAST_PRESET_16_1_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_16_2_1 =
	BT_BAP_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_24_1_1 =
	BT_BAP_LC3_BROADCAST_PRESET_24_1_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_24_2_1 =
	BT_BAP_LC3_BROADCAST_PRESET_24_2_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_32_1_1 =
	BT_BAP_LC3_BROADCAST_PRESET_32_1_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_32_2_1 =
	BT_BAP_LC3_BROADCAST_PRESET_32_2_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_1_1 =
	BT_BAP_LC3_BROADCAST_PRESET_48_1_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_2_1 =
	BT_BAP_LC3_BROADCAST_PRESET_48_2_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_3_1 =
	BT_BAP_LC3_BROADCAST_PRESET_48_3_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_4_1 =
	BT_BAP_LC3_BROADCAST_PRESET_48_4_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_5_1 =
	BT_BAP_LC3_BROADCAST_PRESET_48_5_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_6_1 =
	BT_BAP_LC3_BROADCAST_PRESET_48_6_1(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
/* High reliability settings */
static struct bt_bap_lc3_preset lc3_preset_8_1_2 =
	BT_BAP_LC3_BROADCAST_PRESET_8_1_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_8_2_2 =
	BT_BAP_LC3_BROADCAST_PRESET_8_2_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_16_1_2 =
	BT_BAP_LC3_BROADCAST_PRESET_16_1_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_16_2_2 =
	BT_BAP_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_24_1_2 =
	BT_BAP_LC3_BROADCAST_PRESET_24_1_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_24_2_2 =
	BT_BAP_LC3_BROADCAST_PRESET_24_2_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_32_1_2 =
	BT_BAP_LC3_BROADCAST_PRESET_32_1_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_32_2_2 =
	BT_BAP_LC3_BROADCAST_PRESET_32_2_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_1_2 =
	BT_BAP_LC3_BROADCAST_PRESET_48_1_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_2_2 =
	BT_BAP_LC3_BROADCAST_PRESET_48_2_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_3_2 =
	BT_BAP_LC3_BROADCAST_PRESET_48_3_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_4_2 =
	BT_BAP_LC3_BROADCAST_PRESET_48_4_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_5_2 =
	BT_BAP_LC3_BROADCAST_PRESET_48_5_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_48_6_2 =
	BT_BAP_LC3_BROADCAST_PRESET_48_6_2(BT_AUDIO_LOCATION_MONO_AUDIO, BT_AUDIO_CONTEXT_TYPE_ANY);

/** @brief Preset information */
struct bap_preset {
	struct bt_bap_lc3_preset *preset;
	char name[PRESET_NAME_MAX];
};

/**
 * @brief Presets translation struct
 *
 * @note Translation between preset names and preset structures.
 */
static struct bap_preset bap_presets[] = {
	{.preset = &lc3_preset_8_1_1, .name = "8_1_1"},
	{.preset = &lc3_preset_8_2_1, .name = "8_2_1"},
	{.preset = &lc3_preset_16_1_1, .name = "16_1_1"},
	{.preset = &lc3_preset_16_2_1, .name = "16_2_1"},
	{.preset = &lc3_preset_24_1_1, .name = "24_1_1"},
	{.preset = &lc3_preset_24_2_1, .name = "24_2_1"},
	{.preset = &lc3_preset_32_1_1, .name = "32_1_1"},
	{.preset = &lc3_preset_32_2_1, .name = "32_2_1"},
	{.preset = &lc3_preset_48_1_1, .name = "48_1_1"},
	{.preset = &lc3_preset_48_2_1, .name = "48_2_1"},
	{.preset = &lc3_preset_48_3_1, .name = "48_3_1"},
	{.preset = &lc3_preset_48_4_1, .name = "48_4_1"},
	{.preset = &lc3_preset_48_5_1, .name = "48_5_1"},
	{.preset = &lc3_preset_48_6_1, .name = "48_6_1"},
	{.preset = &lc3_preset_8_1_2, .name = "8_1_2"},
	{.preset = &lc3_preset_8_2_2, .name = "8_2_2"},
	{.preset = &lc3_preset_16_1_2, .name = "16_1_2"},
	{.preset = &lc3_preset_16_2_2, .name = "16_2_2"},
	{.preset = &lc3_preset_24_1_2, .name = "24_1_2"},
	{.preset = &lc3_preset_24_2_2, .name = "24_2_2"},
	{.preset = &lc3_preset_32_1_2, .name = "32_1_2"},
	{.preset = &lc3_preset_32_2_2, .name = "32_2_2"},
	{.preset = &lc3_preset_48_1_2, .name = "48_1_2"},
	{.preset = &lc3_preset_48_2_2, .name = "48_2_2"},
	{.preset = &lc3_preset_48_3_2, .name = "48_3_2"},
	{.preset = &lc3_preset_48_4_2, .name = "48_4_2"},
	{.preset = &lc3_preset_48_5_2, .name = "48_5_2"},
	{.preset = &lc3_preset_48_6_2, .name = "48_6_2"},
};

#define LOCATION_NAME_LEN_MAX 4

/** @brief Audio locations */
struct audio_location {
	enum bt_audio_location location;
	char name[LOCATION_NAME_LEN_MAX + 1];
};

/**
 * @brief	Audio location translation struct
 *
 * @note	If there is any change to the specification of audio
 *		locations then this structure must be checked for conformance.
 */
static struct audio_location locations[] = {
	{.location = BT_AUDIO_LOCATION_MONO_AUDIO, .name = "MA"},
	{.location = BT_AUDIO_LOCATION_FRONT_LEFT, .name = "FL"},
	{.location = BT_AUDIO_LOCATION_FRONT_RIGHT, .name = "FR"},
	{.location = BT_AUDIO_LOCATION_FRONT_CENTER, .name = "FC"},
	{.location = BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_1, .name = "LFE1"},
	{.location = BT_AUDIO_LOCATION_BACK_LEFT, .name = "BL"},
	{.location = BT_AUDIO_LOCATION_BACK_RIGHT, .name = "BR"},
	{.location = BT_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER, .name = "FLC"},
	{.location = BT_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER, .name = "FRC"},
	{.location = BT_AUDIO_LOCATION_BACK_CENTER, .name = "BC"},
	{.location = BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_2, .name = "LFE2"},
	{.location = BT_AUDIO_LOCATION_SIDE_LEFT, .name = "SL"},
	{.location = BT_AUDIO_LOCATION_SIDE_RIGHT, .name = "SR"},
	{.location = BT_AUDIO_LOCATION_TOP_FRONT_LEFT, .name = "TFL"},
	{.location = BT_AUDIO_LOCATION_TOP_FRONT_RIGHT, .name = "TFR"},
	{.location = BT_AUDIO_LOCATION_TOP_FRONT_CENTER, .name = "TFC"},
	{.location = BT_AUDIO_LOCATION_TOP_CENTER, .name = "TC"},
	{.location = BT_AUDIO_LOCATION_TOP_BACK_LEFT, .name = "TBL"},
	{.location = BT_AUDIO_LOCATION_TOP_BACK_RIGHT, .name = "TBR"},
	{.location = BT_AUDIO_LOCATION_TOP_SIDE_LEFT, .name = "TSL"},
	{.location = BT_AUDIO_LOCATION_TOP_SIDE_RIGHT, .name = "TSR"},
	{.location = BT_AUDIO_LOCATION_TOP_BACK_CENTER, .name = "TBC"},
	{.location = BT_AUDIO_LOCATION_BOTTOM_FRONT_CENTER, .name = "BFC"},
	{.location = BT_AUDIO_LOCATION_BOTTOM_FRONT_LEFT, .name = "BFL"},
	{.location = BT_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT, .name = "BFR"},
	{.location = BT_AUDIO_LOCATION_FRONT_LEFT_WIDE, .name = "FLW"},
	{.location = BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE, .name = "FRW"},
	{.location = BT_AUDIO_LOCATION_LEFT_SURROUND, .name = "LS"},
	{.location = BT_AUDIO_LOCATION_RIGHT_SURROUND, .name = "RS"},
};

/** @brief Use case types */
enum usecase_type {
	LECTURE,
	SILENT_TV_1,
	SILENT_TV_2,
	MULTI_LANGUAGE,
	PERSONAL_SHARING,
	PERSONAL_MULTI_LANGUAGE,
};

/** @brief Use case information */
struct usecase_info {
	/* Use case type */
	enum usecase_type use_case;
	/* Use case name */
	char name[40];
};

/** @brief Pre-defined use cases */
static struct usecase_info pre_defined_use_cases[] = {
	{.use_case = LECTURE, .name = "Lecture"},
	{.use_case = SILENT_TV_1, .name = "Silent TV 1"},
	{.use_case = SILENT_TV_2, .name = "Silent TV 2"},
	{.use_case = MULTI_LANGUAGE, .name = "Multi-language"},
	{.use_case = PERSONAL_SHARING, .name = "Personal sharing"},
	{.use_case = PERSONAL_MULTI_LANGUAGE, .name = "Personal multi-language"},
};

/** @} */

#endif /* _PRESETS_H_ */
