/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define LOG_MODULE_NAME net_lwm2m_location_assist
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/lwm2m_path.h>
#include <net/lwm2m_client_utils_location.h>
#include <zephyr/net/lwm2m.h>

#include "gnss_assistance_obj.h"

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
#define CELL_LOCATION_PATHS 7
#else
#define CELL_LOCATION_PATHS 6
#endif

#define PGPS_LOCATION_PATHS_DEFAULT 4
#define PGPS_LOCATION_PATHS_MAX 5

#define AGPS_LOCATION_PATHS_DEFAULT 7
#define AGPS_LOCATION_PATHS_MAX 8

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
void location_assistance_agps_set_mask(const struct nrf_modem_gnss_agps_data_frame *agps_req)
{
	uint32_t mask = 0;

	if (agps_req->data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_UTC;
	}
	if (agps_req->sv_mask_ephe) {
		mask |= LOCATION_ASSIST_NEEDS_EPHEMERIES;
	}
	if (agps_req->sv_mask_alm) {
		mask |= LOCATION_ASSIST_NEEDS_ALMANAC;
	}
	if (agps_req->data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {

		mask |= LOCATION_ASSIST_NEEDS_KLOBUCHAR;
	}
	if (agps_req->data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_NEQUICK;
	}
	if (agps_req->data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_TOW;
		mask |= LOCATION_ASSIST_NEEDS_CLOCK;
	}
	if (agps_req->data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_LOCATION;
	}
	if (agps_req->data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_INTEGRITY;
	}

	location_assist_agps_request_set(mask);
}

int location_assistance_agps_request_send(struct lwm2m_ctx *ctx, bool confirmable)
{
	int path_count = AGPS_LOCATION_PATHS_DEFAULT;
	/* Allocate buffer for a-gps request */
	int ret = location_assist_agps_alloc_buf(
			CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS_BUF_SIZE);

	if (ret != 0) {
		LOG_ERR("Unable to send request (%d)", ret);
		return ret;
	}

	gnss_assistance_prepare_download();

	if (location_assist_agps_get_elevation_mask() >= 0) {
		path_count++;
	}

	char const *send_path[AGPS_LOCATION_PATHS_MAX] = {
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_ASSIST_TYPE),
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_AGPS_MASK),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 8), /* ECI */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 9), /* MNC */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 10), /* MCC */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 2), /* RSRP */
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 12), /* LAC */
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_ELEVATION_MASK)
	};

	/* Send Request to server */
	return lwm2m_engine_send(ctx, send_path, path_count, confirmable);
}
#endif

int location_assistance_ground_fix_request_send(struct lwm2m_ctx *ctx, bool confirmable)
{
	char const *send_path[CELL_LOCATION_PATHS] = {
		LWM2M_PATH(GROUND_FIX_OBJECT_ID, 0, GROUND_FIX_SEND_LOCATION_BACK),
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
	return lwm2m_engine_send(ctx, send_path, CELL_LOCATION_PATHS, confirmable);
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
int location_assistance_pgps_request_send(struct lwm2m_ctx *ctx, bool confirmable)
{
	LOG_INF("Send P-GPS request");
	location_assist_pgps_request_set();
	gnss_assistance_prepare_download();
	int path_count = PGPS_LOCATION_PATHS_DEFAULT;

	if (location_assist_pgps_get_start_gps_day() != 0) {
		path_count++;
	}
	char const *send_path[PGPS_LOCATION_PATHS_MAX] = {
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_ASSIST_TYPE),
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_PRED_COUNT),
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_PRED_INTERVAL),
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_START_GPS_TIME_OF_DAY),
		LWM2M_PATH(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_START_GPS_DAY)
	};

	/* Send Request to server */
	return lwm2m_engine_send(ctx, send_path, path_count, confirmable);
}
#endif
