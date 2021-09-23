/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>

#define MODULE buttons_pm
#include <caf/events/button_event.h>
#include <caf/events/keep_alive_event.h>

static bool event_handler(const struct event_header *eh)
{
	__ASSERT_NO_MSG(is_button_event(eh));
	const struct button_event *event = cast_button_event(eh);

	if (event->pressed) {
		keep_alive();
	}
	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, button_event);
