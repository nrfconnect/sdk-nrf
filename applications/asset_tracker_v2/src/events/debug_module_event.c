/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "debug_module_event.h"

static char *get_evt_type_str(enum debug_module_event_type type)
{
	switch (type) {
	case DEBUG_EVT_MEMFAULT_DATA_READY:
		return "DEBUG_EVT_MEMFAULT_DATA_READY";
	case DEBUG_EVT_ERROR:
		return "DEBUG_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static int log_event(const struct event_header *eh, char *buf,
		     size_t buf_len)
{
	const struct debug_module_event *event = cast_debug_module_event(eh);

	return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct debug_module_event *event = cast_debug_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	profiler_log_encode_uint8(buf, event->type);
#endif
}

EVENT_INFO_DEFINE(debug_module_event,
#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
		  ENCODE(PROFILER_ARG_STRING),
#else
		  ENCODE(PROFILER_ARG_U8),
#endif
		  ENCODE("type"),
		  profile_event);

#endif /* CONFIG_PROFILER */

EVENT_TYPE_DEFINE(debug_module_event,
		  CONFIG_DEBUG_EVENTS_LOG,
		  log_event,
#if defined(CONFIG_PROFILER)
		  &debug_module_event_info);
#else
		  NULL);
#endif
