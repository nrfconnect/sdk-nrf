/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "gnss_module_event.h"
#include "common_module_event.h"

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

static void log_event(const struct app_event_header *aeh)
{
	const struct gnss_module_event *event = cast_gnss_module_event(aeh);

	if (event->type == GNSS_EVT_ERROR_CODE) {
		APP_EVENT_MANAGER_LOG(aeh, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	} else {
		APP_EVENT_MANAGER_LOG(aeh, "%s", get_evt_type_str(event->type));
	}
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct app_event_header *aeh)
{
	const struct gnss_module_event *event = cast_gnss_module_event(aeh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(gnss_module_event,
			 profile_event);

#endif /* CONFIG_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(gnss_module_event,
			 log_event,
			 &gnss_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_GNSS_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
