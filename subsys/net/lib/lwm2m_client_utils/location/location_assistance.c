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
#include "ground_fix_obj.h"

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS_BUF_SIZE)
#define AGNSS_BUF_SIZE CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS_BUF_SIZE
#else
#define AGNSS_BUF_SIZE 4096
#endif

#define GROUND_FIX_PATHS_DEFAULT 6
#define GROUND_FIX_PATHS_MAX 8

#define PGPS_LOCATION_PATHS_DEFAULT 4
#define PGPS_LOCATION_PATHS_MAX 5

#define AGNSS_LOCATION_PATHS_DEFAULT 7
#define AGNSS_LOCATION_PATHS_MAX 8

/* Location Assistance resource IDs */
#define GROUND_FIX_SEND_LOCATION_BACK		0
#define GROUND_FIX_RESULT_CODE			1
#define GROUND_FIX_LATITUDE			2
#define GROUND_FIX_LONGITUDE			3
#define GROUND_FIX_ACCURACY			4

#define GNSS_ASSIST_ASSIST_TYPE				0
#define GNSS_ASSIST_AGNSS_MASK				1
#define GNSS_ASSIST_PGPS_PRED_COUNT			2
#define GNSS_ASSIST_PGPS_PRED_INTERVAL			3
#define GNSS_ASSIST_PGPS_START_GPS_DAY			4
#define GNSS_ASSIST_PGPS_START_GPS_TIME_OF_DAY		5
#define GNSS_ASSIST_ASSIST_DATA				6
#define GNSS_ASSIST_RESULT_CODE				7
#define GNSS_ASSIST_ELEVATION_MASK			8

static struct lwm2m_ctx *req_client_ctx;

#define INITIAL_RETRY_INTERVAL 675
#define MAXIMUM_RETRY_INTERVAL 86400

static bool permanent_error;
static bool temp_error;
static int retry_seconds;

static location_assistance_result_code_cb_t result_code_cb;

static int do_agnss_request_send(struct lwm2m_ctx *ctx);
static int do_pgps_request_send(struct lwm2m_ctx *ctx);
static int do_ground_fix_request_send(struct lwm2m_ctx *ctx);

static struct k_work_delayable location_assist_gnss_work;
static void location_assist_gnss_work_handler(struct k_work *work)
{
	int assist_type = location_assist_gnss_type_get();

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS) &&
	    assist_type == ASSISTANCE_REQUEST_TYPE_AGNSS) {
		do_agnss_request_send(req_client_ctx);
	}

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS) &&
	    assist_type == ASSISTANCE_REQUEST_TYPE_PGPS) {
		do_pgps_request_send(req_client_ctx);
	}
}

static void location_assist_gnss_result_cb(int32_t data)
{
	switch (data) {
	case LOCATION_ASSIST_RESULT_CODE_OK:
		retry_seconds = INITIAL_RETRY_INTERVAL;
		temp_error = false;
		break;
	case LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR:
		LOG_ERR("Permanent error interfacing location services, fix and reboot");
		permanent_error = true;
		break;
	case LOCATION_ASSIST_RESULT_CODE_TEMP_ERR:
		LOG_ERR("Temporary error in location services, scheduling retry in %d s",
		retry_seconds);
		temp_error = true;
		k_work_schedule(&location_assist_gnss_work, K_SECONDS(retry_seconds));
		if (retry_seconds < MAXIMUM_RETRY_INTERVAL) {
			retry_seconds *= 2;
		}
		break;
	default:
		LOG_ERR("Unknown error %d", data);
	}

	/* Notify the application about the result if it has requested a callback */
	if (result_code_cb) {
		result_code_cb(data);
	}
}

static struct k_work_delayable location_assist_ground_fix_work;
static bool ground_temp_error;
static int ground_retry_seconds;

static void location_assist_ground_fix_work_handler(struct k_work *work)
{
	do_ground_fix_request_send(req_client_ctx);
}

static void location_assist_ground_fix_result_cb(int32_t data)
{
	switch (data) {
	case LOCATION_ASSIST_RESULT_CODE_OK:
		ground_retry_seconds = 0;
		ground_temp_error = false;
		break;
	case LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR:
		LOG_ERR("Permanent error interfacing location services, fix and reboot");
		permanent_error = true;
		break;
	case LOCATION_ASSIST_RESULT_CODE_TEMP_ERR:
		LOG_ERR("Temporary error in location services, scheduling retry in %d s",
		ground_retry_seconds);
		ground_temp_error = true;
		k_work_schedule(&location_assist_ground_fix_work, K_SECONDS(ground_retry_seconds));
		if (ground_retry_seconds < MAXIMUM_RETRY_INTERVAL) {
			ground_retry_seconds *= 2;
		}
		break;
	default:
		LOG_ERR("Unknown error %d", data);
	}

	/* Notify the application about the result if it has requested a callback */
	if (result_code_cb) {
		result_code_cb(data);
	}
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
int location_assistance_agnss_set_mask(const struct nrf_modem_gnss_agnss_data_frame *agnss_req)
{
	uint32_t mask = 0;

	/* GPS data need is always expected to be present and first in list. */
	__ASSERT(agnss_req->system_count > 0,
		 "GNSS system data need not found");
	__ASSERT(agnss_req->system[0].system_id == NRF_MODEM_GNSS_SYSTEM_GPS,
		 "GPS data need not found");

	if (agnss_req->data_flags & NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_UTC;
	}
	if (agnss_req->system[0].sv_mask_ephe) {
		mask |= LOCATION_ASSIST_NEEDS_EPHEMERIES;
	}
	if (agnss_req->system[0].sv_mask_alm) {
		mask |= LOCATION_ASSIST_NEEDS_ALMANAC;
	}
	if (agnss_req->data_flags & NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST) {

		mask |= LOCATION_ASSIST_NEEDS_KLOBUCHAR;
	}
	if (agnss_req->data_flags & NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_NEQUICK;
	}
	if (agnss_req->data_flags & NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_TOW;
		mask |= LOCATION_ASSIST_NEEDS_CLOCK;
	}
	if (agnss_req->data_flags & NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_LOCATION;
	}
	if (agnss_req->data_flags & NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST) {
		mask |= LOCATION_ASSIST_NEEDS_INTEGRITY;
	}

	/* Allow setting the mask even when the object is busy or there is an error. */
	location_assist_agnss_request_set(mask);

	if (permanent_error) {
		LOG_ERR("Permanent error interfacing location services, fix and reboot");
		return -EPIPE;
	} else if (temp_error) {
		LOG_ERR("Temporary error in location services, retry scheduled");
		return -EALREADY;
	}

	if (location_assist_gnss_is_busy()) {
		LOG_WRN("GNSS object busy handling request");
		return -EAGAIN;
	}

	location_assist_gnss_type_set(ASSISTANCE_REQUEST_TYPE_AGNSS);

	return 0;
}
#endif

static int do_agnss_request_send(struct lwm2m_ctx *ctx)
{
	int path_count = AGNSS_LOCATION_PATHS_DEFAULT;
	/* Allocate buffer for A-GNSS request */
	int ret = location_assist_agnss_alloc_buf(AGNSS_BUF_SIZE);

	if (ret != 0) {
		LOG_ERR("Unable to send request (%d)", ret);
		return ret;
	}

	gnss_assistance_prepare_download();
	req_client_ctx = ctx;

	if (location_assist_agnss_get_elevation_mask() >= 0) {
		path_count++;
	}

	const struct lwm2m_obj_path send_path[AGNSS_LOCATION_PATHS_MAX] = {
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_ASSIST_TYPE),
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_AGNSS_MASK),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 8), /* ECI */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 9), /* MNC */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 10), /* MCC */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 2), /* RSRP */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 12), /* LAC */
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_ELEVATION_MASK)
	};

	/* Send Request to server */
	return lwm2m_send_cb(ctx, send_path, path_count, NULL);
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
int location_assistance_agnss_request_send(struct lwm2m_ctx *ctx)
{
	if (permanent_error) {
		LOG_ERR("Permanent error interfacing location services, fix and reboot");
		return -EPIPE;
	} else if (temp_error) {
		LOG_ERR("Temporary error in location services retry scheduled");
		return -EALREADY;
	}

	if (location_assist_gnss_is_busy()) {
		LOG_WRN("GNSS object busy handling request");
		return -EAGAIN;
	}
	LOG_INF("Send A-GNSS request");
	retry_seconds = INITIAL_RETRY_INTERVAL;

	return do_agnss_request_send(ctx);
}
#endif

static int do_ground_fix_request_send(struct lwm2m_ctx *ctx)
{
	int path_count = GROUND_FIX_PATHS_DEFAULT;

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
	path_count++;
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_VISIBLE_WIFI_AP_OBJ_SUPPORT)
	path_count++;
#endif

	const struct lwm2m_obj_path send_path[GROUND_FIX_PATHS_MAX] = {
		LWM2M_OBJ(GROUND_FIX_OBJECT_ID, 0, GROUND_FIX_SEND_LOCATION_BACK),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 8), /* ECI */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 9), /* MNC */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 10), /* MCC */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 2), /* RSRP */
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, 12), /* LAC */
#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
		LWM2M_OBJ(ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID),
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_VISIBLE_WIFI_AP_OBJ_SUPPORT)
		LWM2M_OBJ(VISIBLE_WIFI_AP_OBJECT_ID),
#endif
	};

	/* Send Request to server */
	return lwm2m_send_cb(ctx, send_path, path_count, NULL);
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
int location_assistance_ground_fix_request_send(struct lwm2m_ctx *ctx)
{
	int ret;

	if (permanent_error) {
		LOG_ERR("Permanent error interfacing location services, fix and reboot");
		return -EPIPE;
	} else if (ground_temp_error || ground_retry_seconds) {
		LOG_ERR("Temporary error in location services retry scheduled");
		return -EALREADY;
	}

	ret = do_ground_fix_request_send(ctx);
	if (ret == 0) {
		ground_retry_seconds = INITIAL_RETRY_INTERVAL;
	}

	return ret;
}
#endif

static int do_pgps_request_send(struct lwm2m_ctx *ctx)
{
	int path_count = PGPS_LOCATION_PATHS_DEFAULT;
	LOG_INF("Send P-GPS request");
	location_assist_gnss_type_set(ASSISTANCE_REQUEST_TYPE_PGPS);
	gnss_assistance_prepare_download();
	req_client_ctx = ctx;

	if (location_assist_pgps_get_start_gps_day() != 0) {
		path_count++;
	}
	const struct lwm2m_obj_path send_path[PGPS_LOCATION_PATHS_MAX] = {
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_ASSIST_TYPE),
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_PRED_COUNT),
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_PRED_INTERVAL),
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_START_GPS_TIME_OF_DAY),
		LWM2M_OBJ(GNSS_ASSIST_OBJECT_ID, 0, GNSS_ASSIST_PGPS_START_GPS_DAY)
	};

	/* Send Request to server */
	return lwm2m_send_cb(ctx, send_path, path_count, NULL);
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
int location_assistance_pgps_request_send(struct lwm2m_ctx *ctx)
{
	if (permanent_error) {
		LOG_ERR("Permanent error interfacing location services, fix and reboot");
		return -EPIPE;
	} else if (temp_error) {
		LOG_ERR("Temporary error in location services, retry scheduled");
		return -EALREADY;
	}

	if (location_assist_gnss_is_busy()) {
		LOG_WRN("GNSS object busy handling request");
		return -EAGAIN;
	}

	retry_seconds = INITIAL_RETRY_INTERVAL;

	return do_pgps_request_send(ctx);
}
#endif
static bool resend_init_done;

int location_assistance_init_resend_handler(void)
{

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS) ||
	    IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)) {
		if (resend_init_done) {
			k_work_cancel_delayable(&location_assist_gnss_work);
			temp_error = false;
			permanent_error = false;
		} else {
			k_work_init_delayable(&location_assist_gnss_work,
					      location_assist_gnss_work_handler);
		}

		gnss_assistance_set_result_code_cb(location_assist_gnss_result_cb);
	}

	if (IS_ENABLED(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)) {
		if (resend_init_done) {
			k_work_cancel_delayable(&location_assist_ground_fix_work);
			ground_retry_seconds = 0;
			ground_temp_error = false;
			permanent_error = false;
		} else {
			k_work_init_delayable(&location_assist_ground_fix_work,
				      location_assist_ground_fix_work_handler);
		}

		ground_fix_set_result_code_cb(location_assist_ground_fix_result_cb);
	}

	resend_init_done = true;
	return 0;
}

void location_assistance_set_result_code_cb(location_assistance_result_code_cb_t cb)
{
	result_code_cb = cb;
}
