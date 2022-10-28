/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "channel_assignment.h"

#include <errno.h>

#include "uicr.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(channel_assignment, CONFIG_CHAN_ASSIGNMENT_LOG_LEVEL);

static uint8_t channel_value;

void channel_assignment_get(enum audio_channel *channel)
{
	*channel = (enum audio_channel)channel_value;
}

#if CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME
void channel_assignment_set(enum audio_channel channel)
{
	int ret;

	channel_value = channel;

	/* Try to write the channel value to UICR */
	ret = uicr_channel_set(channel);
	if (ret) {
		LOG_DBG("Unable to write channel value to UICR");
	}
}
#endif /* CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME */

void channel_assignment_init(void)
{
#if CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME
	channel_value = uicr_channel_get();

	if (channel_value >= AUDIO_CH_NUM) {
		/* Invalid value in UICR if UICR is not written */
		channel_value = AUDIO_CHANNEL_DEFAULT;
	}
#else
	channel_value = CONFIG_AUDIO_HEADSET_CHANNEL;
#endif /* CONFIG_AUDIO_HEADSET_CHANNEL_RUNTIME */
}
