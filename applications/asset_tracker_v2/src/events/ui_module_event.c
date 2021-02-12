/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ui_module_event.h"

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

static int log_event(const struct event_header *eh, char *buf,
		     size_t buf_len)
{
	const struct ui_module_event *event = cast_ui_module_event(eh);

	if (event->type == UI_EVT_ERROR) {
		return snprintf(buf, buf_len, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	}

	return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct ui_module_event *event = cast_ui_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type),
		strlen(get_evt_type_str(event->type)));
#else
	profiler_log_encode_u32(buf, event->type);
#endif
}

EVENT_INFO_DEFINE(ui_module_event,
#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
		  ENCODE(PROFILER_ARG_STRING),
#else
		  ENCODE(PROFILER_ARG_U32),
#endif
		  ENCODE("type"),
		  profile_event);

#endif /* CONFIG_PROFILER */

EVENT_TYPE_DEFINE(ui_module_event,
		  CONFIG_UI_EVENTS_LOG,
		  log_event,
#if defined(CONFIG_PROFILER)
		  &ui_module_event_info);
#else
		  NULL);
#endif
