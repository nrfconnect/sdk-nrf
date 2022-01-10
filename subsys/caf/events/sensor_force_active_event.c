/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <caf/events/sensor_force_active_event.h>
#include <event_manager_profiler_tracer.h>


static int log_sensor_force_active_event(
	const struct event_header *eh,
	char *buf, size_t buf_len)
{
	const struct sensor_force_active_event *event =
		cast_sensor_force_active_event(eh);

	return snprintf(buf, buf_len,
			"sensor:%s, force_active:%s",
			event->descr,
			event->force_active ? "yes" : "no");
}

static void profile_sensor_force_active_event(struct log_event_buf *buf,
					      const struct event_header *eh)
{
	const struct sensor_force_active_event *event =
		cast_sensor_force_active_event(eh);

	profiler_log_encode_string(buf, event->descr);
	profiler_log_encode_uint8(buf, event->force_active);
}

EVENT_INFO_DEFINE(sensor_force_active_event,
		  ENCODE(PROFILER_ARG_STRING, PROFILER_ARG_U8),
		  ENCODE("sensor", "force_active"),
		  profile_sensor_force_active_event);

EVENT_TYPE_DEFINE(sensor_force_active_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_SENSOR_FORCE_ACTIVE_EVENTS),
		  log_sensor_force_active_event,
		  &sensor_force_active_event_info);
