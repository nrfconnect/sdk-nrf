/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "module_state_event.h"

static void print_event(const struct event_header *eh)
{
	struct module_state_event *event = cast_module_state_event(eh);

	printk("module:%s state:%s", event->name, event->state);
}

EVENT_TYPE_DEFINE(module_state_event, print_event);
