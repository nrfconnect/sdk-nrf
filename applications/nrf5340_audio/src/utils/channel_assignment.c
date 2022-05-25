/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "channel_assignment.h"

#include <errno.h>

#include "uicr.h"

int channel_assignment_get(enum audio_channel *channel)
{
#if CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME
	uint8_t channel_value;

	channel_value = uicr_channel_get();

	if (channel_value >= AUDIO_CHANNEL_COUNT) {
		return -EIO;
	}

	*channel = (enum audio_channel)channel_value;
#else
	*channel = (enum audio_channel)CONFIG_AUDIO_HEADSET_CHANNEL;
#endif /* CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME */

	return 0;
}

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
int channel_assignment_set(enum audio_channel channel)
{
	return uicr_channel_set(channel);
}

#endif /* CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME */
