/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <misc/util.h>

#include "module_state_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	MODULE_STATE_LIST
#undef X
};

static void print_event(const struct event_header *eh)
{
	struct module_state_event *event = cast_module_state_event(eh);

	static_assert(ARRAY_SIZE(state_name) == MODULE_STATE_COUNT,
		      "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < MODULE_STATE_COUNT);

	printk("module:%s state:%s", (const char *)event->module_id,
			state_name[event->state]);
}

EVENT_TYPE_DEFINE(module_state_event, print_event, NULL);
