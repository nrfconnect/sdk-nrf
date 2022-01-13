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

static int log_sensor_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct sensor_event *event = cast_sensor_event(eh);

	EVENT_MANAGER_LOG(eh, "%s", event->descr);
	return 0;
}

static void profile_sensor_event(struct log_event_buf *buf, const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(sensor_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_event);

EVENT_TYPE_DEFINE(sensor_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_SENSOR_EVENTS),
		  log_sensor_event,
		  &sensor_event_info);


static int log_sensor_state_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct sensor_state_event *event = cast_sensor_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(sensor_state_name) == SENSOR_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < SENSOR_STATE_COUNT);
	__ASSERT_NO_MSG(sensor_state_name[event->state] != NULL);

	EVENT_MANAGER_LOG(eh, "sensor:%s state:%s",
			event->descr, sensor_state_name[event->state]);
	return 0;
}

EVENT_TYPE_DEFINE(sensor_state_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_SENSOR_STATE_EVENTS),
		  log_sensor_state_event,
		  NULL);
