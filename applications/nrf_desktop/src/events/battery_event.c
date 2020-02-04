/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

	BUILD_ASSERT_MSG(ARRAY_SIZE(state_name) == BATTERY_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < BATTERY_STATE_COUNT);

	return snprintf(buf, buf_len, "battery %s", state_name[event->state]);
}

static void profile_battery_state_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	const struct battery_state_event *event = cast_battery_state_event(eh);

	profiler_log_encode_u32(buf, event->state);
}

EVENT_INFO_DEFINE(battery_state_event,
		  ENCODE(PROFILER_ARG_U32),
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

	return snprintf(buf, buf_len, "level=%u", event->level);
}

static void profile_battery_level_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	const struct battery_level_event *event = cast_battery_level_event(eh);

	profiler_log_encode_u32(buf, event->level);
}

EVENT_INFO_DEFINE(battery_level_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("level"),
		  profile_battery_level_event);


EVENT_TYPE_DEFINE(battery_level_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BATTERY_LEVEL_EVENT),
		  log_battery_level_event,
		  &battery_level_event_info);
