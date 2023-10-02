/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>
#include <zephyr/net/lwm2m_path.h>
#include <net/lwm2m_client_utils_location.h>
#include <zephyr/net/lwm2m.h>

#include "location_events.h"

static struct lwm2m_ctx *client_ctx;

#define REQUEST_WAIT_INTERVAL K_SECONDS(5)

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
static bool handle_agnss_request(const struct gnss_agnss_request_event *event)
{
	while (location_assistance_agnss_set_mask(&event->agnss_req) == -EAGAIN) {
		k_sleep(REQUEST_WAIT_INTERVAL);
	}
	while (location_assistance_agnss_request_send(client_ctx) == -EAGAIN) {
		k_sleep(REQUEST_WAIT_INTERVAL);
	}

	return true;
}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
static bool handle_ground_fix_location_event(bool send_back)
{
	ground_fix_set_report_back(send_back);
	location_assistance_ground_fix_request_send(client_ctx);

	return true;
}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
static bool handle_pgps_data_request_event(void)
{
	while (location_assistance_pgps_request_send(client_ctx) == -EAGAIN) {
		k_sleep(REQUEST_WAIT_INTERVAL);
	}
	return true;
}
#endif

static bool event_handler(const struct app_event_header *eh)
{
	if (client_ctx == 0) {
		return false;
	}
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
	if (is_gnss_agnss_request_event(eh)) {
		struct gnss_agnss_request_event *event = cast_gnss_agnss_request_event(eh);

		handle_agnss_request(event);
		return true;
	}
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
	if (is_ground_fix_location_request_event(eh)) {
		handle_ground_fix_location_event(true);
		return true;
	} else if (is_ground_fix_location_inform_event(eh)) {
		handle_ground_fix_location_event(false);
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

APP_EVENT_LISTENER(location_handler, event_handler);
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
APP_EVENT_SUBSCRIBE(location_handler, gnss_agnss_request_event);
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
APP_EVENT_SUBSCRIBE(location_handler, ground_fix_location_request_event);
APP_EVENT_SUBSCRIBE(location_handler, ground_fix_location_inform_event);
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
APP_EVENT_SUBSCRIBE(location_handler, pgps_data_request_event);
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
