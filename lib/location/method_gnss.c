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

/* Max waiting time for modem to enter the PSM in minutes. This prevents Location lib from getting
 * stuck indefinitely if e.g. the application keeps modem constantly awake.
 */
#define PSM_WAIT_BACKSTOP 5
#if !defined(CONFIG_NRF_CLOUD_AGPS) && !defined(CONFIG_NRF_CLOUD_PGPS)
/* range 10240-3456000000 ms, see AT command %XMODEMSLEEP */
#define MIN_SLEEP_DURATION_FOR_STARTING_GNSS 10240
#define AT_MDM_SLEEP_NOTIF_START "AT%%XMODEMSLEEP=1,%d,%d"
#define AT_MDM_SLEEP_NOTIF_STOP "AT%XMODEMSLEEP=0" /* not used at the moment */
#endif
#if (defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS))
#define AGPS_REQUEST_RECV_BUF_SIZE 3500
#define AGPS_REQUEST_HTTPS_RESP_HEADER_SIZE 400
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
static struct k_work method_gnss_pvt_work;

#if defined(CONFIG_NRF_CLOUD_AGPS)
static struct k_work method_gnss_agps_request_work;
#if !defined(CONFIG_NRF_CLOUD_MQTT)
static char agps_data_buf[AGPS_REQUEST_RECV_BUF_SIZE];
#endif
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct k_work method_gnss_pgps_request_work;
static struct k_work method_gnss_manage_pgps_work;
static struct k_work method_gnss_notify_pgps_work;
static struct nrf_cloud_pgps_prediction *prediction;
static struct gps_pgps_request pgps_request;
#endif

static int fixes_remaining;
static bool running;
static K_SEM_DEFINE(entered_psm_mode, 0, 1);

#if (defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS))
static struct nrf_modem_gnss_agps_data_frame agps_request;
#if !defined(CONFIG_NRF_CLOUD_MQTT)
static char rest_api_recv_buf[CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE +
			      AGPS_REQUEST_HTTPS_RESP_HEADER_SIZE];
static char jwt_buf[600];
#endif
#endif

#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
static struct k_work method_gnss_agps_cb_work;
static void method_gnss_agps_cb_work_fn(struct k_work *item);
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_manage_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	LOG_DBG("Sending prediction to modem...");

	err = nrf_cloud_pgps_inject(prediction, &agps_request);

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
	LOG_DBG("P-GPS event type: %d", event->type);

	if (event->type == PGPS_EVT_AVAILABLE) {
		prediction = event->prediction;
		k_work_submit_to_queue(loc_core_work_queue_get(),
				       &method_gnss_manage_pgps_work);
	} else if (event->type == PGPS_EVT_REQUEST) {
		memcpy(&pgps_request, event->request, sizeof(pgps_request));
		k_work_submit_to_queue(loc_core_work_queue_get(),
				       &method_gnss_pgps_request_work);
	}
}

static void method_gnss_notify_pgps(struct k_work *work)
{
	ARG_UNUSED(work);
	int err = nrf_cloud_pgps_notify_prediction();

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
		/* If PSM becomes disabled e.g. due to network change, allow GNSS to be started
		 * in case there was a pending position request waiting for the sleep to start. If
		 * PSM becomes enabled, block GNSS until the modem enters PSM by taking the
		 * semaphore.
		 */
		if (evt->psm_cfg.active_time == -1) {
			k_sem_give(&entered_psm_mode);
		} else if (evt->psm_cfg.active_time > 0) {
			k_sem_take(&entered_psm_mode, K_NO_WAIT);
		}
		break;
	default:
		break;
	}
}
#endif // !CONFIG_NRF_CLOUD_AGPS && !CONFIG_NRF_CLOUD_PGPS

#if defined(CONFIG_NRF_CLOUD_AGPS)
#if defined(CONFIG_NRF_CLOUD_MQTT)
static void method_gnss_agps_request_work_fn(struct k_work *item)
{
	int err = nrf_cloud_agps_request(&agps_request);
	if (err) {
		LOG_ERR("nRF Cloud A-GPS request failed, error: %d", err);
		return;
	}

	LOG_DBG("A-GPS data requested");
}

#else // defined(CONFIG_NRF_CLOUD_MQTT)
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

	/* Request for MODEM_INFO_OPERATOR returns both MNC and MCC in the same string. */
	err = modem_info_string_get(MODEM_INFO_OPERATOR,
				    resp_buf_cops,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
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
		.auth = jwt_buf,
		.rx_buf = rest_api_recv_buf,
		.rx_buf_len = sizeof(rest_api_recv_buf),
		.fragment_size = 0
	};

	int err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));

	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		return;
	}

	struct nrf_cloud_rest_agps_request request = {
						      NRF_CLOUD_REST_AGPS_REQ_CUSTOM,
						      &agps_request,
						      NULL};
	struct lte_lc_cells_info net_info = {0};

	/* Get network info for the A-GPS location request. */
	err = method_gnss_get_modem_info(&net_info.current_cell);

	if (err) {
		LOG_WRN("Requesting A-GPS data without location assistance");
	} else {
		request.net_info = &net_info;
	}

	struct nrf_cloud_rest_agps_result result = {agps_data_buf, sizeof(agps_data_buf), 0};

	err = nrf_cloud_rest_agps_data_get(&rest_ctx, &request, &result);
	if (err) {
		LOG_ERR("nRF Cloud A-GPS request failed, error: %d", err);
		return;
	}

	LOG_DBG("A-GPS data requested");

	err = nrf_cloud_agps_process(result.buf, result.agps_sz);
	if (err) {
		LOG_ERR("A-GPS data processing failed, error: %d", err);
		return;
	}

	LOG_DBG("A-GPS data processed");
}
#endif // defined(CONFIG_NRF_CLOUD_MQTT)
#endif // defined(CONFIG_NRF_CLOUD_AGPS)

#if defined(CONFIG_NRF_CLOUD_PGPS) && !defined(CONFIG_NRF_CLOUD_MQTT)
static void method_gnss_pgps_request_work_fn(struct k_work *item)
{
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = CONFIG_NRF_CLOUD_REST_RECV_TIMEOUT * MSEC_PER_SEC,
		.auth = jwt_buf,
		.rx_buf = rest_api_recv_buf,
		.rx_buf_len = sizeof(rest_api_recv_buf),
		.fragment_size = 0
	};

	int err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));

	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		return;
	}

	struct nrf_cloud_rest_pgps_request request = {
		.pgps_req = &pgps_request
	};

	err = nrf_cloud_rest_pgps_data_get(&rest_ctx, &request);
	if (err) {
		nrf_cloud_pgps_request_reset();
		LOG_ERR("nRF Cloud P-GPS request failed, error: %d", err);
		return;
	}

	LOG_DBG("P-GPS data requested");

	err = nrf_cloud_pgps_process(rest_ctx.response, rest_ctx.response_len);
	if (err) {
		nrf_cloud_pgps_request_reset();
		LOG_ERR("P-GPS data processing failed, error: %d", err);
		return;
	}

	LOG_DBG("P-GPS data processed");
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
		LOG_DBG("No A-GPS data types requested");
		return false;
	} else {
		return true;
	}
}

#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_request_assistance(void)
{
	int err = nrf_modem_gnss_read(&agps_request,
				      sizeof(agps_request),
				      NRF_MODEM_GNSS_DATA_AGPS_REQ);

	if (err) {
		LOG_WRN("Reading A-GPS req data from GNSS failed, error: %d", err);
		return;
	}

	LOG_DBG("A-GPS request from modem (ephe: 0x%08x alm: 0x%08x flags: 0x%02x)",
		agps_request.sv_mask_ephe,
		agps_request.sv_mask_alm,
		agps_request.data_flags);
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
	k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_agps_cb_work);
#else
#if defined(CONFIG_NRF_CLOUD_AGPS)
	/* Check the request. If no A-GPS data types except ephemeris or almanac are requested,
	 * jump to P-GPS (if enabled)
	 */
	if (method_gnss_agps_required(&agps_request)) {
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_agps_request_work);
	} else
#endif
	{
#if defined(CONFIG_NRF_CLOUD_PGPS)
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_notify_pgps_work);
#endif
	}
#endif // defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
}
#endif // defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)

void method_gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_pvt_work);
		break;

	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
		method_gnss_request_assistance();
#endif
		break;
	}
}

int method_gnss_cancel(void)
{
	int err = nrf_modem_gnss_stop();
	int sleeping;

	if (err == NRF_EPERM) {
		/* Let's convert NRF_EPERM to EPERM to make sure loc_core works properly */
		err = EPERM;
		/* NRF_EPERM is logged in loc_core */
	} else if (err) {
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
			return true;
		}

		LOG_DBG("LTE active time: %d seconds", active_time);

		if (active_time >= 0) {
			LOG_DBG("Waiting for LTE to enter PSM");

			/* Wait for the PSM to start. If semaphore is reset during the waiting
			 * period, the position request was canceled.
			 */
			if (k_sem_take(&entered_psm_mode, K_MINUTES(PSM_WAIT_BACKSTOP))
			    == -EAGAIN) {
				if (!running) { /* Request was cancelled */
					return false;
				}
				/* We're still running, i.e., the wait for PSM timed out */
				LOG_ERR("PSM is configured, but modem did not enter PSM ");
				LOG_ERR("in %d minutes. Starting GNSS anyway.",
					PSM_WAIT_BACKSTOP);
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

static void method_gnss_print_pvt(const struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked = 0;

	/* Print number of tracked satellites */
	for (uint32_t i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (pvt_data->sv[i].sv == 0) {
			break;
		}

		tracked++;
	}

	LOG_DBG("Tracked satellites: %d, fix valid: %s",
		tracked,
		pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID ? "true" : "false");

	/* Print details for each satellite */
	for (uint32_t i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (pvt_data->sv[i].sv == 0) {
			break;
		}

		const struct nrf_modem_gnss_sv *sv_data = &pvt_data->sv[i];

		LOG_DBG("PRN: %3d, C/N0: %4.1f, in fix: %d, unhealthy: %d",
			sv_data->sv,
			sv_data->cn0 / 10.0,
			sv_data->flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX ? 1 : 0,
			sv_data->flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY ? 1 : 0);
	}
}

static void method_gnss_pvt_work_fn(struct k_work *item)
{
	struct nrf_modem_gnss_pvt_data_frame pvt_data;
	static struct loc_location location_result = { 0 };

	if (!running) {
		/* Cancel has already been called, so ignore the notification. */
		return;
	}

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		return;
	}

	method_gnss_print_pvt(&pvt_data);

	/* Store fix data only if we get a valid fix. Thus, the last valid data is always kept
	 * in memory and it is not overwritten in case we get an invalid fix.
	 */
	if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		fixes_remaining--;

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

	if (fixes_remaining <= 0) {
		/* We are done, stop GNSS and publish the fix. */
		method_gnss_cancel();
		loc_core_event_cb(&location_result);
	}
}
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
static void method_gnss_agps_cb_work_fn(struct k_work *item)
{
	loc_core_event_cb_assistance_request(&agps_request);
}
#endif

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

	/* By default we take the first fix. */
	fixes_remaining = 1;

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

		/* In high accuracy mode, use the configured fix count. */
		fixes_remaining = gnss_config.num_consecutive_fixes;
		break;
	}

	if (err) {
		LOG_ERR("Failed to configure GNSS");
		loc_core_event_cb_error();
		running = false;
		return;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS");
		loc_core_event_cb_error();
		running = false;
		return;
	}

	loc_core_timer_start(gnss_config.timeout);
}

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
		LOG_DBG("DATE_TIME_OBTAINED_MODEM");
		break;
	case DATE_TIME_OBTAINED_NTP:
		LOG_DBG("DATE_TIME_OBTAINED_NTP");
		break;
	case DATE_TIME_OBTAINED_EXT:
		LOG_DBG("DATE_TIME_OBTAINED_EXT");
		break;
	case DATE_TIME_NOT_OBTAINED:
		LOG_DBG("DATE_TIME_NOT_OBTAINED");
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
	if (!date_time_is_valid()) {
		date_time_update_async(method_gnss_date_time_event_handler);
	}
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

#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
	k_work_init(&method_gnss_agps_cb_work, method_gnss_agps_cb_work_fn);
#endif
	k_work_init(&method_gnss_pvt_work, method_gnss_pvt_work_fn);
#if defined(CONFIG_NRF_CLOUD_AGPS)
	k_work_init(&method_gnss_agps_request_work, method_gnss_agps_request_work_fn);
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#if !defined(CONFIG_NRF_CLOUD_MQTT)
	k_work_init(&method_gnss_pgps_request_work, method_gnss_pgps_request_work_fn);
#endif
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
