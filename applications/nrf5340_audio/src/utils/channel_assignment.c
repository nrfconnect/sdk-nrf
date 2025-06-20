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

static uint32_t location_value;

void channel_assignment_get(enum bt_audio_location *location)
{
	*location = (enum bt_audio_location)location_value;
}

#if CONFIG_AUDIO_HEADSET_LOCATION_RUNTIME
void channel_assignment_set(enum bt_audio_location location)
{
	int ret;

	location_value = location;

	/* Try to write the channel value to UICR */
	ret = uicr_channel_set(location);
	if (ret) {
		LOG_ERR("Unable to write channel value to UICR");
	}
}
#endif /* CONFIG_AUDIO_HEADSET_LOCATION_RUNTIME */

static int channel_assignment_init(void)
{
#if CONFIG_AUDIO_HEADSET_LOCATION_RUNTIME
	location_value = uicr_channel_get();

	if (location_value >= BT_AUDIO_LOCATION_RIGHT_SURROUND) {
		/* Invalid value in UICR if UICR is not written */
		location_value = AUDIO_CHANNEL_DEFAULT;
	}
#else
	location_value = CONFIG_AUDIO_HEADSET_LOCATION;
#endif /* CONFIG_AUDIO_HEADSET_LOCATION_RUNTIME */

	LOG_WRN("Location value is 0x%x", location_value);
	return 0;
}

SYS_INIT(channel_assignment_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
