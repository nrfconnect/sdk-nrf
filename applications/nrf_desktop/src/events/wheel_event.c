/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "wheel_event.h"

static void log_wheel_event(const struct event_header *eh)
{
	const struct wheel_event *event = cast_wheel_event(eh);

	EVENT_MANAGER_LOG(eh, "wheel=%d", event->wheel);
}

EVENT_TYPE_DEFINE(wheel_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_WHEEL_EVENT),
		  log_wheel_event,
		  NULL);
