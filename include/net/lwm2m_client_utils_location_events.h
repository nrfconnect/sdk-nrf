/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file lwm2m_client_utils_location_events.h
 * @defgroup lwm2m_client_utils_location_events LwM2M location events
 * @ingroup lwm2m_client_utils
 * @{
 * @brief Header file that contains declarations for events that are used to interact with the
 *	  LwM2M client utils location event handler library, location/location_event_handler.c.
 */

#ifndef LWM2M_CLIENT_UTILS_LOCATION_EVENTS_H__
#define LWM2M_CLIENT_UTILS_LOCATION_EVENTS_H__

#include <nrf_modem_gnss.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS)
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
struct gnss_agps_request_event {
	struct app_event_header header;

	struct nrf_modem_gnss_agps_data_frame agps_req;
};

APP_EVENT_TYPE_DECLARE(gnss_agps_request_event);

#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS */
#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
struct ground_fix_location_request_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(ground_fix_location_request_event);

struct ground_fix_location_inform_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(ground_fix_location_inform_event);
#endif /* CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT */

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
struct pgps_data_request_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(pgps_data_request_event);
#endif /* CONFIG_LWM2M_CLIEN_UTILS_LOCATION_ASSIST_PGPS */
#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS */

#endif /* LWM2M_CLIENT_UTILS_LOCATION_EVENTS_H__ */

/**@} */
