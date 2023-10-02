/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "location_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum location_module_event_type type)
{
	switch (type) {
	case LOCATION_MODULE_EVT_GNSS_DATA_READY:
		return "LOCATION_MODULE_EVT_GNSS_DATA_READY";
	case LOCATION_MODULE_EVT_DATA_NOT_READY:
		return "LOCATION_MODULE_EVT_DATA_NOT_READY";
	case LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY:
		return "LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY";
	case LOCATION_MODULE_EVT_TIMEOUT:
		return "LOCATION_MODULE_EVT_TIMEOUT";
	case LOCATION_MODULE_EVT_ACTIVE:
		return "LOCATION_MODULE_EVT_ACTIVE";
	case LOCATION_MODULE_EVT_INACTIVE:
		return "LOCATION_MODULE_EVT_INACTIVE";
	case LOCATION_MODULE_EVT_SHUTDOWN_READY:
		return "LOCATION_MODULE_EVT_SHUTDOWN_READY";
	case LOCATION_MODULE_EVT_AGNSS_NEEDED:
		return "LOCATION_MODULE_EVT_AGNSS_NEEDED";
	case LOCATION_MODULE_EVT_PGPS_NEEDED:
		return "LOCATION_MODULE_EVT_PGPS_NEEDED";
	case LOCATION_MODULE_EVT_ERROR_CODE:
		return "LOCATION_MODULE_EVT_ERROR_CODE";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct app_event_header *aeh)
{
	const struct location_module_event *event = cast_location_module_event(aeh);

	if (event->type == LOCATION_MODULE_EVT_ERROR_CODE) {
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
	const struct location_module_event *event = cast_location_module_event(aeh);

#if defined(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING)
	nrf_profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	nrf_profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(location_module_event,
			 profile_event);

#endif /* CONFIG_NRF_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(location_module_event,
			 log_event,
			 &location_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_LOCATION_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
