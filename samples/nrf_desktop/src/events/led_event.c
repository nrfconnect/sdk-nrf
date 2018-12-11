/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "led_event.h"

static const char * const mode_name[] = {
#define X(name) STRINGIFY(name),
	LED_MODE_LIST
#undef X
};

static int log_led_event(const struct event_header *eh, char *buf,
			  size_t buf_len)
{
	struct led_event *event = cast_led_event(eh);
	int temp;
	int pos = 0;

	temp = snprintf(buf, buf_len, "led_id:%u mode:%s period:%u color: <",
			event->led_id, mode_name[event->mode], event->period);
	if (temp < 0) {
		return temp;
	}
	pos += temp;
	if (pos >= buf_len) {
		return pos;
	}

	for (size_t i = 0; i < sizeof(event->color.c); i++) {
		temp = snprintf(buf + pos, buf_len - pos, " %u",
				event->color.c[i]);
		if (temp < 0) {
			return temp;
		}
		pos += temp;
		if (pos >= buf_len) {
			return pos;
		}
	}
	temp = snprintf(buf + pos, buf_len - pos, " >");
	if (temp < 0) {
		return temp;
	}
	pos += temp;

	return pos;
}

EVENT_TYPE_DEFINE(led_event, log_led_event, NULL);
