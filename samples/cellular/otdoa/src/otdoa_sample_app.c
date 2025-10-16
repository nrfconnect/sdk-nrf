/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <otdoa_al/otdoa_api.h>
#include <otdoa_al/otdoa_otdoa2al_api.h>
#include "otdoa_gpio.h"
#include "otdoa_sample_app.h"

#define UBSA_FILE_PATH "/lfs/ubsa.csv"

LOG_MODULE_DECLARE(otdoa_sample, LOG_LEVEL_INF);

static K_SEM_DEFINE(lte_connected, 0, 1);

/* Timer for periodic position estimates */
struct k_timer pos_est_timer;
#define POS_EST_TIMER_DURATION K_MSEC(CONFIG_PERIODIC_EST_INTERVAL * 60 * 1000)

/* Flag to distinguish between DL requests initiated during an OTDOA
 * session, and those that are not part of a session (e.g. those for testing uBDA DL)
 */
static int dl_in_session;

static const char cert[] = {
#include "hellaphy.pem.inc"
};
BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

/* forward references */
static void otdoa_event_handler(const otdoa_api_event_data_t *p_event_data);
static void pos_est_timer_cb(struct k_timer *timer);
static void pos_est_timer_restart(void);
void otdoa_sample_start(uint32_t session_length, uint32_t capture_flags, uint32_t timeout_msec);

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_NW_REG_STATUS) {
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
			(evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			printk("Connected to LTE");
			k_sem_give(&lte_connected);
		}
	}
}

int otdoa_sample_main(void)
{
	LOG_INF("OTDOA sample started");

	int err = nrf_modem_lib_init();

	if (err) {
		LOG_ERR("Modem library initialization failed, error: %d", err);
		return err;
	}
	LOG_INF("Modem version = %s", nrf_modem_build_version());

	otdoa_api_cfg_set_file_path("/lfs/config");

	/* Don't include null terminator in PEM file length */
	err = otdoa_api_install_tls_cert(cert, sizeof(cert) - 1);
	if (err != 0) {
		LOG_ERR("otdoa_api_install_tls_cert() failed with return %d", err);
		return err;
	}

	/* AL and OTDOA library can use the same callback */
	err = otdoa_api_init(UBSA_FILE_PATH, otdoa_event_handler);
	if (err != 0) {
		LOG_ERR("otdoa_api_init() failed with return %d", err);
		return err;
	}

	LOG_INF("Connecting to LTE...");

	lte_lc_register_handler(lte_event_handler);

	lte_lc_connect();

	k_timer_init(&pos_est_timer, pos_est_timer_cb, NULL);

	k_sem_take(&lte_connected, K_FOREVER);

	LOG_INF("Connected!");

	set_blink_sleep();

	/* Optionally enable periodic estimates */
	pos_est_timer_restart();

	return 0;
}

static void display_result_details(const otdoa_api_result_details_t *p_details)
{
	if (!p_details) {
		return;
	}

	LOG_INF("  serv. ecgi: %u", p_details->serving_cell_ecgi);
	LOG_INF("  rssi: %d", p_details->serving_rssi_dbm);
	LOG_INF("  dlearfcn: %u", p_details->dlearfcn);
	LOG_INF("  length: %u", p_details->session_length);

	LOG_INF("  est. algorithm: %s", p_details->estimate_algorithm);
	LOG_INF("  measured cells: %u", p_details->num_measured_cells);

	LOG_INF("  Detected Cells:");
	for (unsigned int u = 0; u < p_details->num_measured_cells; u++) {
		if (p_details->toa_detect_count[u] > 0) {
			LOG_INF("    ecgi: %u  detects: %u", p_details->ecgi_list[u],
				p_details->toa_detect_count[u]);
		}
	}
}

static void otdoa_event_handler(const otdoa_api_event_data_t *p_event_data)
{
	switch (p_event_data->event) {
	case OTDOA_EVENT_RESULTS: {
		LOG_INF("OTDOA_EVENT_RESULTS:");
		LOG_INF("  latitude: %.06f", p_event_data->results.latitude);
		LOG_INF("  longitude: %.06f", p_event_data->results.longitude);
		LOG_INF("  accuracy: %.01f m", (double)(p_event_data->results.accuracy));
		display_result_details(&p_event_data->results.details);
		LOG_INF("OTDOA position estimate SUCCESS");
		set_blink_sleep();
		pos_est_timer_restart();
		break;
	}

	case OTDOA_EVENT_FAIL: {
		LOG_ERR("OTDOA_EVENT_FAIL:  failure code = %d", p_event_data->failure_code);
		set_blink_error();
		pos_est_timer_restart();
		break;
	}

	case OTDOA_EVENT_UBSA_DL_REQ: {
		/* OTDOA Library requests download of a new uBSA file */
		dl_in_session = 1;
		set_blink_download();
		/* Call the API to start the DL */
		LOG_INF("OTDOA_EVENT_UBSA_DL_REQ:");
		LOG_INF("  ecgi: %u  dlearfcn: %u", p_event_data->dl_request.ecgi,
			p_event_data->dl_request.dlearfcn);
		LOG_INF("  max cells: %u  radius: %u", p_event_data->dl_request.max_cells,
			p_event_data->dl_request.ubsa_radius_meters);

		int err = otdoa_api_ubsa_download(&p_event_data->dl_request, false);

		if (err != OTDOA_API_SUCCESS) {
			LOG_ERR("otdoa_api_ubsa_download() failed with return %d", err);
			err = otdoa_api_cancel_session();
			if (err != OTDOA_API_SUCCESS) {
				LOG_ERR("otdoa_api_cancel_session() failed with return %d", err);
			}
			set_blink_error();
		}
		pos_est_timer_restart();
		break;
	}

	case OTDOA_EVENT_UBSA_DL_COMPL: {
		LOG_INF("OTDOA_EVENT_UBSA_DL_COMPL:");
		LOG_INF("  status: %d", p_event_data->dl_compl.status);

		/* If DL request was done as part of a session, indicate to the OTDOA library
		 * that the updated uBSA file is now available.
		 */
		if (dl_in_session) {
			dl_in_session = 0;
			set_blink_prs();
			int err = otdoa_api_ubsa_available(p_event_data->dl_compl.status);

			if (err != OTDOA_API_SUCCESS) {
				LOG_ERR("otdoa_api_ubsa_available() failed with return %d", err);
				err = otdoa_api_cancel_session();
				if (err != OTDOA_API_SUCCESS) {
					LOG_ERR("otdoa_api_cancel_session() failed with return %d",
						err);
				}
			}
		} else {
			/* DL request was not part of a session, so just handle the result */
			if (p_event_data->dl_compl.status == OTDOA_API_SUCCESS) {
				set_blink_sleep();
				LOG_INF("OTDOA uBSA DL SUCCESS");
			} else {
				LOG_ERR("OTDOA uBSA DL FAILED");
				set_blink_error();
			}
			pos_est_timer_restart();
		}
		break;
	}

	default: {
		LOG_INF("Unhandle event %d in otdoa_event_handler()", p_event_data->event);
		break;
	}
	}
}

/* callback for periodic position estimates timer */
static void pos_est_timer_cb(struct k_timer *timer)
{
	uint32_t timeout_msec = OTDOA_SAMPLE_DEFAULT_TIMEOUT_MS;

	otdoa_sample_start(32, 0, timeout_msec);
}

static void pos_est_timer_restart(void)
{
#if CONFIG_PERIODIC_EST_INTERVAL > 0
	k_timer_start(&pos_est_timer, POS_EST_TIMER_DURATION, K_NO_WAIT);
#endif
}

static void pos_est_timer_stop(void)
{
	k_timer_stop(&pos_est_timer);
}

/*
 * API functions called from shell
 */
void otdoa_sample_start(uint32_t session_length, uint32_t capture_flags, uint32_t timeout_msec)
{
	otdoa_api_session_params_t params = {0};

	params.session_length = session_length;
	params.capture_flags = capture_flags;
	params.timeout = timeout_msec;

	dl_in_session = 0;

	int err = otdoa_api_start_session(&params);

	if (err != OTDOA_API_SUCCESS) {
		set_blink_error();
		LOG_ERR("otdoa_api_start_session() failed with return %d", err);
	} else {
		set_blink_prs();
		pos_est_timer_stop();
	}
}

void otdoa_sample_cancel(void)
{
	int err = otdoa_api_cancel_session();

	if (err != OTDOA_API_SUCCESS) {
		LOG_ERR("otdoa_api_cancel_session() failed with return %d", err);
	}
	set_blink_sleep();
	pos_est_timer_stop();
}

void otdoa_sample_ubsa_dl_test(uint32_t ecgi, uint32_t dlearfcn, uint32_t radius,
			       uint32_t max_cells, uint16_t mcc, uint16_t mnc)
{
	otdoa_api_ubsa_dl_req_t dl_req = {0};

	dl_req.ecgi = ecgi;
	dl_req.dlearfcn = dlearfcn;
	dl_req.ubsa_radius_meters = radius;
	dl_req.max_cells = max_cells;
	dl_req.mcc = mcc;
	dl_req.mnc = mnc;
	set_blink_download();
	LOG_INF("Getting uBSA (ECGI: %u (0x%08x) DLEARFCN: %u  Radius: %u  Num Cells: %u)", ecgi,
		ecgi, dlearfcn, radius, max_cells);

	int err = otdoa_api_ubsa_download(&dl_req, true);

	if (err != OTDOA_API_SUCCESS) {
		LOG_ERR("otdoa_api_ubsa_download() failed with return %d", err);
		set_blink_error();
	} else {
		set_blink_download();
	}
	pos_est_timer_stop();
}
