/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "wheel_event.h"

static int log_wheel_event(const struct event_header *eh, char *buf,
			size_t buf_len)
{
	struct wheel_event *event = cast_wheel_event(eh);

	return snprintf(buf, buf_len, "wheel=%d", event->wheel);
}

EVENT_TYPE_DEFINE(wheel_event, log_wheel_event, NULL);
