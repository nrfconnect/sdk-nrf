/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BROADCAST_CONFIG_TOOL_H_
#define _BROADCAST_CONFIG_TOOL_H_

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

#define PRESET_NAME_MAX 8
#define LANGUAGE_LEN	3

/* The location and context type will be set by further down */
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

struct bap_preset {
	struct bt_bap_lc3_preset *preset;
	char name[PRESET_NAME_MAX];
};

#define CONTEXT_NAME_LEN_MAX 20
struct audio_context {
	enum bt_audio_context context;
	char name[CONTEXT_NAME_LEN_MAX];
};

#define LOCATION_NAME_LEN_MAX 5
struct audio_location {
	enum bt_audio_location location;
	char name[5];
};

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

static struct audio_context contexts[] = {
	{.context = BT_AUDIO_CONTEXT_TYPE_PROHIBITED, .name = "Prohibited"},
	{.context = BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED, .name = "Unspecified"},
	{.context = BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL, .name = "Conversational"},
	{.context = BT_AUDIO_CONTEXT_TYPE_MEDIA, .name = "Media"},
	{.context = BT_AUDIO_CONTEXT_TYPE_GAME, .name = "Game"},
	{.context = BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL, .name = "Instructional"},
	{.context = BT_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS, .name = "Voice assistants"},
	{.context = BT_AUDIO_CONTEXT_TYPE_LIVE, .name = "Live"},
	{.context = BT_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS, .name = "Sound effects"},
	{.context = BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS, .name = "Notifications"},
	{.context = BT_AUDIO_CONTEXT_TYPE_RINGTONE, .name = "Ringtone"},
	{.context = BT_AUDIO_CONTEXT_TYPE_ALERTS, .name = "Alerts"},
	{.context = BT_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM, .name = "Emergency alarm"},
};

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

enum usecase_type {
	LECTURE,
	SILENT_TV_1,
	SILENT_TV_2,
	MULTI_LANGUAGE,
	PERSONAL_SHARING,
	PERSONAL_MULTI_LANGUAGE,
};

struct usecase_info {
	enum usecase_type use_case;
	char name[40];
};

static struct usecase_info pre_defined_use_cases[] = {
	{.use_case = LECTURE, .name = "Lecture"},
	{.use_case = SILENT_TV_1, .name = "Silent TV 1"},
	{.use_case = SILENT_TV_2, .name = "Silent TV 2"},
	{.use_case = MULTI_LANGUAGE, .name = "Multi-language"},
	{.use_case = PERSONAL_SHARING, .name = "Personal sharing"},
	{.use_case = PERSONAL_MULTI_LANGUAGE, .name = "Personal multi-language"},
};

static inline char *location_bit_to_str(enum bt_audio_location location)
{
	switch (location) {
	case BT_AUDIO_LOCATION_MONO_AUDIO:
		return "Mono Audio";
	case BT_AUDIO_LOCATION_FRONT_LEFT:
		return "Front Left";
	case BT_AUDIO_LOCATION_FRONT_RIGHT:
		return "Front Right";
	case BT_AUDIO_LOCATION_FRONT_CENTER:
		return "Front Center";
	case BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_1:
		return "Low Frequency 1";
	case BT_AUDIO_LOCATION_BACK_LEFT:
		return "Back Left";
	case BT_AUDIO_LOCATION_BACK_RIGHT:
		return "Back Right";
	case BT_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER:
		return "Front Left of Center";
	case BT_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER:
		return "Front Right of Center";
	case BT_AUDIO_LOCATION_BACK_CENTER:
		return "Back Center";
	case BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_2:
		return "Low Frequency 2";
	case BT_AUDIO_LOCATION_SIDE_LEFT:
		return "Side Left";
	case BT_AUDIO_LOCATION_SIDE_RIGHT:
		return "Side Right";
	case BT_AUDIO_LOCATION_TOP_FRONT_LEFT:
		return "Top Front Left";
	case BT_AUDIO_LOCATION_TOP_FRONT_RIGHT:
		return "Top Front Right";
	case BT_AUDIO_LOCATION_TOP_FRONT_CENTER:
		return "Top Front Center";
	case BT_AUDIO_LOCATION_TOP_CENTER:
		return "Top Center";
	case BT_AUDIO_LOCATION_TOP_BACK_LEFT:
		return "Top Back Left";
	case BT_AUDIO_LOCATION_TOP_BACK_RIGHT:
		return "Top Back Right";
	case BT_AUDIO_LOCATION_TOP_SIDE_LEFT:
		return "Top Side Left";
	case BT_AUDIO_LOCATION_TOP_SIDE_RIGHT:
		return "Top Side Right";
	case BT_AUDIO_LOCATION_TOP_BACK_CENTER:
		return "Top Back Center";
	case BT_AUDIO_LOCATION_BOTTOM_FRONT_CENTER:
		return "Bottom Front Center";
	case BT_AUDIO_LOCATION_BOTTOM_FRONT_LEFT:
		return "Bottom Front Left";
	case BT_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT:
		return "Bottom Front Right";
	case BT_AUDIO_LOCATION_FRONT_LEFT_WIDE:
		return "Front Left Wide";
	case BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE:
		return "Front Right Wide";
	case BT_AUDIO_LOCATION_LEFT_SURROUND:
		return "Left Surround";
	case BT_AUDIO_LOCATION_RIGHT_SURROUND:
		return "Right Surround";
	default:
		return "Unknown";
	}
}

static inline char *context_bit_to_str(enum bt_audio_context context)
{
	switch (context) {
	case BT_AUDIO_CONTEXT_TYPE_PROHIBITED:
		return "Prohibited";
	case BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED:
		return "Unspecified";
	case BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL:
		return "Conversational";
	case BT_AUDIO_CONTEXT_TYPE_MEDIA:
		return "Media";
	case BT_AUDIO_CONTEXT_TYPE_GAME:
		return "Game";
	case BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL:
		return "Instructional";
	case BT_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS:
		return "Voice assistant";
	case BT_AUDIO_CONTEXT_TYPE_LIVE:
		return "Live";
	case BT_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS:
		return "Sound effects";
	case BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS:
		return "Notifications";
	case BT_AUDIO_CONTEXT_TYPE_RINGTONE:
		return "Ringtone";
	case BT_AUDIO_CONTEXT_TYPE_ALERTS:
		return "Alerts";
	case BT_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM:
		return "Emergency alarm";
	default:
		return "Unknown";
	}
}

#endif /* _BROADCAST_CONFIG_TOOL_H_ */
