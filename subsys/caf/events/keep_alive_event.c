/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/keep_alive_event.h>

#include <zephyr.h>


static void profile_simple_result_event(struct log_event_buf *buf,
					const struct event_header *eh)
{
	(void)buf;
	(void)eh;
}

EVENT_INFO_DEFINE(keep_alive_event,
		  ENCODE(),
		  ENCODE(),
		  profile_simple_result_event);

EVENT_TYPE_DEFINE(keep_alive_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_KEEP_ALIVE_EVENTS),
		  NULL,
		  &keep_alive_event_info);
