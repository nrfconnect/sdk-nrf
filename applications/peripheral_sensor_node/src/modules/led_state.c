/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <caf/events/led_event.h>
#include <caf/events/power_event.h>

#define MODULE led_state
#include <caf/events/module_state_event.h>

#include "led_state_def.h"

static bool handle_wake_up_event(const struct wake_up_event *evt)
{
	struct led_event *event = new_led_event();

	event->led_id = LED_ID_1;
	event->led_effect = &led_effect_on;
	APP_EVENT_SUBMIT(event);

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_wake_up_event(aeh)) {
		return handle_wake_up_event(cast_wake_up_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
