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

#ifndef AUDIO_CHANNEL_DEFAULT
#define AUDIO_CHANNEL_DEFAULT AUDIO_CH_L
#endif /* AUDIO_CHANNEL_DEFAULT */

static const char CH_L_TAG[] = "HL";
static const char CH_R_TAG[] = "HR";
static const char GW_TAG[] = "GW";

/**@brief Audio channel assignment values */
enum audio_channel {
	AUDIO_CH_L,
	AUDIO_CH_R,
	AUDIO_CH_NUM,
};

/**
 * @brief Get assigned audio channel.
 *
 * @param[out] channel Channel value
 *
 * @return 0 if successful
 * @return -EIO if channel is not assigned.
 */
int channel_assignment_get(enum audio_channel *channel);

#if CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME
/**
 * @brief Assign audio channel.
 *
 * @param[out] channel Channel value
 *
 * @return 0 if successful
 * @return -EROFS if different channel is already written
 * @return -EIO if channel is not assigned.
 */
int channel_assignment_set(enum audio_channel channel);
#endif /* AUDIO_HEADSET_CHANNEL_RUNTIME */

#endif /* _CHANNEL_ASSIGNMENT_H_ */
