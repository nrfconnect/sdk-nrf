/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>

#include <zephyr/net/lwm2m_path.h>
#include <net/lwm2m_client_utils_location.h>
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
	uint32_t mask = 0;

	if (event->agps_req.data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_UTC;
	}
	if (event->agps_req.sv_mask_ephe) {
		mask |= LOCATION_ASSIST_NEEDS_EPHEMERIES;
	}
	if (event->agps_req.sv_mask_alm) {
		mask |= LOCATION_ASSIST_NEEDS_ALMANAC;
	}
	if (event->agps_req.data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {

		mask |= LOCATION_ASSIST_NEEDS_KLOBUCHAR;
	}
	if (event->agps_req.data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_NEQUICK;
	}
	if (event->agps_req.data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_TOW;
		mask |= LOCATION_ASSIST_NEEDS_CLOCK;
	}
	if (event->agps_req.data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_LOCATION;
	}
	if (event->agps_req.data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_INTEGRITY;
	}

	location_assist_agps_request_set(mask);

	char const *send_path[AGPS_LOCATION_PATHS] = {
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_ASSIST_TYPE),
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_AGPS_MASK),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 8), /* ECI */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 9), /* MNC */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 10), /* MCC */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 2), /* RSRP */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 12) /* LAC */
	};

	/* Send Request to server */
	lwm2m_engine_send(client_ctx, send_path, AGPS_LOCATION_PATHS,
				true);

	return true;
}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL)
static bool handle_cell_location_event(bool send_back)
{
	if (send_back) {
		LOG_INF("Cell location event with send back request");
		location_assist_cell_request_set();
	} else {
		LOG_INF("Cell location inform event");
		location_assist_cell_inform_set();
	}

	char const *send_path[CELL_LOCATION_PATHS] = {
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, LOCATION_ASSIST_ASSIST_TYPE),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 8), /* ECI */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 9), /* MNC */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 10), /* MCC */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 2), /* RSRP */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 12), /* LAC */
#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
		LWM2M_PATH(ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID)
#endif
	};

	/* Send Request to server */
	lwm2m_engine_send(client_ctx, send_path, CELL_LOCATION_PATHS,
				true);

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

		LOG_DBG("Got agps request event");
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
