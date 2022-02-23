/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <caf/events/factory_reset_event.h>


static void profile_factory_reset_event(struct log_event_buf *buf,
					const struct event_header *eh)
{
	(void)buf;
	(void)eh;
}

EVENT_INFO_DEFINE(factory_reset_event,
		  ENCODE(),
		  ENCODE(),
		  profile_factory_reset_event);

EVENT_TYPE_DEFINE(factory_reset_event,
		  NULL,
		  &factory_reset_event_info,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_FACTORY_RESET_EVENTS,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
