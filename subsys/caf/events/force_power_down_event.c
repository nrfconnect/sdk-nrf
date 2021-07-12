/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/force_power_down_event.h>

#include <zephyr.h>


static void profile_simple_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	(void)buf;
	(void)eh;
}

EVENT_INFO_DEFINE(force_power_down_event,
		  ENCODE(),
		  ENCODE(),
		  profile_simple_event);

EVENT_TYPE_DEFINE(force_power_down_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_FORCE_POWER_DOWN_EVENTS),
		  NULL,
		  &force_power_down_event_info);
