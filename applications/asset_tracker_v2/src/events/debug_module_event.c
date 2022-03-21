/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "debug_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum debug_module_event_type type)
{
	switch (type) {
	case DEBUG_EVT_MEMFAULT_DATA_READY:
		return "DEBUG_EVT_MEMFAULT_DATA_READY";
	case DEBUG_EVT_QEMU_X86_INITIALIZED:
		return "DEBUG_EVT_QEMU_X86_INITIALIZED";
	case DEBUG_EVT_QEMU_X86_NETWORK_CONNECTED:
		return "DEBUG_EVT_QEMU_X86_NETWORK_CONNECTED";
	case DEBUG_EVT_ERROR:
		return "DEBUG_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct event_header *eh)
{
	const struct debug_module_event *event = cast_debug_module_event(eh);

	EVENT_MANAGER_LOG(eh, "%s", get_evt_type_str(event->type));
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

COMMON_EVENT_INFO_DEFINE(debug_module_event,
			 profile_event);

#endif /* CONFIG_PROFILER */

COMMON_EVENT_TYPE_DEFINE(debug_module_event,
			 CONFIG_DEBUG_EVENTS_LOG,
			 log_event,
			 &debug_module_event_info);
