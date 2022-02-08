/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ui_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum ui_module_event_type type)
{
	switch (type) {
	case UI_EVT_BUTTON_DATA_READY:
		return "UI_EVT_BUTTON_DATA_READY";
	case UI_EVT_SHUTDOWN_READY:
		return "UI_EVT_SHUTDOWN_READY";
	case UI_EVT_ERROR:
		return "UI_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct event_header *eh)
{
	const struct ui_module_event *event = cast_ui_module_event(eh);

	if (event->type == UI_EVT_ERROR) {
		EVENT_MANAGER_LOG(eh, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	} else {
		EVENT_MANAGER_LOG(eh, "%s", get_evt_type_str(event->type));
	}
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct ui_module_event *event = cast_ui_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_EVENT_INFO_DEFINE(ui_module_event,
			 profile_event);

#endif /* CONFIG_PROFILER */

COMMON_EVENT_TYPE_DEFINE(ui_module_event,
			 CONFIG_UI_EVENTS_LOG,
			 log_event,
			 &ui_module_event_info);
