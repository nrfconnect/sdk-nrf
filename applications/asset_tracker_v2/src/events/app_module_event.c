/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "app_module_event.h"
#include "common_module_event.h"

static char *type2str(enum app_module_data_type type)
{
	switch (type) {
	case APP_DATA_ENVIRONMENTAL:
		return "ENV";
	case APP_DATA_MOVEMENT:
		return "MOVE";
	case APP_DATA_MODEM_STATIC:
		return "MOD_STAT";
	case APP_DATA_MODEM_DYNAMIC:
		return "MOD_DYN";
	case APP_DATA_BATTERY:
		return "BAT";
	case APP_DATA_LOCATION:
		return "LOCATION";
	default:
		return "Unknown type";
	}
}

static char *get_evt_type_str(enum app_module_event_type type)
{
	switch (type) {
	case APP_EVT_DATA_GET:
		return "APP_EVT_DATA_GET";
	case APP_EVT_CONFIG_GET:
		return "APP_EVT_CONFIG_GET";
	case APP_EVT_DATA_GET_ALL:
		return "APP_EVT_DATA_GET_ALL";
	case APP_EVT_START:
		return "APP_EVT_START";
	case APP_EVT_LTE_CONNECT:
		return "APP_EVT_LTE_CONNECT";
	case APP_EVT_LTE_DISCONNECT:
		return "APP_EVT_LTE_DISCONNECT";
	case APP_EVT_SHUTDOWN_READY:
		return "APP_EVT_SHUTDOWN_READY";
	case APP_EVT_ERROR:
		return "APP_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct app_event_header *aeh)
{
	const struct app_module_event *event = cast_app_module_event(aeh);
	char data_types[60] = "\0";

	if (event->type == APP_EVT_ERROR) {
		APP_EVENT_MANAGER_LOG(aeh, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	} else if (event->type == APP_EVT_DATA_GET) {
		for (int i = 0; i < event->count; i++) {
			/* data_types should contain space for the stringified type and a null
			 * character.
			 */
			size_t max_append_len = sizeof(data_types) - strlen(data_types) - 1;

			if (i < event->count - 1) {
				/* For every type except the last one, we also need space
				 * for a comma and a space character.
				 */
				max_append_len -= 2;
			}

			if (strlen(type2str(event->data_list[i])) > max_append_len) {
				return;
			}

			strcat(data_types, type2str(event->data_list[i]));

			if (i == event->count - 1) {
				break;
			}

			strcat(data_types, ", ");
		}

		APP_EVENT_MANAGER_LOG(aeh, "%s - Requested data types (%s)",
				get_evt_type_str(event->type), data_types);
	} else {
		APP_EVENT_MANAGER_LOG(aeh, "%s", get_evt_type_str(event->type));
	}
}

#if defined(CONFIG_NRF_PROFILER)

static void profile_event(struct log_event_buf *buf,
			  const struct app_event_header *aeh)
{
	const struct app_module_event *event = cast_app_module_event(aeh);

#if defined(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING)
	nrf_profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	nrf_profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(app_module_event,
			 profile_event);

#endif /* CONFIG_NRF_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(app_module_event,
			 log_event,
			 &app_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_APP_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
