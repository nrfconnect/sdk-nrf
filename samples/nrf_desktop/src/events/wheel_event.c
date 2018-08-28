/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "wheel_event.h"

static void print_event(const struct event_header *eh)
{
	struct wheel_event *event = cast_wheel_event(eh);

	printk("wheel=%d", event->wheel);
}

EVENT_TYPE_DEFINE(wheel_event, print_event, NULL);
