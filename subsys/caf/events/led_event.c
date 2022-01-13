/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <caf/events/led_event.h>

static int log_led_event(const struct event_header *eh, char *buf,
			  size_t buf_len)
{
	const struct led_event *event = cast_led_event(eh);

	EVENT_MANAGER_LOG(eh, "led_id:%u effect:%p",
			event->led_id, event->led_effect);
	return 0;
}

EVENT_TYPE_DEFINE(led_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_LED_EVENTS),
		  log_led_event,
		  NULL);

static int log_led_ready_event(const struct event_header *eh, char *buf,
			 size_t buf_len)
{
	const struct led_ready_event *event = cast_led_ready_event(eh);

	EVENT_MANAGER_LOG(eh, "led_id:%u effect:%p",
			event->led_id, event->led_effect);
	return 0;
}

EVENT_TYPE_DEFINE(led_ready_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_LED_READY_EVENTS),
		  log_led_ready_event,
		  NULL);
