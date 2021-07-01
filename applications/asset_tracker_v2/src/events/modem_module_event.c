/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "modem_module_event.h"

static char *get_evt_type_str(enum modem_module_event_type type)
{
	switch (type) {
	case MODEM_EVT_LTE_CONNECTED:
		return "MODEM_EVT_LTE_CONNECTED";
	case MODEM_EVT_LTE_DISCONNECTED:
		return "MODEM_EVT_LTE_DISCONNECTED";
	case MODEM_EVT_LTE_CONNECTING:
		return "MODEM_EVT_LTE_CONNECTING";
	case MODEM_EVT_LTE_CELL_UPDATE:
		return "MODEM_EVT_LTE_CELL_UPDATE";
	case MODEM_EVT_LTE_PSM_UPDATE:
		return "MODEM_EVT_LTE_PSM_UPDATE";
	case MODEM_EVT_LTE_EDRX_UPDATE:
		return "MODEM_EVT_LTE_EDRX_UPDATE";
	case MODEM_EVT_MODEM_STATIC_DATA_READY:
		return "MODEM_EVT_MODEM_STATIC_DATA_READY";
	case MODEM_EVT_MODEM_DYNAMIC_DATA_READY:
		return "MODEM_EVT_MODEM_DYNAMIC_DATA_READY";
	case MODEM_EVT_MODEM_STATIC_DATA_NOT_READY:
		return "MODEM_EVT_MODEM_STATIC_DATA_NOT_READY";
	case MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY:
		return "MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY";
	case MODEM_EVT_BATTERY_DATA_NOT_READY:
		return "MODEM_EVT_BATTERY_DATA_NOT_READY";
	case MODEM_EVT_BATTERY_DATA_READY:
		return "MODEM_EVT_BATTERY_DATA_READY";
	case MODEM_EVT_NEIGHBOR_CELLS_DATA_NOT_READY:
		return "MODEM_EVT_NEIGHBOR_CELLS_DATA_NOT_READY";
	case MODEM_EVT_NEIGHBOR_CELLS_DATA_READY:
		return "MODEM_EVT_NEIGHBOR_CELLS_DATA_READY";
	case MODEM_EVT_SHUTDOWN_READY:
		return "MODEM_EVT_SHUTDOWN_READY";
	case MODEM_EVT_ERROR:
		return "MODEM_EVT_ERROR";
	default:
		return "Unknown event";
	}
}

static int log_event(const struct event_header *eh, char *buf,
		     size_t buf_len)
{
	const struct modem_module_event *event = cast_modem_module_event(eh);

	if (event->type == MODEM_EVT_ERROR) {
		return snprintf(buf, buf_len, "%s - Error code %d",
				get_evt_type_str(event->type), event->data.err);
	}

	return snprintf(buf, buf_len, "%s", get_evt_type_str(event->type));
}

#if defined(CONFIG_PROFILER)

static void profile_event(struct log_event_buf *buf,
			 const struct event_header *eh)
{
	const struct modem_module_event *event = cast_modem_module_event(eh);

#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
	profiler_log_encode_string(buf, get_evt_type_str(event->type),
		strlen(get_evt_type_str(event->type)));
#else
	profiler_log_encode_u32(buf, event->type);
#endif
}

EVENT_INFO_DEFINE(modem_module_event,
#if defined(CONFIG_PROFILER_EVENT_TYPE_STRING)
		  ENCODE(PROFILER_ARG_STRING),
#else
		  ENCODE(PROFILER_ARG_U32),
#endif
		  ENCODE("type"),
		  profile_event);

#endif /* CONFIG_PROFILER */
EVENT_TYPE_DEFINE(modem_module_event,
		  CONFIG_MODEM_EVENTS_LOG,
		  log_event,
#if defined(CONFIG_PROFILER)
		  &modem_module_event_info);
#else
		  NULL);
#endif
