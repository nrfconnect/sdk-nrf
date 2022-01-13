/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <assert.h>
#include <stdio.h>

#include "battery_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	BATTERY_STATE_LIST
#undef X
};


static int log_battery_state_event(const struct event_header *eh, char *buf,
			  size_t buf_len)
{
	const struct battery_state_event *event = cast_battery_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == BATTERY_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < BATTERY_STATE_COUNT);

	EVENT_MANAGER_LOG(eh, "battery %s", state_name[event->state]);
	return 0;
}

static void profile_battery_state_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	const struct battery_state_event *event = cast_battery_state_event(eh);

	profiler_log_encode_uint8(buf, event->state);
}

EVENT_INFO_DEFINE(battery_state_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("state"),
		  profile_battery_state_event);


EVENT_TYPE_DEFINE(battery_state_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BATTERY_STATE_EVENT),
		  log_battery_state_event,
		  &battery_state_event_info);


static int log_battery_level_event(const struct event_header *eh, char *buf,
			  size_t buf_len)
{
	const struct battery_level_event *event = cast_battery_level_event(eh);

	EVENT_MANAGER_LOG(eh, "level=%u", event->level);
	return 0;
}

static void profile_battery_level_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	const struct battery_level_event *event = cast_battery_level_event(eh);

	profiler_log_encode_uint8(buf, event->level);
}

EVENT_INFO_DEFINE(battery_level_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("level"),
		  profile_battery_level_event);


EVENT_TYPE_DEFINE(battery_level_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BATTERY_LEVEL_EVENT),
		  log_battery_level_event,
		  &battery_level_event_info);
