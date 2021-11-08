/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <sys/util.h>
#include <stdio.h>

#include "pelion_event.h"


static const char * const state_name[PELION_STATE_COUNT] = {
	[PELION_STATE_DISABLED] = "PELION_STATE_DISABLED",
	[PELION_STATE_INITIALIZED] = "PELION_STATE_INITIALIZED",
	[PELION_STATE_REGISTERED] = "PELION_STATE_REGISTERED",
	[PELION_STATE_UNREGISTERED] = "PELION_STATE_UNREGISTERED",
	[PELION_STATE_SUSPENDED] = "PELION_STATE_SUSPENDED"
};

static int log_pelion_state_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct pelion_state_event *event = cast_pelion_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == PELION_STATE_COUNT,
		     "Invalid number of elements");

	__ASSERT_NO_MSG(event);
	__ASSERT_NO_MSG(event->state < PELION_STATE_COUNT);

	return snprintf(buf, buf_len, "%s", state_name[event->state]);
}

static void profile_pelion_state_event(struct log_event_buf *buf,
				       const struct event_header *eh)
{
	const struct pelion_state_event *event = cast_pelion_state_event(eh);

	__ASSERT_NO_MSG(event);

	profiler_log_encode_uint8(buf, event->state);
}

EVENT_INFO_DEFINE(pelion_state_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("state"),
		  profile_pelion_state_event);

EVENT_TYPE_DEFINE(pelion_state_event,
		  IS_ENABLED(CONFIG_PELION_INIT_LOG_PELION_STATE_EVENTS),
		  log_pelion_state_event,
		  &pelion_state_event_info);

EVENT_TYPE_DEFINE(pelion_create_objects_event,
		  IS_ENABLED(CONFIG_PELION_INIT_LOG_PELION_CREATE_OBJECTS_EVENTS),
		  NULL,
		  NULL);
