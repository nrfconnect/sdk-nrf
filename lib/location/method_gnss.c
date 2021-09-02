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
#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_agps.h>
#endif

#include "loc_core.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/* range 10240-3456000000 ms, see AT command %XMODEMSLEEP */
#define MIN_SLEEP_DURATION_FOR_STARTING_GNSS 10240
#define AT_MDM_SLEEP_NOTIF_START "AT%%XMODEMSLEEP=1,%d,%d"
#define AT_MDM_SLEEP_NOTIF_STOP "AT%XMODEMSLEEP=0" /* not used at the moment */
#define AGPS_REQUEST_RECV_BUF_SIZE 3500
#define AGPS_REQUEST_HTTPS_RESP_HEADER_SIZE 500

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

struct k_work_args {
	struct k_work work_item;
	struct loc_gnss_config gnss_config;
};

static struct k_work_args method_gnss_start_work;
struct k_work method_gnss_fix_work;
struct k_work method_gnss_timeout_work;
struct k_work method_gnss_agps_request_work;

static int fix_attemps_remaining;
static bool first_fix_obtained;
static bool running;


#if defined(CONFIG_NRF_CLOUD_AGPS)
static char rest_api_recv_buf[CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE + AGPS_REQUEST_HTTPS_RESP_HEADER_SIZE];
static char agps_data_buf[AGPS_REQUEST_RECV_BUF_SIZE];
#endif

static K_SEM_DEFINE(entered_psm_mode, 0, 1);

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
	default:
		break;
	}
}

#if defined(CONFIG_NRF_CLOUD_AGPS)
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

	struct nrf_cloud_rest_agps_request request =
		{ NRF_CLOUD_REST_AGPS_REQ_ASSISTANCE, NULL, NULL};

	struct nrf_cloud_rest_agps_result result = {agps_data_buf, sizeof(agps_data_buf), 0};

	nrf_cloud_rest_agps_data_get(&rest_ctx, &request, &result);

	nrf_cloud_agps_process(result.buf, result.agps_sz, NULL);
}
#endif

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
#if defined(CONFIG_NRF_CLOUD_AGPS)
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		LOG_DBG("GNSS: Request A-GPS data");
		k_work_submit_to_queue(loc_core_work_queue_get(), &method_gnss_agps_request_work);
		break;
#endif
	}
}

int method_gnss_cancel(void)
{
	int err = nrf_modem_gnss_stop();

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
	 * sleep state. */
	int sleeping = k_sem_count_get(&entered_psm_mode);
	if (!sleeping) {
		k_sem_reset(&entered_psm_mode);
	}

	fix_attemps_remaining = 0;
	first_fix_obtained = false;

	return -err;
}

static void method_gnss_fix_work_fn(struct k_work *item)
{
	struct nrf_modem_gnss_pvt_data_frame pvt_data;
	static struct loc_location location_result = { 0 };

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		return;
	}

	/* Store fix data only if we get a valid fix. Thus, the last valid data is always kept
	 * in memory and it is not overwritten in case we get an invalid fix. */
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

	/* Start countdown of remaining fix attemps only once we get the first valid fix. */
	if (first_fix_obtained) {
		if (!fix_attemps_remaining) {
			/* We are done, stop GNSS and publish the fix */
			method_gnss_cancel();
			loc_core_event_cb(&location_result);
		} else {
			fix_attemps_remaining--;
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
	int tau;
	int active_time;
	enum lte_lc_system_mode mode;
	struct k_work_args *work_data = CONTAINER_OF(work, struct k_work_args, work_item);
	const struct loc_gnss_config gnss_config = work_data->gnss_config;

	lte_lc_system_mode_get(&mode, NULL);

	/* Don't care about PSM if we are in GNSS only mode */
	if(mode != LTE_LC_SYSTEM_MODE_GPS) {
		int ret = lte_lc_psm_get(&tau, &active_time);
		if (ret < 0) {
			LOG_ERR("Cannot get PSM config: %d. Starting GNSS right away.", ret);
		} else if ((tau >= 0) & (active_time >= 0)) {
			LOG_INF("Waiting for LTE to enter PSM: tau %d active_time %d",
				tau, active_time);

			/* Wait for the PSM to start. If semaphore is reset during the waiting
			 * period, the position request was canceled. Thus, return without
			 * doing anything */
			if (k_sem_take(&entered_psm_mode, K_FOREVER) == -EAGAIN) {
				return;
			}
			k_sem_give(&entered_psm_mode);
		}
	}

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
			fix_attemps_remaining = gnss_config.num_consecutive_fixes;
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

	LOG_DBG("GNSS started");
}

void method_gnss_modem_sleep_notif_subscribe(uint32_t threshold_ms)
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

int method_gnss_location_get(const struct loc_method_config *config)
{
	const struct loc_gnss_config gnss_config = config->gnss;

	if (running) {
		LOG_ERR("Previous operation ongoing.");
		return -EBUSY;
	}

#if defined(CONFIG_NRF_CLOUD_AGPS)
	/* Start and stop GNSS just to see if A-GPS data is needed
	 * (triggers event NRF_MODEM_GNSS_EVT_AGPS_REQ) */
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

	/* Subscribe to sleep notification to monitor when modem enters power saving mode */
	method_gnss_modem_sleep_notif_subscribe(MIN_SLEEP_DURATION_FOR_STARTING_GNSS);

	k_work_init(&method_gnss_fix_work, method_gnss_fix_work_fn);
	k_work_init(&method_gnss_timeout_work, method_gnss_timeout_work_fn);
#if defined(CONFIG_NRF_CLOUD_AGPS)
	k_work_init(&method_gnss_agps_request_work, method_gnss_agps_request_work_fn);
#endif
	lte_lc_register_handler(method_gnss_lte_ind_handler);

	return 0;
}
