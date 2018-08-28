/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "module_state_event.h"

const void *MODULE_STATE_READY = "ready";
const void *MODULE_STATE_OFF = "off";
const void *MODULE_STATE_STANDBY = "standby";
const void *MODULE_STATE_ERROR = "error";

static void print_event(const struct event_header *eh)
{
	struct module_state_event *event = cast_module_state_event(eh);

	printk("module:%s state:%s", (const char *)event->module_id,
			(const char *)event->state);
}

EVENT_TYPE_DEFINE(module_state_event, print_event, NULL);
