/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "gnss_module_event.h"

static char *get_evt_type_str(enum gnss_module_event_type type)
{
	switch (type) {
	case GNSS_EVT_DATA_READY:
		return "GNSS_EVT_DATA_READY";
	case GNSS_EVT_TIMEOUT:
		return "GNSS_EVT_TIMEOUT";
	case GNSS_EVT_ACTIVE:
		return "GNSS_EVT_ACTIVE";
	case GNSS_EVT_INACTIVE:
		return "GNSS_EVT_INACTIVE";
	case GNSS_EVT_SHUTDOWN_READY:
		return "GNSS_EVT_SHUTDOWN_READY";
	case GNSS_EVT_AGPS_NEEDED:
		return "GNSS_EVT_AGPS_NEEDED";
	case GNSS_EVT_ERROR_CODE:
		return "GNSS_EVT_ERROR_CODE";
	default:
		return "Unknown event";
	}
}

static int log_event(const struct event_header *eh, char *buf,
		     size_t buf_len)
{
	const struct gnss_module_event *event = cast_gnss_module_event(eh);

	if (event->type == GNSS_EVT_ERROR_CODE) {
		EVENT_MANAGER_LOG(eh, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	} else {
		EVENT_MANAGER_LOG(eh, "%s", get_evt_type_str(event->type));
	}
	return 0;
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct gnss_module_event *event = cast_gnss_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	profiler_log_encode_uint8(buf, event->type);
#endif
}

EVENT_INFO_DEFINE(gnss_module_event,
#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
		  ENCODE(PROFILER_ARG_STRING),
#else
		  ENCODE(PROFILER_ARG_U8),
#endif
		  ENCODE("type"),
		  profile_event);

#endif /* CONFIG_PROFILER */

EVENT_TYPE_DEFINE(gnss_module_event,
		  CONFIG_GNSS_EVENTS_LOG,
		  log_event,
#if defined(CONFIG_PROFILER)
		  &gnss_module_event_info);
#else
		  NULL);
#endif
