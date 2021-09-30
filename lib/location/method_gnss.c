/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <logging/log.h>

#include <modem/location.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <nrf_modem_gnss.h>
#include <nrf_errno.h>
#include "loc_core.h"
#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_agps.h>
#include <stdlib.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_agps.h>
#include <net/nrf_cloud_pgps.h>
#include <pm_config.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <date_time.h>
#endif

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_PGPS)
BUILD_ASSERT(IS_ENABLED(CONFIG_DATE_TIME));
#endif

#if !defined(CONFIG_NRF_CLOUD_AGPS) && !defined(CONFIG_NRF_CLOUD_PGPS)
/* range 10240-3456000000 ms, see AT command %XMODEMSLEEP */
#define MIN_SLEEP_DURATION_FOR_STARTING_GNSS 10240
#define AT_MDM_SLEEP_NOTIF_START "AT%%XMODEMSLEEP=1,%d,%d"
#define AT_MDM_SLEEP_NOTIF_STOP "AT%XMODEMSLEEP=0" /* not used at the moment */
#endif
#if (defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS))
#define AGPS_REQUEST_RECV_BUF_SIZE 3500
#define AGPS_REQUEST_HTTPS_RESP_HEADER_SIZE 500
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#define NUM_PREDICTIONS CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS
#define PREDICTION_PERIOD CONFIG_NRF_CLOUD_PGPS_PREDICTION_PERIOD
#endif

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

struct k_work_args {
	struct k_work work_item;
	struct loc_gnss_config gnss_config;
};

static struct k_work_args method_gnss_start_work;
static struct k_work method_gnss_fix_work;
static struct k_work method_gnss_timeout_work;

#if defined(CONFIG_NRF_CLOUD_AGPS)
static struct k_work method_gnss_agps_request_work;
static struct nrf_modem_gnss_agps_data_frame gnss_api_agps_request;
static char agps_data_buf[AGPS_REQUEST_RECV_BUF_SIZE];
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct k_work method_gnss_pgps_request_work;
static struct k_work method_gnss_manage_pgps_work;
static struct k_work method_gnss_notify_pgps_work;
static struct nrf_cloud_pgps_prediction *prediction;
#endif

static int fix_attempts_remaining;
static bool first_fix_obtained;
static bool running;
static K_SEM_DEFINE(entered_psm_mode, 0, 1);

#if (defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS))
static char rest_api_recv_buf[CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE +
			      AGPS_REQUEST_HTTPS_RESP_HEADER_SIZE];
static struct gps_agps_request agps_request;
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_manage_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	LOG_INF("Sending prediction to modem...");
#if defined(CONFIG_NRF_CLOUD_AGPS)
	err = nrf_cloud_pgps_inject(prediction, &agps_request);
#else
	err = nrf_cloud_pgps_inject(prediction, NULL);
#endif
	if (err) {
		LOG_ERR("Unable to send prediction to modem: %d", err);
	}

	err = nrf_cloud_pgps_preemptive_updates();
	if (err) {
		LOG_ERR("Error requesting updates: %d", err);
	}
}

void method_gnss_pgps_handler(struct nrf_cloud_pgps_event *event)
{
	/* GPS unit asked for it, but we didn't have it; check now */
	LOG_INF("P-GPS event type: %d", event->type);

	if (event->type == PGPS_EVT_AVAILABLE) {
		prediction = event->prediction;
		k_work_submit_to_queue(loc_core_work_queue_get(),
				       &method_gnss_manage_pgps_work);
	} else if (event->type == PGPS_EVT_REQUEST) {
		k_work_submit_to_queue(loc_core_work_queue_get(),
				       &method_gnss_pgps_request_work);
	}
}

static void method_gnss_notify_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("Error requesting notification of prediction availability: %d", err);
	}
}
#endif

#if !defined(CONFIG_NRF_CLOUD_AGPS) && !defined(CONFIG_NRF_CLOUD_PGPS)
void method_gnss_lte_ind_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
		if (evt->modem_sleep.type == LTE_LC_MODEM_SLEEP_PSM) {
			/* Signal to method_gnss_positioning_work_fn() that GNSS can be started. */
			k_sem_give(&entered_psm_mode);
		}
		break;

	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
		/* Prevent GNSS from starting while LTE is running. */
		k_sem_reset(&entered_psm_mode);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		/* If the PSM becomes disabled e.g. due to network change, allow GNSS to be started
		 * in case there was a pending position request waiting for the sleep to start.
		 */
		if ((evt->psm_cfg.tau == -1) || (evt->psm_cfg.active_time == -1)) {
			k_sem_give(&entered_psm_mode);
		}
		break;
	default:
		break;
	}
}
#endif // !CONFIG_NRF_CLOUD_AGPS && !CONFIG_NRF_CLOUD_PGPS

#if defined(CONFIG_NRF_CLOUD_AGPS)
static int method_gnss_get_modem_info(struct lte_lc_cell *serving_cell)
{
	__ASSERT_NO_MSG(serving_cell != NULL);

	int err = modem_info_init();

	if (err) {
		LOG_ERR("Could not initialize modem info module, error: %d",
			err);
		return err;
	}

	char resp_buf_cellid[MODEM_INFO_MAX_RESPONSE_SIZE];
	char resp_buf_tac[MODEM_INFO_MAX_RESPONSE_SIZE];
	char resp_buf_cops[MODEM_INFO_MAX_RESPONSE_SIZE];

	err = modem_info_string_get(MODEM_INFO_CELLID,
				    resp_buf_cellid,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		LOG_ERR("Could not obtain cell id");
		return err;
	}

	err = modem_info_string_get(MODEM_INFO_AREA_CODE,
				    resp_buf_tac,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		LOG_ERR("Could not obtain tracking area code");
		return err;
	}

	/* Request for MODEM_INFO_MNC returns both MNC and MCC in the same string. */
	err = modem_info_string_get(MODEM_INFO_MNC, resp_buf_cops, MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		LOG_ERR("Could not obtain current operator information");
		return err;
	}

	serving_cell->id = strtol(resp_buf_cellid, NULL, 16);
	serving_cell->tac = strtol(resp_buf_tac, NULL, 16);
	serving_cell->mnc = strtol(&resp_buf_cops[3], NULL, 10);
	/* Null-terminated MCC, read and store it. */
	resp_buf_cops[3] = '\0';
	serving_cell->mcc = strtol(resp_buf_cops, NULL, 10);

	return 0;
}

static void method_gnss_agps_request_work_fn(struct k_work *item)
{
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = CONFIG_NRF_CLOUD_REST_RECV_TIMEOUT * MSEC_PER_SEC,
		.auth = CONFIG_AGPS_LOCATION_SERVICE_NRF_CLOUD_JWT_STRING,
		.rx_buf = rest_api_recv_buf,
		.rx_buf_len = sizeof(rest_api_recv_buf),
		.fragment_size = 0
	};

	struct nrf_cloud_rest_agps_request request = {
						      NRF_CLOUD_REST_AGPS_REQ_CUSTOM,
						      &agps_request,
						      NULL};
	struct lte_lc_cells_info net_info = {0};

	/* Get network info for the A-GPS location request. */
	int err = method_gnss_get_modem_info(&net_info.current_cell);

	if (err) {
		LOG_WRN("Requesting A-GPS data without location assistance");
	} else {
		request.net_info = &net_info;
	}

	struct nrf_cloud_rest_agps_result result = {agps_data_buf, sizeof(agps_data_buf), 0};

	nrf_cloud_rest_agps_data_get(&rest_ctx, &request, &result);
	nrf_cloud_agps_process(result.buf, result.agps_sz);
}
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_pgps_request_work_fn(struct k_work *item)
{
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = CONFIG_NRF_CLOUD_REST_RECV_TIMEOUT * MSEC_PER_SEC,
		.auth = CONFIG_AGPS_LOCATION_SERVICE_NRF_CLOUD_JWT_STRING,
		.rx_buf = rest_api_recv_buf,
		.rx_buf_len = sizeof(rest_api_recv_buf),
		.fragment_size = 0
	};

	struct gps_pgps_request pgps_req = {
		.prediction_count = NUM_PREDICTIONS,
		.prediction_period_min = PREDICTION_PERIOD,
		.gps_day = 0,
		.gps_time_of_day = 0
	};

	struct nrf_cloud_rest_pgps_request request = {&pgps_req};

	nrf_cloud_rest_pgps_data_get(&rest_ctx, &request);
	nrf_cloud_pgps_process(rest_ctx.response, rest_ctx.response_len);
}
#endif

bool method_gnss_agps_required(struct nrf_modem_gnss_agps_data_frame *request)
{
	int type_count = 0;

#if !defined(CONFIG_NRF_CLOUD_PGPS)
	/* If P-GPS is enabled, use predicted ephemeris to save power instead of requesting them
	 *  using A-GPS.
	 */
	if (request->sv_mask_ephe) {
		type_count++;
	}
	if (request->sv_mask_alm) {
		type_count++;
	}
#endif
	if (request->data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		type_count++;
	}
	if (request->data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {
		type_count++;
	}
	if (request->data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		type_count++;
	}
	if (request->data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		type_count++;
	}
	if (request->data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		type_count++;
	}
	if (request->data_flags &  NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		type_count++;
	}

	if (type_count == 0) {
		LOG_INF("No A-GPS data types requested");
		return false;
	} else {
		return true;
	}
}

#if defined(CONFIG_NRF_CLOUD_AGPS)
/* Converts the A-GPS data request from GNSS API to GPS driver format. */
static void method_gnss_agps_request_convert(
	const struct nrf_modem_gnss_agps_data_frame *src,
	struct gps_agps_request *dest)
{
	dest->sv_mask_ephe = src->sv_mask_ephe;
	dest->sv_mask_alm = src->sv_mask_alm;

	if (src->data_flags | NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		dest->utc = 1;
	}
	if (src->data_flags | NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {
		dest->klobuchar = 1;
	}
	if (src->data_flags | NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		dest->nequick = 1;
	}
	if (src->data_flags | NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		dest->system_time_tow = 1;
	}
	if (src->data_flags | NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		dest->position = 1;
	}
	if (src->data_flags | NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		dest->integrity = 1;
	}
}
#endif

static void method_gnss_request_assistance(void)
{
#if defined(CONFIG_NRF_CLOUD_AGPS)
	int err = nrf_modem_gnss_read(&gnss_api_agps_request,
				      sizeof(gnss_api_agps_request),
				      NRF_MODEM_GNSS_DATA_AGPS_REQ);

	if (err) {
		LOG_WRN("Reading A-GPS req data from GNSS failed, error: %d", err);
		return;
	}

	LOG_INF("A-GPS request from modem: emask:0x%08X amask:0x%08X flags:%d",
		gnss_api_agps_request.sv_mask_ephe,
		gnss_api_agps_request.sv_mask_alm,
		gnss_api_agps_request.data_flags);

	method_gnss_agps_request_convert(&gnss_api_agps_request, &agps_request);

	/* Check the request. If no A-GPS data types are requested, jump to P-GPS (if enabled) */
	if (method_gnss_agps_required(&gnss_api_agps_request)) {
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_agps_request_work);
	} else
#endif
	{
#if defined(CONFIG_NRF_CLOUD_PGPS)
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_notify_pgps_work);
#endif
	}
}

void method_gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_DBG("GNSS: Got fix");
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_fix_work);
		break;

	case NRF_MODEM_GNSS_EVT_PVT:
		LOG_DBG("GNSS: Got PVT event");
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_fix_work);
		break;

	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GNSS: Timeout");
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_timeout_work);
		break;
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		method_gnss_request_assistance();
		break;
	}
}

int method_gnss_cancel(void)
{
	int err = nrf_modem_gnss_stop();
	int sleeping;

	if (!err) {
		LOG_DBG("GNSS stopped");
	} else if (err == NRF_EPERM) {
		/* Let's convert NRF_EPERM to EPERM to make sure loc_core works properly */
		err = EPERM;
		/* NRF_EPERM is logged in loc_core */
	} else {
		LOG_ERR("Failed to stop GNSS");
	}

	running = false;

	/* Cancel any work that has not been started yet */
	(void)k_work_cancel(&method_gnss_start_work.work_item);

	/* If we are currently not in PSM, i.e., LTE is running, reset the semaphore to unblock
	 * method_gnss_positioning_work_fn() and allow the ongoing location request to terminate.
	 * Otherwise, don't reset the semaphore in order not to lose information about the current
	 * sleep state.
	 */
	sleeping = k_sem_count_get(&entered_psm_mode);
	if (!sleeping) {
		k_sem_reset(&entered_psm_mode);
	}

	fix_attempts_remaining = 0;
	first_fix_obtained = false;

	return -err;
}

#if !defined(CONFIG_NRF_CLOUD_AGPS) && !defined(CONFIG_NRF_CLOUD_PGPS)
static bool method_gnss_entered_psm(void)
{
	int ret = 0;
	int tau;
	int active_time;
	enum lte_lc_system_mode mode;

	lte_lc_system_mode_get(&mode, NULL);

	/* Don't care about PSM if we are in GNSS only mode */
	if (mode != LTE_LC_SYSTEM_MODE_GPS) {
		ret = lte_lc_psm_get(&tau, &active_time);
		if (ret < 0) {
			LOG_ERR("Cannot get PSM config: %d. Starting GNSS right away.", ret);
		} else if ((tau >= 0) & (active_time >= 0)) {
			LOG_INF("Waiting for LTE to enter PSM: tau %d active_time %d",
				tau, active_time);

			/* Wait for the PSM to start. If semaphore is reset during the waiting
			 * period, the position request was canceled.
			 */
			if (k_sem_take(&entered_psm_mode, K_FOREVER) == -EAGAIN) {
				return false;
			}
			k_sem_give(&entered_psm_mode);
		}
	}
	return true;
}

static void method_gnss_modem_sleep_notif_subscribe(uint32_t threshold_ms)
{
	char buf_sub[48];
	int err;

	snprintk(buf_sub, sizeof(buf_sub), AT_MDM_SLEEP_NOTIF_START, 0, threshold_ms);

	err = at_cmd_write(buf_sub, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Cannot subscribe to modem sleep notifications, err %d", err);
	} else {
		LOG_DBG("Subscribed to modem sleep notifications");
	}
}
#endif // !CONFIG_NRF_CLOUD_AGPS && !CONFIG_NRF_CLOUD_PGPS

static void method_gnss_fix_work_fn(struct k_work *item)
{
	struct nrf_modem_gnss_pvt_data_frame pvt_data;
	static struct loc_location location_result = { 0 };

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		return;
	}

	/* Store fix data only if we get a valid fix. Thus, the last valid data is always kept
	 * in memory and it is not overwritten in case we get an invalid fix.
	 */
	if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		first_fix_obtained = true;

		location_result.latitude = pvt_data.latitude;
		location_result.longitude = pvt_data.longitude;
		location_result.accuracy = pvt_data.accuracy;
		location_result.datetime.valid = true;
		location_result.datetime.year = pvt_data.datetime.year;
		location_result.datetime.month = pvt_data.datetime.month;
		location_result.datetime.day = pvt_data.datetime.day;
		location_result.datetime.hour = pvt_data.datetime.hour;
		location_result.datetime.minute = pvt_data.datetime.minute;
		location_result.datetime.second = pvt_data.datetime.seconds;
		location_result.datetime.ms = pvt_data.datetime.ms;
	}

	/* Start countdown of remaining fix attempts only once we get the first valid fix. */
	if (first_fix_obtained) {
		if (!fix_attempts_remaining) {
			/* We are done, stop GNSS and publish the fix */
			method_gnss_cancel();
			loc_core_event_cb(&location_result);
		} else {
			fix_attempts_remaining--;
		}
	}
}

static void method_gnss_timeout_work_fn(struct k_work *item)
{
	loc_core_event_cb_timeout();

	method_gnss_cancel();
}

static void method_gnss_positioning_work_fn(struct k_work *work)
{
	int err = 0;
	struct k_work_args *work_data = CONTAINER_OF(work, struct k_work_args, work_item);
	const struct loc_gnss_config gnss_config = work_data->gnss_config;

#if !defined(CONFIG_NRF_CLOUD_AGPS) && !defined(CONFIG_NRF_CLOUD_PGPS)
	if (!method_gnss_entered_psm()) {
		/* Location request was cancelled while waiting for the PSM to start. Do nothing. */
		return;
	}
#endif

	/* Configure GNSS to continuous tracking mode */
	err = nrf_modem_gnss_fix_interval_set(1);

	uint8_t use_case;

	switch (gnss_config.accuracy) {
	case LOC_ACCURACY_LOW:
		use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START |
			   NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
		err |= nrf_modem_gnss_use_case_set(use_case);
		break;

	case LOC_ACCURACY_NORMAL:
		use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;
		err |= nrf_modem_gnss_use_case_set(use_case);
		break;

	case LOC_ACCURACY_HIGH:
		use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;
		err |= nrf_modem_gnss_use_case_set(use_case);
		if (!err) {
			fix_attempts_remaining = gnss_config.num_consecutive_fixes;
		}
		break;
	}

	if (err) {
		LOG_ERR("Failed to configure GNSS");
		loc_core_event_cb_error();
		running = false;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS");
		loc_core_event_cb_error();
		running = false;
	}

	loc_core_timer_start(gnss_config.timeout);

	LOG_INF("GNSS started");
}

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
		LOG_INF("DATE_TIME_OBTAINED_MODEM");
		break;
	case DATE_TIME_OBTAINED_NTP:
		LOG_INF("DATE_TIME_OBTAINED_NTP");
		break;
	case DATE_TIME_OBTAINED_EXT:
		LOG_INF("DATE_TIME_OBTAINED_EXT");
		break;
	case DATE_TIME_NOT_OBTAINED:
		LOG_INF("DATE_TIME_NOT_OBTAINED");
		break;
	default:
		break;
	}
}
#endif

int method_gnss_location_get(const struct loc_method_config *config)
{
	const struct loc_gnss_config gnss_config = config->gnss;
	int err;

	if (running) {
		LOG_ERR("Previous operation ongoing.");
		return -EBUSY;
	}

	/* GNSS event handler is already set once in method_gnss_init(). If no other thread is
	 * using GNSS, setting it again is not needed.
	 */
	err = nrf_modem_gnss_event_handler_set(method_gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return -err;
	}

#if defined(CONFIG_NRF_CLOUD_PGPS)
	date_time_update_async(method_gnss_date_time_event_handler);
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* P-GPS is only initialized here because initialization may trigger P-GPS data request
	 * which would fail if the device is not registered to a network.
	 */
	static bool initialized;

	if (!initialized) {
		struct nrf_cloud_pgps_init_param param = {
			.event_handler = method_gnss_pgps_handler,
			.storage_base = PM_MCUBOOT_SECONDARY_ADDRESS,
			.storage_size = PM_MCUBOOT_SECONDARY_SIZE};

		err = nrf_cloud_pgps_init(&param);
		if (err) {
			LOG_ERR("Error from PGPS init: %d", err);
		} else {
			initialized = true;
		}
	}
#endif
#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
	/* Start and stop GNSS just to see if A-GPS data is needed
	 * (triggers event NRF_MODEM_GNSS_EVT_AGPS_REQ)
	 */
	nrf_modem_gnss_start();
	nrf_modem_gnss_stop();
#endif
	k_work_init(&method_gnss_start_work.work_item, method_gnss_positioning_work_fn);
	method_gnss_start_work.gnss_config = gnss_config;
	k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_start_work.work_item);

	running = true;

	return 0;
}

int method_gnss_init(void)
{
	running = false;

	int err = nrf_modem_gnss_init();

	if (err) {
		LOG_ERR("Failed to initialize GNSS interface, error %d", err);
		return -err;
	}

	err = nrf_modem_gnss_event_handler_set(method_gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return -err;
	}

	k_work_init(&method_gnss_fix_work, method_gnss_fix_work_fn);
	k_work_init(&method_gnss_timeout_work, method_gnss_timeout_work_fn);
#if defined(CONFIG_NRF_CLOUD_AGPS)
	k_work_init(&method_gnss_agps_request_work, method_gnss_agps_request_work_fn);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_init(&method_gnss_pgps_request_work, method_gnss_pgps_request_work_fn);
	k_work_init(&method_gnss_manage_pgps_work, method_gnss_manage_pgps);
	k_work_init(&method_gnss_notify_pgps_work, method_gnss_notify_pgps);

#endif
#if !defined(CONFIG_NRF_CLOUD_AGPS) && !defined(CONFIG_NRF_CLOUD_PGPS)
	/* Subscribe to sleep notification to monitor when modem enters power saving mode */
	method_gnss_modem_sleep_notif_subscribe(MIN_SLEEP_DURATION_FOR_STARTING_GNSS);
	lte_lc_register_handler(method_gnss_lte_ind_handler);
#endif
	return 0;
}
