/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>

#include <zephyr/net/lwm2m_path.h>
#include <net/lwm2m_client_utils_location.h>
#include <net/lwm2m_client_utils_location_events.h>
#include <net/lwm2m_client_utils.h>
#include <zephyr/net/lwm2m.h>

static struct lwm2m_ctx *client_ctx;

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
#define CELL_LOCATION_PATHS 7
#else
#define CELL_LOCATION_PATHS 6
#endif

#define AGPS_LOCATION_PATHS 7

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_location_event_handler, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
static bool handle_agps_request(const struct gnss_agps_request_event *event)
{
	LOG_DBG("AGPS request");
	location_assistance_agps_set_mask(&event->agps_req);
	location_assistance_agps_request_send(client_ctx, true);

	return true;
}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL)
static bool handle_cell_location_event(bool send_back)
{
	ground_fix_set_report_back(send_back);
	location_assistance_ground_fix_request_send(client_ctx, true);

	return true;
}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
static bool handle_pgps_data_request_event(void)
{
	location_assistance_pgps_request_send(client_ctx, true);
	return true;
}
#endif

static bool event_handler(const struct app_event_header *eh)
{
	if (client_ctx == 0) {
		return false;
	}
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
	if (is_gnss_agps_request_event(eh)) {
		struct gnss_agps_request_event *event = cast_gnss_agps_request_event(eh);

		handle_agps_request(event);
		return true;
	}
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL)
	if (is_cell_location_request_event(eh)) {
		handle_cell_location_event(true);
		return true;
	} else if (is_cell_location_inform_event(eh)) {
		handle_cell_location_event(false);
		return true;
	}
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
	if (is_pgps_data_request_event(eh)) {
		handle_pgps_data_request_event();
		return true;
	}
#endif

	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
APP_EVENT_SUBSCRIBE(MODULE, gnss_agps_request_event);
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL)
APP_EVENT_SUBSCRIBE(MODULE, cell_location_request_event);
APP_EVENT_SUBSCRIBE(MODULE, cell_location_inform_event);
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
APP_EVENT_SUBSCRIBE(MODULE, pgps_data_request_event);
#endif

int location_event_handler_init(struct lwm2m_ctx *ctx)
{
	int err = 0;

	if (client_ctx == 0) {
		client_ctx = ctx;
	} else {
		err = -EALREADY;
	}

	return err;
}
