/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "cloud_module_event.h"
#include "common_module_event.h"

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
	case CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		return "CLOUD_EVT_USER_ASSOCIATION_REQUEST";
	case CLOUD_EVT_USER_ASSOCIATED:
		return "CLOUD_EVT_USER_ASSOCIATED";
	case CLOUD_EVT_CONFIG_RECEIVED:
		return "CLOUD_EVT_CONFIG_RECEIVED";
	case CLOUD_EVT_CONFIG_EMPTY:
		return "CLOUD_EVT_CONFIG_EMPTY";
	case CLOUD_EVT_DATA_ACK:
		return "CLOUD_EVT_DATA_ACK";
	case CLOUD_EVT_SHUTDOWN_READY:
		return "CLOUD_EVT_SHUTDOWN_READY";
	case CLOUD_EVT_FOTA_START:
		return "CLOUD_EVT_FOTA_START";
	case CLOUD_EVT_FOTA_DONE:
		return "CLOUD_EVT_FOTA_DONE";
	case CLOUD_EVT_FOTA_ERROR:
		return "CLOUD_EVT_FOTA_ERROR";
	case CLOUD_EVT_ERROR:
		return "CLOUD_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct app_event_header *aeh)
{
	const struct cloud_module_event *event = cast_cloud_module_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct app_event_header *aeh)
{
	const struct cloud_module_event *event = cast_cloud_module_event(aeh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(cloud_module_event,
			 profile_event);

#endif /* CONFIG_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(cloud_module_event,
			 log_event,
			 &cloud_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_CLOUD_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
