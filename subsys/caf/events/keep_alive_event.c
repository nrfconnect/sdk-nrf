/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/keep_alive_event.h>

#include <zephyr.h>


static void profile_simple_result_event(struct log_event_buf *buf,
					const struct application_event_header *aeh)
{
	(void)buf;
	(void)aeh;
}

EVENT_INFO_DEFINE(keep_alive_event,
		  ENCODE(),
		  ENCODE(),
		  profile_simple_result_event);

APPLICATION_EVENT_TYPE_DEFINE(keep_alive_event,
		  NULL,
		  &keep_alive_event_info,
		  APPLICATION_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_KEEP_ALIVE_EVENTS,
				(APPLICATION_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
