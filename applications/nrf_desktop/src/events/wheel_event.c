/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "wheel_event.h"

static int log_wheel_event(const struct event_header *eh, char *buf,
			   size_t buf_len)
{
	const struct wheel_event *event = cast_wheel_event(eh);

	return snprintf(buf, buf_len, "wheel=%d", event->wheel);
}

EVENT_TYPE_DEFINE(wheel_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_WHEEL_EVENT),
		  log_wheel_event,
		  NULL);
