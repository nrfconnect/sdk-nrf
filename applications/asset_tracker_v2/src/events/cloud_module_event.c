/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "cloud_module_event.h"

static char *get_evt_type_str(enum cloud_module_event_type type)
{
	switch (type) {
	case CLOUD_EVT_CONNECTED:
		return "CLOUD_EVT_CONNECTED";
	case CLOUD_EVT_DISCONNECTED:
		return "CLOUD_EVT_DISCONNECTED";
	case CLOUD_EVT_CONNECTING:
		return "CLOUD_EVT_CONNECTING";
	case CLOUD_EVT_CONNECTION_TIMEOUT:
		return "CLOUD_EVT_CONNECTION_TIMEOUT";
	case CLOUD_EVT_CONFIG_RECEIVED:
		return "CLOUD_EVT_CONFIG_RECEIVED";
	case CLOUD_EVT_CONFIG_EMPTY:
		return "CLOUD_EVT_CONFIG_EMPTY";
	case CLOUD_EVT_DATA_ACK:
		return "CLOUD_EVT_DATA_ACK";
	case CLOUD_EVT_SHUTDOWN_READY:
		return "CLOUD_EVT_SHUTDOWN_READY";
	case CLOUD_EVT_FOTA_DONE:
		return "CLOUD_EVT_FOTA_DONE";
	case CLOUD_EVT_ERROR:
		return "CLOUD_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static int log_event(const struct event_header *eh, char *buf,
		     size_t buf_len)
{
	const struct cloud_module_event *event = cast_cloud_module_event(eh);

	return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct cloud_module_event *event = cast_cloud_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type),
		strlen(get_evt_type_str(event->type)));
#else
	profiler_log_encode_u32(buf, event->type);
#endif
}

EVENT_INFO_DEFINE(cloud_module_event,
#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
		  ENCODE(PROFILER_ARG_STRING),
#else
		  ENCODE(PROFILER_ARG_U32),
#endif
		  ENCODE("type"),
		  profile_event);

#endif /* CONFIG_PROFILER */

EVENT_TYPE_DEFINE(cloud_module_event,
		  CONFIG_CLOUD_EVENTS_LOG,
		  log_event,
#if defined(CONFIG_PROFILER)
		  &cloud_module_event_info);
#else
		  NULL);
#endif
