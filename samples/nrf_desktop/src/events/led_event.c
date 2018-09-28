/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <misc/printk.h>

#include "led_event.h"

static const char * const mode_name[] = {
#define X(name) STRINGIFY(name),
	LED_MODE_LIST
#undef X
};

static void print_event(const struct event_header *eh)
{
	struct led_event *event = cast_led_event(eh);

	printk("led_id:%u mode:%s period:%u color: <", event->led_id,
		mode_name[event->mode], event->period);
	for (size_t i = 0; i < sizeof(event->color.c); i++) {
		printk(" %u", event->color.c[i]);
	}
	printk(" >");
}

EVENT_TYPE_DEFINE(led_event, print_event, NULL);
