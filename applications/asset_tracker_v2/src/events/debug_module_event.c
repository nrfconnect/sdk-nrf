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
	case DEBUG_EVT_EMULATOR_INITIALIZED:
		return "DEBUG_EVT_EMULATOR_INITIALIZED";
	case DEBUG_EVT_EMULATOR_NETWORK_CONNECTED:
		return "DEBUG_EVT_EMULATOR_NETWORK_CONNECTED";
	case DEBUG_EVT_ERROR:
		return "DEBUG_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct app_event_header *aeh)
{
	const struct debug_module_event *event = cast_debug_module_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_NRF_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct app_event_header *aeh)
{
	const struct debug_module_event *event = cast_debug_module_event(aeh);

#if defined(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING)
	nrf_profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	nrf_profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(debug_module_event,
			 profile_event);

#endif /* CONFIG_NRF_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(debug_module_event,
			 log_event,
			 &debug_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_DEBUG_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
