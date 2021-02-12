/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "app_module_event.h"

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
	case APP_DATA_GNSS:
		return "GNSS";
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

static int log_event(const struct event_header *eh, char *buf,
		     size_t buf_len)
{
	const struct app_module_event *event = cast_app_module_event(eh);
	char event_name[50] = "\0";
	char data_types[50] = "\0";

	strcpy(event_name, get_evt_type_str(event->type));

	if (event->type == APP_EVT_ERROR) {
		return snprintf(buf, buf_len, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	} else if (event->type == APP_EVT_DATA_GET) {
		for (int i = 0; i < event->count; i++) {
			strcat(data_types, type2str(event->data_list[i]));

			if (i == event->count - 1) {
				break;
			}

			strcat(data_types, ", ");
		}

		return snprintf(buf, buf_len, "%s - Requested data types (%s)",
				event_name, data_types);
	}

	return snprintf(buf, buf_len, "%s", event_name);
}

static void profile_event(struct log_event_buf *buf,
			  const struct event_header *eh)
{
	const struct app_module_event *event = cast_app_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type),
		strlen(get_evt_type_str(event->type)));
#else
	profiler_log_encode_u32(buf, event->type);
#endif
}

EVENT_INFO_DEFINE(app_module_event,
#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
		  ENCODE(PROFILER_ARG_STRING),
#else
		  ENCODE(PROFILER_ARG_U32),
#endif
		  ENCODE("type"),
		  profile_event);

EVENT_TYPE_DEFINE(app_module_event,
		  CONFIG_APP_EVENTS_LOG,
		  log_event,
#if defined(CONFIG_PROFILER)
		  &app_module_event_info);
#else
		  NULL);
#endif
