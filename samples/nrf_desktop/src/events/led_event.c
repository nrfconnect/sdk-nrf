/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "led_event.h"


static int log_led_event(const struct event_header *eh, char *buf,
			  size_t buf_len)
{
	struct led_event *event = cast_led_event(eh);
	int temp;
	int pos = 0;

	temp = snprintf(buf, buf_len, "led_id:%u effect:%p",
			event->led_id, event->led_effect);
	if (temp < 0) {
		return temp;
	}
	pos += temp;

	return pos;
}

EVENT_TYPE_DEFINE(led_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_LED_EVENT),
		  log_led_event,
		  NULL);
