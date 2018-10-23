/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <misc/util.h>

#include "battery_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	BATTERY_STATE_LIST
#undef X
};


static void print_battery_state_event(const struct event_header *eh)
{
	struct battery_state_event *event = cast_battery_state_event(eh);

	static_assert(ARRAY_SIZE(state_name) == BATTERY_STATE_COUNT,
		      "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < BATTERY_STATE_COUNT);

	printk("battery %s", state_name[event->state]);
}

static void log_battery_state_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	struct battery_state_event *event = cast_battery_state_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->state);
}

EVENT_INFO_DEFINE(battery_state_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("state"),
		  log_battery_state_event);


EVENT_TYPE_DEFINE(battery_state_event, print_battery_state_event,
		  &battery_state_event_info);


static void print_battery_level_event(const struct event_header *eh)
{
	struct battery_level_event *event = cast_battery_level_event(eh);

	printk("level=%u", event->level);
}

static void log_battery_level_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	struct battery_level_event *event = cast_battery_level_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->level);
}

EVENT_INFO_DEFINE(battery_level_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("level"),
		  log_battery_level_event);


EVENT_TYPE_DEFINE(battery_level_event, print_battery_level_event,
		  &battery_level_event_info);
