/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "module_state_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	MODULE_STATE_LIST
#undef X
};

static void log_module_state_event(const struct event_header *eh)
{
	const struct module_state_event *event = cast_module_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == MODULE_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < MODULE_STATE_COUNT);

	EVENT_MANAGER_LOG(eh, "module:%s state:%s",
		      (const char *)event->module_id, state_name[event->state]);
}

EVENT_TYPE_DEFINE(module_state_event,
		  log_module_state_event,
		  NULL,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_BRIDGE_LOG_MODULE_STATE_EVENT,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
