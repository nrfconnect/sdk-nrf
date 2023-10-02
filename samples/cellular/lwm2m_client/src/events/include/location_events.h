/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_EVENTS_H__
#define LOCATION_EVENTS_H__

#include <nrf_modem_gnss.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
struct gnss_agnss_request_event {
	struct app_event_header header;

	struct nrf_modem_gnss_agnss_data_frame agnss_req;
};

APP_EVENT_TYPE_DECLARE(gnss_agnss_request_event);

#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS */
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

#endif /* LOCATION_EVENTS_H__ */
