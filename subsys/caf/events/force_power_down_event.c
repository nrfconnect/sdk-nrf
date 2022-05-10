/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/force_power_down_event.h>

#include <zephyr/kernel.h>


static void profile_simple_event(struct log_event_buf *buf,
				 const struct app_event_header *aeh)
{
	(void)buf;
	(void)aeh;
}

APP_EVENT_INFO_DEFINE(force_power_down_event,
		  ENCODE(),
		  ENCODE(),
		  profile_simple_event);

APP_EVENT_TYPE_DEFINE(force_power_down_event,
		  NULL,
		  &force_power_down_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_FORCE_POWER_DOWN_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
