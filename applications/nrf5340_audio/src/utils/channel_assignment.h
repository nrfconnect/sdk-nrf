/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CHANNEL_ASSIGNMENT_H_
#define _CHANNEL_ASSIGNMENT_H_

/** @file
 *  @brief Audio channel assignment
 *
 * Audio channel can be assigned at runtime or compile-time, depending on configuration.
 *
 */

#include <zephyr/bluetooth/audio/audio.h>
#include <audio_defines.h>

#ifndef AUDIO_CHANNEL_DEFAULT
#define AUDIO_CHANNEL_DEFAULT BT_AUDIO_LOCATION_FRONT_LEFT
#endif /* AUDIO_CHANNEL_DEFAULT */

static const char HS_LOC_L_TAG[] = "HL";
static const char HS_LOC_R_TAG[] = "HR";
static const char HS_LOC_LR_TAG[] = "HS";
static const char HS_LOC_OTHER_TAG[] = "H*";
static const char GW_TAG[] = "GW";

/**
 * @brief Get assigned audio channel.
 *
 * @param[out] channel Channel value
 */
void channel_assignment_get(enum bt_audio_location *location);

#if CONFIG_AUDIO_HEADSET_LOCATION_RUNTIME
/**
 * @brief Assign audio channel.
 *
 * @param[out] channel Channel value
 */
void channel_assignment_set(enum bt_audio_location location);
#endif /* AUDIO_HEADSET_LOCATION_RUNTIME */

#endif /* _CHANNEL_ASSIGNMENT_H_ */
