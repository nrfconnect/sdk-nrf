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


static void log_battery_state_event(const struct app_event_header *aeh)
{
	const struct battery_state_event *event = cast_battery_state_event(aeh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == BATTERY_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < BATTERY_STATE_COUNT);

	APP_EVENT_MANAGER_LOG(aeh, "battery %s", state_name[event->state]);
}

static void profile_battery_state_event(struct log_event_buf *buf,
				    const struct app_event_header *aeh)
{
	const struct battery_state_event *event = cast_battery_state_event(aeh);

	nrf_profiler_log_encode_uint8(buf, event->state);
}

APP_EVENT_INFO_DEFINE(battery_state_event,
		  ENCODE(NRF_PROFILER_ARG_U8),
		  ENCODE("state"),
		  profile_battery_state_event);

APP_EVENT_TYPE_DEFINE(battery_state_event,
		  log_battery_state_event,
		  &battery_state_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_BATTERY_STATE_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));


static void log_battery_level_event(const struct app_event_header *aeh)
{
	const struct battery_level_event *event = cast_battery_level_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "level=%u", event->level);
}

static void profile_battery_level_event(struct log_event_buf *buf,
				    const struct app_event_header *aeh)
{
	const struct battery_level_event *event = cast_battery_level_event(aeh);

	nrf_profiler_log_encode_uint8(buf, event->level);
}

APP_EVENT_INFO_DEFINE(battery_level_event,
		  ENCODE(NRF_PROFILER_ARG_U8),
		  ENCODE("level"),
		  profile_battery_level_event);

APP_EVENT_TYPE_DEFINE(battery_level_event,
		  log_battery_level_event,
		  &battery_level_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_BATTERY_LEVEL_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
