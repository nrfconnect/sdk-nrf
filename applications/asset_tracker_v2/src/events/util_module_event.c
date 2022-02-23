/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "util_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum util_module_event_type type)
{
	switch (type) {
	case UTIL_EVT_SHUTDOWN_REQUEST:
		return "UTIL_EVT_SHUTDOWN_REQUEST";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct event_header *eh)
{
	const struct util_module_event *event = cast_util_module_event(eh);

	EVENT_MANAGER_LOG(eh, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct util_module_event *event = cast_util_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_EVENT_INFO_DEFINE(util_module_event,
			 profile_event);

#endif /* CONFIG_PROFILER */

COMMON_EVENT_TYPE_DEFINE(util_module_event,
			 log_event,
			 &util_module_event_info,
			 EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_UTIL_EVENTS_LOG,
					(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
