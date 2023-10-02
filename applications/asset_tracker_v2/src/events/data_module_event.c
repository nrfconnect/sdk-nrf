/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "data_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum data_module_event_type type)
{
	switch (type) {
	case DATA_EVT_DATA_SEND:
		return "DATA_EVT_DATA_SEND";
	case DATA_EVT_DATA_READY:
		return "DATA_EVT_DATA_READY";
	case DATA_EVT_DATA_SEND_BATCH:
		return "DATA_EVT_DATA_SEND_BATCH";
	case DATA_EVT_UI_DATA_READY:
		return "DATA_EVT_UI_DATA_READY";
	case DATA_EVT_UI_DATA_SEND:
		return "DATA_EVT_UI_DATA_SEND";
	case DATA_EVT_IMPACT_DATA_READY:
		return "DATA_EVT_IMPACT_DATA_READY";
	case DATA_EVT_IMPACT_DATA_SEND:
		return "DATA_EVT_IMPACT_DATA_SEND";
	case DATA_EVT_CLOUD_LOCATION_DATA_SEND:
		return "DATA_EVT_CLOUD_LOCATION_DATA_SEND";
	case DATA_EVT_AGNSS_REQUEST_DATA_SEND:
		return "DATA_EVT_AGNSS_REQUEST_DATA_SEND";
	case DATA_EVT_CONFIG_INIT:
		return "DATA_EVT_CONFIG_INIT";
	case DATA_EVT_CONFIG_READY:
		return "DATA_EVT_CONFIG_READY";
	case DATA_EVT_CONFIG_GET:
		return "DATA_EVT_CONFIG_GET";
	case DATA_EVT_CONFIG_SEND:
		return "DATA_EVT_CONFIG_SEND";
	case DATA_EVT_SHUTDOWN_READY:
		return "DATA_EVT_SHUTDOWN_READY";
	case DATA_EVT_DATE_TIME_OBTAINED:
		return "DATA_EVT_DATE_TIME_OBTAINED";
	case DATA_EVT_ERROR:
		return "DATA_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct app_event_header *aeh)
{
	const struct data_module_event *event = cast_data_module_event(aeh);

	if (event->type == DATA_EVT_ERROR) {
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
	const struct data_module_event *event = cast_data_module_event(aeh);

#if defined(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING)
	nrf_profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	nrf_profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(data_module_event,
			 profile_event);

#endif /* CONFIG_NRF_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(data_module_event,
			 log_event,
			 &data_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_DATA_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
