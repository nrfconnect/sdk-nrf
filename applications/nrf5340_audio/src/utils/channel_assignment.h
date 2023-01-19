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

#include <audio_defines.h>

#ifndef AUDIO_CHANNEL_DEFAULT
#define AUDIO_CHANNEL_DEFAULT AUDIO_CH_L
#endif /* AUDIO_CHANNEL_DEFAULT */

static const char CH_L_TAG[] = "HL";
static const char CH_R_TAG[] = "HR";
static const char GW_TAG[] = "GW";

/**
 * @brief Get assigned audio channel.
 *
 * @param[out] channel Channel value
 */
void channel_assignment_get(enum audio_channel *channel);

#if CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME
/**
 * @brief Assign audio channel.
 *
 * @param[out] channel Channel value
 */
void channel_assignment_set(enum audio_channel channel);
#endif /* AUDIO_HEADSET_CHANNEL_RUNTIME */

/**
 * @brief Initialize the channel assignment
 */
void channel_assignment_init(void);

#endif /* _CHANNEL_ASSIGNMENT_H_ */
