/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>

#define MODULE buttons_pm
#include <caf/events/button_event.h>
#include <caf/events/keep_alive_event.h>

static bool app_event_handler(const struct app_event_header *aeh)
{
	__ASSERT_NO_MSG(is_button_event(aeh));
	const struct button_event *event = cast_button_event(aeh);

	if (event->pressed) {
		keep_alive();
	}
	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
