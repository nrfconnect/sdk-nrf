/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "gps_module_event.h"

static char *get_evt_type_str(enum gps_module_event_type type)
{
	switch (type) {
	case GPS_EVT_DATA_READY:
		return "GPS_EVT_DATA_READY";
	case GPS_EVT_TIMEOUT:
		return "GPS_EVT_TIMEOUT";
	case GPS_EVT_ACTIVE:
		return "GPS_EVT_ACTIVE";
	case GPS_EVT_INACTIVE:
		return "GPS_EVT_INACTIVE";
	case GPS_EVT_SHUTDOWN_READY:
		return "GPS_EVT_SHUTDOWN_READY";
	case GPS_EVT_AGPS_NEEDED:
		return "GPS_EVT_AGPS_NEEDED";
	case GPS_EVT_ERROR_CODE:
		return "GPS_EVT_ERROR_CODE";
	default:
		return "Unknown event";
	}
}

static int log_event(const struct event_header *eh, char *buf,
		     size_t buf_len)
{
	const struct gps_module_event *event = cast_gps_module_event(eh);

	if (event->type == GPS_EVT_ERROR_CODE) {
		return snprintf(buf, buf_len, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	}

	return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct gps_module_event *event = cast_gps_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type),
		strlen(get_evt_type_str(event->type)));
#else
	profiler_log_encode_u32(buf, event->type);
#endif
}

EVENT_INFO_DEFINE(gps_module_event,
#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
		  ENCODE(PROFILER_ARG_STRING),
#else
		  ENCODE(PROFILER_ARG_U32),
#endif
		  ENCODE("type"),
		  profile_event);

#endif /* CONFIG_PROFILER */

EVENT_TYPE_DEFINE(gps_module_event,
		  CONFIG_GPS_EVENTS_LOG,
		  log_event,
#if defined(CONFIG_PROFILER)
		  &gps_module_event_info);
#else
		  NULL);
#endif
