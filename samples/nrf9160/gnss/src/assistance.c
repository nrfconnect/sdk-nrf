/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/modem_info.h>
#include <modem/modem_jwt.h>
#include <net/nrf_cloud_rest.h>
#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_agps.h>
#endif /* CONFIG_NRF_CLOUD_AGPS */
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif /* CONFIG_NRF_CLOUD_PGPS */

#include "assistance.h"

LOG_MODULE_DECLARE(gnss_sample, CONFIG_GNSS_SAMPLE_LOG_LEVEL);

static char jwt_buf[600];
static char rx_buf[2048];

#if defined(CONFIG_NRF_CLOUD_AGPS)
static char agps_data_buf[3500];
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct nrf_modem_gnss_agps_data_frame agps_need;
static struct gps_pgps_request pgps_request;
static struct nrf_cloud_pgps_prediction *prediction;
static struct k_work get_pgps_data_work;
static struct k_work inject_pgps_data_work;
#endif /* CONFIG_NRF_CLOUD_PGPS */

static struct k_work_q *work_q;
static volatile bool assistance_active;

#if defined(CONFIG_NRF_CLOUD_AGPS)
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
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void get_pgps_data_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

	assistance_active = true;

	LOG_INF("Sending request for P-GPS predictions to nRF Cloud...");

	err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));
	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);

		goto exit;
	}

	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = NRF_CLOUD_REST_TIMEOUT_NONE,
		.auth = jwt_buf,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
		.fragment_size = 0, /* Defaults to CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE when 0 */
		.status = 0,
		.response = NULL,
		.response_len = 0,
		.total_response_len = 0
	};

	struct nrf_cloud_rest_pgps_request request = {
		.pgps_req = &pgps_request
	};

	err = nrf_cloud_rest_pgps_data_get(&rest_ctx, &request);
	if (err) {
		LOG_ERR("Failed to send P-GPS request, error: %d", err);

		nrf_cloud_pgps_request_reset();

		goto exit;
	}

	LOG_INF("Processing P-GPS response");

	err = nrf_cloud_pgps_process(rest_ctx.response, rest_ctx.response_len);
	if (err) {
		LOG_ERR("Failed to process P-GPS response, error: %d", err);

		nrf_cloud_pgps_request_reset();

		goto exit;
	}

	LOG_INF("P-GPS response processed");

exit:
	assistance_active = false;
}

static void inject_pgps_data_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

	assistance_active = true;

	LOG_INF("Injecting P-GPS ephemerides");

	err = nrf_cloud_pgps_inject(prediction, &agps_need);
	if (err) {
		LOG_ERR("Failed to inject P-GPS ephemerides");
	}

	err = nrf_cloud_pgps_preemptive_updates();
	if (err) {
		LOG_ERR("Failed to request P-GPS updates");
	}

	assistance_active = false;
}

static void pgps_event_handler(struct nrf_cloud_pgps_event *event)
{
	switch (event->type) {
	case PGPS_EVT_AVAILABLE:
		prediction = event->prediction;

		k_work_submit_to_queue(work_q, &inject_pgps_data_work);
		break;

	case PGPS_EVT_REQUEST:
		memcpy(&pgps_request, event->request, sizeof(pgps_request));

		k_work_submit_to_queue(work_q, &get_pgps_data_work);
		break;

	case PGPS_EVT_LOADING:
		LOG_INF("Loading P-GPS predictions");
		assistance_active = true;
		break;

	case PGPS_EVT_READY:
		LOG_INF("P-GPS predictions ready");
		assistance_active = false;
		break;

	default:
		/* No action needed */
		break;
	}
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

int assistance_init(struct k_work_q *assistance_work_q)
{
	work_q = assistance_work_q;

#if defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_init(&get_pgps_data_work, get_pgps_data_work_fn);
	k_work_init(&inject_pgps_data_work, inject_pgps_data_work_fn);

	struct nrf_cloud_pgps_init_param pgps_param = {
		.event_handler = pgps_event_handler,
		/* storage is defined by CONFIG_NRF_CLOUD_PGPS_STORAGE */
		.storage_base = 0u,
		.storage_size = 0u
	};

	if (nrf_cloud_pgps_init(&pgps_param) != 0) {
		LOG_ERR("Failed to initialize P-GPS");
		return -1;
	}
#endif /* CONFIG_NRF_CLOUD_PGPS */

	return 0;
}

int assistance_request(struct nrf_modem_gnss_agps_data_frame *agps_request)
{
	int err = 0;

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* Store the A-GPS data request for P-GPS use. */
	memcpy(&agps_need, agps_request, sizeof(agps_need));

#if defined(CONFIG_NRF_CLOUD_AGPS)
	if (!agps_request->data_flags) {
		/* No assistance needed from A-GPS, skip directly to P-GPS. */
		nrf_cloud_pgps_notify_prediction();
		return 0;
	}

	/* P-GPS will handle ephemerides, so skip those. */
	agps_request->sv_mask_ephe = 0;
	/* Almanacs are not needed with P-GPS, so skip those. */
	agps_request->sv_mask_alm = 0;
#endif /* CONFIG_NRF_CLOUD_AGPS */
#endif /* CONFIG_NRF_CLOUD_PGPS */
#if defined(CONFIG_NRF_CLOUD_AGPS)
	assistance_active = true;

	err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));
	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		goto agps_exit;
	}

	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = NRF_CLOUD_REST_TIMEOUT_NONE,
		.auth = jwt_buf,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
		.fragment_size = 0, /* Defaults to CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE when 0 */
		.status = 0,
		.response = NULL,
		.response_len = 0,
		.total_response_len = 0
	};

	struct nrf_cloud_rest_agps_request request = {
		.type = NRF_CLOUD_REST_AGPS_REQ_CUSTOM,
		.agps_req = agps_request,
		.net_info = NULL,
#if defined(CONFIG_NRF_CLOUD_AGPS_FILTERED_RUNTIME)
		.filtered = true,
		/* Note: if you change the mask angle here, you may want to
		 * also change it to match in gnss_init_and_start() in main.c.
		 */
		.mask_angle = CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK
		/* Note: if CONFIG_NRF_CLOUD_AGPS_FILTERED is enabled but
		 * CONFIG_NRF_CLOUD_AGPS_FILTERED_RUNTIME is not,
		 * the nrf_cloud_rest library will set the above fields to
		 * true and CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK respectively.
		 * When CONFIG_NRF_CLOUD_AGPS_FILTERED is disabled, it will
		 * set them to false and 0.
		 */
#endif
	};

	struct nrf_cloud_rest_agps_result result = {
		.buf = agps_data_buf,
		.buf_sz = sizeof(agps_data_buf),
		.agps_sz = 0
	};

	struct lte_lc_cells_info net_info = { 0 };

	err = serving_cell_info_get(&net_info.current_cell);
	if (err) {
		LOG_ERR("Could not get cell info, error: %d", err);
	} else {
		/* Network info for the location request. */
		request.net_info = &net_info;
	}

	LOG_INF("Requesting A-GPS data, ephe 0x%08x, alm 0x%08x, flags 0x%02x",
		agps_request->sv_mask_ephe,
		agps_request->sv_mask_alm,
		agps_request->data_flags);

	err = nrf_cloud_rest_agps_data_get(&rest_ctx, &request, &result);
	if (err) {
		LOG_ERR("Failed to get A-GPS data, error: %d", err);
		goto agps_exit;
	}

	LOG_INF("Processing A-GPS data");

	err = nrf_cloud_agps_process(result.buf, result.agps_sz);
	if (err) {
		LOG_ERR("Failed to process A-GPS data, error: %d", err);
		goto agps_exit;
	}

	LOG_INF("A-GPS data processed");

agps_exit:
	assistance_active = false;
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
	nrf_cloud_pgps_notify_prediction();
#endif /* CONFIG_NRF_CLOUD_PGPS */

	return err;
}

bool assistance_is_active(void)
{
	return assistance_active;
}
