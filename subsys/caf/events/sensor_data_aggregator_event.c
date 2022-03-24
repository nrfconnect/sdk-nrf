/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <event_manager.h>

#include <caf/events/sensor_data_aggregator_event.h>

static void log_sensor_data_aggregator_event(const struct event_header *eh)
{
	const struct sensor_data_aggregator_event *event = cast_sensor_data_aggregator_event(eh);

	EVENT_MANAGER_LOG(eh, "Send sensor buffer desc: %s", event->sensor_descr);
}

static void profile_sensor_data_aggregator_event(struct log_event_buf *buf,
					    const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(sensor_data_aggregator_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_data_aggregator_event);

EVENT_TYPE_DEFINE(sensor_data_aggregator_event,
		  true,
		  log_sensor_data_aggregator_event,
		  &sensor_data_aggregator_event_info);


EVENT_TYPE_DEFINE(sensor_data_aggregator_release_buffer_event,
		  true,
		  NULL,
		  NULL);
