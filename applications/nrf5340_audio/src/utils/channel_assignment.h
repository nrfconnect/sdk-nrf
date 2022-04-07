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
#define AUDIO_CHANNEL_DEFAULT AUDIO_CHANNEL_LEFT
#endif /* AUDIO_CHANNEL_DEFAULT */

/**@brief Audio channel assignment values */
enum audio_channel {
	AUDIO_CHANNEL_LEFT = 0,
	AUDIO_CHANNEL_RIGHT,

	AUDIO_CHANNEL_COUNT
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
