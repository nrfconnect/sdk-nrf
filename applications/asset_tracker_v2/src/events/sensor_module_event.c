/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "sensor_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum sensor_module_event_type type)
{
	switch (type) {
	case SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED:
		return "SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED";
	case SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED:
		return "SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED";
	case SENSOR_EVT_MOVEMENT_IMPACT_DETECTED:
		return "SENSOR_EVT_MOVEMENT_IMPACT_DETECTED";
	case SENSOR_EVT_ENVIRONMENTAL_DATA_READY:
		return "SENSOR_EVT_ENVIRONMENTAL_DATA_READY";
	case SENSOR_EVT_FUEL_GAUGE_READY:
		return "SENSOR_EVT_FUEL_GAUGE_READY";
	case SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED:
		return "SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED";
	case SENSOR_EVT_SHUTDOWN_READY:
		return "SENSOR_EVT_SHUTDOWN_READY";
	case SENSOR_EVT_ERROR:
		return "SENSOR_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct app_event_header *aeh)
{
	const struct sensor_module_event *event = cast_sensor_module_event(aeh);

	if (event->type == SENSOR_EVT_ERROR) {
		APP_EVENT_MANAGER_LOG(aeh, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	} else {
		APP_EVENT_MANAGER_LOG(aeh, "%s", get_evt_type_str(event->type));
	}
}

#if defined(CONFIG_NRF_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct app_event_header *aeh)
{
	const struct sensor_module_event *event = cast_sensor_module_event(aeh);

#if defined(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING)
	nrf_profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	nrf_profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(sensor_module_event,
			 profile_event);

#endif /* CONFIG_NRF_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(sensor_module_event,
			 log_event,
			 &sensor_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_SENSOR_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
