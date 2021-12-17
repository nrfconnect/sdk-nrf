/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <sys/util.h>

#include <caf/events/module_state_event.h>


static const char * const state_name[] = {
	[MODULE_STATE_READY] = "READY",
	[MODULE_STATE_OFF] = "OFF",
	[MODULE_STATE_STANDBY] = "STANDBY",
	[MODULE_STATE_ERROR] = "ERROR",
};

static int log_module_state_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct module_state_event *event = cast_module_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == MODULE_STATE_COUNT,
		     "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < MODULE_STATE_COUNT);
	__ASSERT_NO_MSG(state_name[event->state] != NULL);

	return snprintf(buf, buf_len, "module:%s state:%s",
			module_name_get(event->module_id), state_name[event->state]);
}

EVENT_TYPE_DEFINE(module_state_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_MODULE_STATE_EVENTS),
		  log_module_state_event,
		  NULL);
