/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_sensor_event.h"

static void log_sensor_start_event(const struct app_event_header *aeh)
{
	const struct sensor_start_event *event = cast_sensor_start_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "Start %s sensor, delay:%u, period:%u",
				event->descr, event->delay, event->period);
}

APP_EVENT_TYPE_DEFINE(sensor_start_event,
		  log_sensor_start_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));


static void log_sensor_stop_event(const struct app_event_header *aeh)
{
	const struct sensor_stop_event *event = cast_sensor_stop_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "Stop %s sensor", event->descr);
}

APP_EVENT_TYPE_DEFINE(sensor_stop_event,
		  log_sensor_stop_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
