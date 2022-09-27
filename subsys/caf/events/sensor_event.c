/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <caf/events/sensor_event.h>


static const char * const sensor_state_name[] = {
	[SENSOR_STATE_DISABLED] = "DISABLED",
	[SENSOR_STATE_SLEEP] = "SLEEP",
	[SENSOR_STATE_ACTIVE] = "ACTIVE",
	[SENSOR_STATE_ERROR] = "ERROR",
};

static void log_sensor_event(const struct app_event_header *aeh)
{
	const struct sensor_event *event = cast_sensor_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "%s", event->descr);
}

static void profile_sensor_event(struct log_event_buf *buf,
				 const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(sensor_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_event);

APP_EVENT_TYPE_DEFINE(sensor_event,
		  log_sensor_event,
		  &sensor_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_SENSOR_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));


static void log_sensor_state_event(const struct app_event_header *aeh)
{
	const struct sensor_state_event *event = cast_sensor_state_event(aeh);

	BUILD_ASSERT(ARRAY_SIZE(sensor_state_name) == SENSOR_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < SENSOR_STATE_COUNT);
	__ASSERT_NO_MSG(sensor_state_name[event->state] != NULL);

	APP_EVENT_MANAGER_LOG(aeh, "sensor:%s state:%s",
			event->descr, sensor_state_name[event->state]);
}

APP_EVENT_TYPE_DEFINE(sensor_state_event,
		  log_sensor_state_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_SENSOR_STATE_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_set_sensor_period_event(const struct app_event_header *aeh)
{
	const struct set_sensor_period_event *event = cast_set_sensor_period_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "sensor %s sample period changed to: %d",
				 event->descr, event->sampling_period);
}


APP_EVENT_TYPE_DEFINE(set_sensor_period_event,
		  log_set_sensor_period_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
