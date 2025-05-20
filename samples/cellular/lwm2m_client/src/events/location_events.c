/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <net/lwm2m_client_utils_location.h>

#include "location_events.h"

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
static void log_gnss_agnss_request_event(const struct app_event_header *eh)
{
	APP_EVENT_MANAGER_LOG(eh, "got agnss request event");
}

APP_EVENT_TYPE_DEFINE(gnss_agnss_request_event, log_gnss_agnss_request_event, NULL,
		      APP_EVENT_FLAGS_CREATE());

#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
static void log_ground_fix_location_request_event(const struct app_event_header *eh)
{
	APP_EVENT_MANAGER_LOG(eh, "got ground fix location request event");
}

APP_EVENT_TYPE_DEFINE(ground_fix_location_request_event, log_ground_fix_location_request_event,
		      NULL, APP_EVENT_FLAGS_CREATE());

static void log_ground_fix_location_inform_event(const struct app_event_header *eh)
{
	APP_EVENT_MANAGER_LOG(eh, "got ground fix location inform event");
}

APP_EVENT_TYPE_DEFINE(ground_fix_location_inform_event, log_ground_fix_location_inform_event, NULL,
		      APP_EVENT_FLAGS_CREATE());
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
static void log_pgps_data_request_event(const struct app_event_header *eh)
{
	APP_EVENT_MANAGER_LOG(eh, "got pgps data request event");
}

APP_EVENT_TYPE_DEFINE(pgps_data_request_event, log_pgps_data_request_event, NULL,
		      APP_EVENT_FLAGS_CREATE());
#endif
