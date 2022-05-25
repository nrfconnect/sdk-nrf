/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>

#include <caf/events/sensor_data_aggregator_event.h>

static void log_sensor_data_aggregator_event(const struct app_event_header *aeh)
{
	const struct sensor_data_aggregator_event *event = cast_sensor_data_aggregator_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh,
			      "Send sensor buffer desc address: %p",
			      (void *)event->sensor_descr);
}

static void profile_sensor_data_aggregator_event(struct log_event_buf *buf,
					    const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(sensor_data_aggregator_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_data_aggregator_event);

APP_EVENT_TYPE_DEFINE(sensor_data_aggregator_event,
		  log_sensor_data_aggregator_event,
		  &sensor_data_aggregator_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));


APP_EVENT_TYPE_DEFINE(sensor_data_aggregator_release_buffer_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
