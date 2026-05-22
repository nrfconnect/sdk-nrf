/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/modem_info.h>
#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include <net/nrf_cloud_agnss.h>
#endif /* CONFIG_NRF_CLOUD_AGNSS */
#if defined(CONFIG_NRF_CLOUD_PGNSS)
#include <net/nrf_cloud_pgnss.h>
#endif /* CONFIG_NRF_CLOUD_PGNSS */
#include <net/nrf_cloud_coap.h>

#include "assistance.h"

LOG_MODULE_DECLARE(gnss_sample, CONFIG_GNSS_SAMPLE_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static char agnss_data_buf[NRF_CLOUD_AGNSS_MAX_DATA_SIZE];
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGNSS)
static struct nrf_modem_gnss_agnss_data_frame agnss_need;
static struct gps_pgnss_request pgnss_request;
static struct nrf_cloud_pgnss_prediction *prediction;
static struct k_work get_pgnss_data_work;
static struct k_work inject_pgnss_data_work;
#endif /* CONFIG_NRF_CLOUD_PGNSS */

static struct k_work_q *work_q;
static volatile bool assistance_active;
static bool coap_connected;

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static int serving_cell_info_get(struct lte_lc_cell *serving_cell)
{
	int err;

	err = modem_info_init();
	if (err) {
		return err;
	}

	char resp_buf[MODEM_INFO_MAX_RESPONSE_SIZE];

	err = modem_info_string_get(MODEM_INFO_CELLID,
				    resp_buf,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		return err;
	}

	serving_cell->id = strtol(resp_buf, NULL, 16);

	err = modem_info_string_get(MODEM_INFO_AREA_CODE,
				    resp_buf,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		return err;
	}

	serving_cell->tac = strtol(resp_buf, NULL, 16);

	/* Request for MODEM_INFO_MNC returns both MNC and MCC in the same string. */
	err = modem_info_string_get(MODEM_INFO_OPERATOR,
				    resp_buf,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		return err;
	}

	serving_cell->mnc = strtol(&resp_buf[3], NULL, 10);
	/* Null-terminate MCC, read and store it. */
	resp_buf[3] = '\0';
	serving_cell->mcc = strtol(resp_buf, NULL, 10);

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGNSS)
static void get_pgnss_data_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

	assistance_active = true;

	LOG_INF("Sending request for PGNSS predictions to nRF Cloud...");

	struct nrf_cloud_coap_pgnss_request request = {
		.pgnss_req = &pgnss_request
	};

	struct nrf_cloud_pgnss_result file_location = {0};

	static char host[64];
	static char path[128];

	memset(host, 0, sizeof(host));
	memset(path, 0, sizeof(path));

	file_location.host = host;
	file_location.host_sz = sizeof(host);
	file_location.path = path;
	file_location.path_sz = sizeof(path);

	err = nrf_cloud_coap_pgnss_url_get(&request, &file_location);
	if (err) {
		LOG_ERR("GNSS: Failed to get PGNSS data, error: %d", err);

		nrf_cloud_pgnss_request_reset();

		goto exit;
	}

	LOG_INF("Processing PGNSS response");

	err = nrf_cloud_pgnss_update(&file_location);
	if (err) {
		LOG_ERR("Failed to process PGNSS response, error: %d", err);

		nrf_cloud_pgnss_request_reset();

		goto exit;
	}

	LOG_INF("PGNSS response processed");

	err = nrf_cloud_pgnss_notify_prediction();
	if (err) {
		LOG_ERR("Failed to request current prediction, error: %d", err);

		goto exit;
	}

	LOG_INF("PGNSS predictions requested");
exit:
	assistance_active = false;
}

static void inject_pgnss_data_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

	assistance_active = true;

	LOG_INF("Injecting PGNSS ephemerides");

	err = nrf_cloud_pgnss_inject(prediction, &agnss_need);
	if (err) {
		LOG_ERR("Failed to inject PGNSS ephemerides");
	}

	err = nrf_cloud_pgnss_preemptive_updates();
	if (err) {
		LOG_ERR("Failed to request PGNSS updates");
	}

	assistance_active = false;
}

static void pgnss_event_handler(struct nrf_cloud_pgnss_event *event)
{
	switch (event->type) {
	case PGNSS_EVT_AVAILABLE:
		prediction = event->prediction;

		k_work_submit_to_queue(work_q, &inject_pgnss_data_work);
		break;

	case PGNSS_EVT_REQUEST:
		memcpy(&pgnss_request, event->request, sizeof(pgnss_request));

		k_work_submit_to_queue(work_q, &get_pgnss_data_work);
		break;

	case PGNSS_EVT_LOADING:
		LOG_INF("Loading PGNSS predictions");
		assistance_active = true;
		break;

	case PGNSS_EVT_READY:
		LOG_INF("PGNSS predictions ready");
		assistance_active = false;
		break;

	default:
		/* No action needed */
		break;
	}
}
#endif /* CONFIG_NRF_CLOUD_PGNSS */

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static const char *get_system_string(uint8_t system_id)
{
	switch (system_id) {
	case NRF_MODEM_GNSS_SYSTEM_INVALID:
		return "invalid";

	case NRF_MODEM_GNSS_SYSTEM_GPS:
		return "GPS";

	case NRF_MODEM_GNSS_SYSTEM_QZSS:
		return "QZSS";

	default:
		return "unknown";
	}
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

int assistance_init(struct k_work_q *assistance_work_q)
{
	work_q = assistance_work_q;

	int err = nrf_cloud_coap_init();

	if (err) {
		LOG_ERR("Failed to initialize nRF Cloud CoAP client: %d", err);
		return err;
	}

#if defined(CONFIG_NRF_CLOUD_PGNSS)
	k_work_init(&get_pgnss_data_work, get_pgnss_data_work_fn);
	k_work_init(&inject_pgnss_data_work, inject_pgnss_data_work_fn);

	struct nrf_cloud_pgnss_init_param pgnss_param = {
		.event_handler = pgnss_event_handler,
		/* storage is defined by CONFIG_NRF_CLOUD_PGNSS_STORAGE */
		.storage_base = 0u,
		.storage_size = 0u
	};

	if (nrf_cloud_pgnss_init(&pgnss_param) != 0) {
		LOG_ERR("Failed to initialize PGNSS");
		return -1;
	}
#endif /* CONFIG_NRF_CLOUD_PGNSS */

	return 0;
}

int assistance_request(struct nrf_modem_gnss_agnss_data_frame *agnss_request)
{
	int err = 0;

	if (!coap_connected) {
		err = nrf_cloud_coap_connect(NULL);
		if (err) {
			LOG_ERR("Connecting to nRF Cloud failed: %d", err);
			return err;
		}
		coap_connected = true;
	}

#if defined(CONFIG_NRF_CLOUD_PGNSS)
	/* Store the A-GNSS data request for PGNSS use. */
	memcpy(&agnss_need, agnss_request, sizeof(agnss_need));

#if defined(CONFIG_NRF_CLOUD_AGNSS)
	if (!agnss_request->data_flags) {
		/* No assistance needed from A-GNSS, skip directly to PGNSS. */
		nrf_cloud_pgnss_notify_prediction();
		return 0;
	}

	/* PGNSS will handle GPS ephemerides, so skip those. */
	agnss_request->system[0].sv_mask_ephe = 0;
	/* GPS almanacs are not needed with PGNSS, so skip those. */
	agnss_request->system[0].sv_mask_alm = 0;
#endif /* CONFIG_NRF_CLOUD_AGNSS */
#endif /* CONFIG_NRF_CLOUD_PGNSS */
#if defined(CONFIG_NRF_CLOUD_AGNSS)
	assistance_active = true;

	struct nrf_cloud_coap_agnss_request request = {
		NRF_CLOUD_COAP_AGNSS_REQ_CUSTOM,
		agnss_request,
		NULL,
#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED_RUNTIME)
		true,
		/* Note: if you change the mask angle here, you may want to
		 * also change it to match in gnss_init_and_start() in main.c.
		 */
		CONFIG_NRF_CLOUD_AGNSS_ELEVATION_MASK
#endif
	};

	struct nrf_cloud_coap_agnss_result result = {
		agnss_data_buf,
		sizeof(agnss_data_buf),
		0
	};

	struct lte_lc_cells_info net_info = { 0 };

	err = serving_cell_info_get(&net_info.current_cell);
	if (err) {
		LOG_ERR("Could not get cell info, error: %d", err);
	} else {
		/* Network info for the location request. */
		request.net_info = &net_info;
	}

	LOG_INF("Requesting A-GNSS data: data_flags: 0x%02x", agnss_request->data_flags);
	for (int i = 0; i < agnss_request->system_count; i++) {
		LOG_INF("Requesting A-GNSS data: %s ephe: 0x%llx, alm: 0x%llx",
			get_system_string(agnss_request->system[i].system_id),
			agnss_request->system[i].sv_mask_ephe,
			agnss_request->system[i].sv_mask_alm);
	}

	err = nrf_cloud_coap_agnss_data_get(&request, &result);
	if (err) {
		LOG_ERR("Failed to get A-GNSS data, error: %d", err);
		goto agnss_exit;
	}

	LOG_INF("Processing A-GNSS data");

	err = nrf_cloud_agnss_process(result.buf, result.agnss_sz);
	if (err) {
		LOG_ERR("Failed to process A-GNSS data, error: %d", err);
		goto agnss_exit;
	}

	LOG_INF("A-GNSS data processed");

agnss_exit:
	assistance_active = false;
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGNSS)
	nrf_cloud_pgnss_notify_prediction();
#endif /* CONFIG_NRF_CLOUD_PGNSS */

	return err;
}

bool assistance_is_active(void)
{
	return assistance_active;
}
