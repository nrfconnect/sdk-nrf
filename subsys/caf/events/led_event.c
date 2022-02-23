/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <caf/events/led_event.h>

static void log_led_event(const struct event_header *eh)
{
	const struct led_event *event = cast_led_event(eh);

	EVENT_MANAGER_LOG(eh, "led_id:%u effect:%p",
			event->led_id, event->led_effect);
}

EVENT_TYPE_DEFINE(led_event,
		  log_led_event,
		  NULL,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_LED_EVENTS,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_led_ready_event(const struct event_header *eh)
{
	const struct led_ready_event *event = cast_led_ready_event(eh);

	EVENT_MANAGER_LOG(eh, "led_id:%u effect:%p",
			event->led_id, event->led_effect);
}

EVENT_TYPE_DEFINE(led_ready_event,
		  log_led_ready_event,
		  NULL,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_LED_READY_EVENTS,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
