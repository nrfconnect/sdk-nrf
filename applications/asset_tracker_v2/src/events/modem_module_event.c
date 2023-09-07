/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "modem_module_event.h"
#include "common_module_event.h"

static char *get_evt_type_str(enum modem_module_event_type type)
{
	switch (type) {
	case MODEM_EVT_INITIALIZED:
		return "MODEM_EVT_INITIALIZED";
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
	case MODEM_EVT_SHUTDOWN_READY:
		return "MODEM_EVT_SHUTDOWN_READY";
	case MODEM_EVT_ERROR:
		return "MODEM_EVT_ERROR";
	case MODEM_EVT_CARRIER_EVENT_LTE_LINK_UP_REQUEST:
		return "MODEM_EVT_CARRIER_EVENT_LTE_LINK_UP_REQUEST";
	case MODEM_EVT_CARRIER_EVENT_LTE_LINK_DOWN_REQUEST:
		return "MODEM_EVT_CARRIER_EVENT_LTE_LINK_DOWN_REQUEST";
	case MODEM_EVT_CARRIER_EVENT_LTE_POWER_OFF_REQUEST:
		return "MODEM_EVT_CARRIER_EVENT_LTE_POWER_OFF_REQUEST";
	case MODEM_EVT_CARRIER_FOTA_PENDING:
		return "MODEM_EVT_CARRIER_FOTA_PENDING";
	case MODEM_EVT_CARRIER_FOTA_STOPPED:
		return "MODEM_EVT_CARRIER_FOTA_STOPPED";
	case MODEM_EVT_CARRIER_REBOOT_REQUEST:
		return "MODEM_EVT_CARRIER_REBOOT_REQUEST";
	default:
		return "Unknown event";
	}
}

static void log_event(const struct app_event_header *aeh)
{
	const struct modem_module_event *event = cast_modem_module_event(aeh);

	if (event->type == MODEM_EVT_ERROR) {
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
	const struct modem_module_event *event = cast_modem_module_event(aeh);

#if defined(CONFIG_NRF_PROFILER_EVENT_TYPE_STRING)
	nrf_profiler_log_encode_string(buf, get_evt_type_str(event->type));
#else
	nrf_profiler_log_encode_uint8(buf, event->type);
#endif
}

COMMON_APP_EVENT_INFO_DEFINE(modem_module_event,
			 profile_event);

#endif /* CONFIG_NRF_PROFILER */

COMMON_APP_EVENT_TYPE_DEFINE(modem_module_event,
			 log_event,
			 &modem_module_event_info,
			 APP_EVENT_FLAGS_CREATE(
				IF_ENABLED(CONFIG_MODEM_EVENTS_LOG,
					(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
