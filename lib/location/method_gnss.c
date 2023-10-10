/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <nrf_errno.h>
#include "location_core.h"
#include "location_utils.h"
#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include "scan_cellular.h"
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_agnss.h>
#include <stdlib.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_pgps.h>
#endif
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
/* Verify that MQTT, REST, CoAP or external service is enabled */
BUILD_ASSERT(
	IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) ||
	IS_ENABLED(CONFIG_NRF_CLOUD_REST) ||
	IS_ENABLED(CONFIG_NRF_CLOUD_COAP) ||
	IS_ENABLED(CONFIG_LOCATION_SERVICE_EXTERNAL),
	"CONFIG_NRF_CLOUD_MQTT, CONFIG_NRF_CLOUD_REST, CONFIG_NRF_CLOUD_COAP or "
	"CONFIG_LOCATION_SERVICE_EXTERNAL must be enabled");
#endif

/* Maximum waiting time before GNSS is started regardless of RRC or PSM state [min]. This prevents
 * Location library from getting stuck indefinitely if the application keeps LTE connection
 * constantly active.
 */
#define SLEEP_WAIT_BACKSTOP 5
#if !defined(CONFIG_NRF_CLOUD_AGNSS)
/* range 10240-3456000000 ms, see AT command %XMODEMSLEEP */
#define MIN_SLEEP_DURATION_FOR_STARTING_GNSS 10240
#define AT_MDM_SLEEP_NOTIF_START "AT%%XMODEMSLEEP=1,%d,%d"
#endif
#if (defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS))
#define AGNSS_REQUEST_RECV_BUF_SIZE 3500
#define AGNSS_REQUEST_HTTPS_RESP_HEADER_SIZE 400
/* Minimum time between two A-GNSS data requests in seconds. */
#define AGNSS_REQUEST_MIN_INTERVAL (60 * 60)
/* A-GNSS data expiration threshold in minutes before requesting fresh data. An 80 minute threshold
 * is used because it leaves enough time to try again after an hour if fetching of A-GNSS data fails
 * once. Also, because of overlapping ephemeris validity times, fresh ephemerides are
 * needed on average every two hours with both 80 minute and shorter expiration thresholds.
 */
#define AGNSS_EXPIRY_THRESHOLD (80)
/* P-GPS data expiration threshold is zero minutes, because there is no overlap between
 * predictions.
 */
#define PGPS_EXPIRY_THRESHOLD 0
/* A-GNSS minimum number of expired ephemerides to request all ephemerides. */
#define AGNSS_EPHE_MIN_COUNT 3
/* P-GPS minimum number of expired ephemerides to trigger injection of predictions.
 * With P-GPS, predictions are not available for all satellites, especially when
 * predictions are made further into the future, so it is natural that predictions are
 * missing for some of the satellites.
 */
#define PGPS_EPHE_MIN_COUNT 12
/* A-GNSS minimum number of expired almanacs to request all almanacs. */
#define AGNSS_ALM_MIN_COUNT 3
/* PRN of the first QZSS satellite. The range for QZSS PRNs is 193...202. */
#define FIRST_QZSS_PRN 193
#endif

#define VISIBILITY_DETECTION_EXEC_TIME CONFIG_LOCATION_METHOD_GNSS_VISIBILITY_DETECTION_EXEC_TIME
#define VISIBILITY_DETECTION_SAT_LIMIT CONFIG_LOCATION_METHOD_GNSS_VISIBILITY_DETECTION_SAT_LIMIT

static struct k_work method_gnss_start_work;
static struct k_work method_gnss_pvt_work;
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
/* Flag indicating whether nrf_modem_gnss_agnss_expiry_get() is supported. If the API is
 * supported, NRF_MODEM_GNSS_EVT_AGNSS_REQ events are ignored.
 */
static bool agnss_expiry_get_supported;
static struct k_work method_gnss_agnss_req_event_handle_work;
static struct k_work method_gnss_agnss_req_work;
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static int64_t agnss_req_timestamp;
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && \
	(defined(CONFIG_NRF_CLOUD_REST) || defined(CONFIG_NRF_CLOUD_COAP)) && \
	!defined(CONFIG_NRF_CLOUD_MQTT)
static char agnss_rest_data_buf[AGNSS_REQUEST_RECV_BUF_SIZE];
#endif
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
static struct k_work method_gnss_pgps_request_work;
#endif
static struct k_work method_gnss_inject_pgps_work;
static struct k_work method_gnss_notify_pgps_work;
static struct nrf_cloud_pgps_prediction *prediction;
static struct gps_pgps_request pgps_request;
#endif

static bool running;
static struct location_gnss_config gnss_config;
static K_SEM_DEFINE(entered_psm_mode, 0, 1);
static K_SEM_DEFINE(entered_rrc_idle, 1, 1);

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
static struct nrf_modem_gnss_agnss_data_frame agnss_request;
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static struct nrf_modem_gnss_agnss_data_frame pgps_agnss_request = {
	/* Inject current time by default. */
	.data_flags = NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST,
	.system_count = 1,
	/* Ephe mask for GPS is initially all set, because event PGPS_EVT_AVAILABLE may be received
	 * before the assistance request from GNSS. If ephe mask would be zero, no predictions
	 * would be injected.
	 */
	.system[0].system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
	.system[0].sv_mask_ephe = 0xffffffff,
	.system[0].sv_mask_alm = 0x00000000,
};
#endif

#if defined(CONFIG_NRF_CLOUD_REST) && !defined(CONFIG_NRF_CLOUD_MQTT)
#if (defined(CONFIG_NRF_CLOUD_AGNSS) && !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)) || \
	(defined(CONFIG_NRF_CLOUD_PGPS) && !defined(CONFIG_LOCATION_SERVICE_EXTERNAL))
static char rest_api_recv_buf[CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE +
			      AGNSS_REQUEST_HTTPS_RESP_HEADER_SIZE];
#endif
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_PGPS)
static struct k_work method_gnss_pgps_ext_work;
static void method_gnss_pgps_ext_work_fn(struct k_work *item);
#endif

/* Count of consecutive NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME flags in PVT data. Used for
 * triggering GNSS priority mode if enabled.
 */
static int insuf_timewin_count;
static int fixes_remaining;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
static struct location_data_details_gnss location_data_details_gnss;
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_inject_pgps_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;

	LOG_DBG("Sending prediction to modem (ephe: 0x%08x)...",
		(uint32_t)pgps_agnss_request.system[0].sv_mask_ephe);

	err = nrf_cloud_pgps_inject(prediction, &pgps_agnss_request);
	if (err) {
		LOG_ERR("Unable to send prediction to modem: %d", err);
	}
}

void method_gnss_pgps_handler(struct nrf_cloud_pgps_event *event)
{
	LOG_DBG("P-GPS event type: %d", event->type);

	if (event->type == PGPS_EVT_READY) {
		/* P-GPS has finished downloading predictions; request the current prediction. */
		k_work_submit_to_queue(location_core_work_queue_get(),
				       &method_gnss_notify_pgps_work);
	} else if (event->type == PGPS_EVT_AVAILABLE) {
		/* Inject the specified prediction into the modem. */
		prediction = event->prediction;
		k_work_submit_to_queue(location_core_work_queue_get(),
				       &method_gnss_inject_pgps_work);
	} else if (event->type == PGPS_EVT_REQUEST) {
		memcpy(&pgps_request, event->request, sizeof(pgps_request));
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
		k_work_submit_to_queue(location_core_work_queue_get(), &method_gnss_pgps_ext_work);
#else
		k_work_submit_to_queue(location_core_work_queue_get(),
				       &method_gnss_pgps_request_work);
#endif
	}
}

static void method_gnss_notify_pgps_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	int err = nrf_cloud_pgps_notify_prediction();

	if (err) {
		LOG_ERR("Error requesting notification of prediction availability: %d", err);
	}
}
#endif

void method_gnss_lte_ind_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
		if (evt->modem_sleep.type == LTE_LC_MODEM_SLEEP_PSM) {
			/* Allow GNSS operation once LTE modem is in power saving mode. */
			k_sem_give(&entered_psm_mode);
		}
		break;
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
		/* Prevent GNSS from starting while LTE is active. */
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
	case LTE_LC_EVT_RRC_UPDATE:
		if (evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED) {
			/* Prevent GNSS from starting while RRC is in connected mode. */
			k_sem_reset(&entered_rrc_idle);
		} else if (evt->rrc_mode == LTE_LC_RRC_MODE_IDLE) {
			/* Allow GNSS operation once RRC is in idle mode. */
			k_sem_give(&entered_rrc_idle);
		}
		break;
	default:
		break;
	}
}

#if defined(CONFIG_NRF_CLOUD_AGNSS) && !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
#if defined(CONFIG_NRF_CLOUD_MQTT)
static void method_gnss_nrf_cloud_agnss_request(void)
{
	int err = nrf_cloud_agnss_request(&agnss_request);

	if (err) {
		LOG_ERR("nRF Cloud A-GNSS request failed, error: %d", err);
		return;
	}

	LOG_DBG("A-GNSS data requested");
}

#elif defined(CONFIG_NRF_CLOUD_REST) || defined(CONFIG_NRF_CLOUD_COAP)
static void method_gnss_nrf_cloud_agnss_request(void)
{
	int err;

#if defined(CONFIG_NRF_CLOUD_REST)
	const char *jwt_buf;
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = NRF_CLOUD_REST_TIMEOUT_NONE,
		.rx_buf = rest_api_recv_buf,
		.rx_buf_len = sizeof(rest_api_recv_buf),
		.fragment_size = 0
	};

	jwt_buf = location_utils_nrf_cloud_jwt_generate();
	if (jwt_buf == NULL) {
		return;
	}
	rest_ctx.auth = (char *)jwt_buf;
#endif

	struct nrf_cloud_rest_agnss_request request = {
		NRF_CLOUD_REST_AGNSS_REQ_CUSTOM,
		&agnss_request,
		NULL,
		false,
		0
	};

	struct lte_lc_cells_info net_info = {0};
	struct lte_lc_cells_info *scan_results;

	/* Get network info for the A-GNSS location request.
	 * Timeout value is just some number that should be big enough.
	 */
	scan_cellular_execute(5000, 0);
	scan_results = scan_cellular_results_get();
	if (scan_results == NULL) {
		LOG_WRN("Requesting A-GNSS data without location assistance");
	} else {
		net_info.current_cell.mcc = scan_results->current_cell.mcc;
		net_info.current_cell.mnc = scan_results->current_cell.mnc;
		net_info.current_cell.id = scan_results->current_cell.id;
		net_info.current_cell.tac = scan_results->current_cell.tac;
		net_info.current_cell.phys_cell_id = scan_results->current_cell.phys_cell_id;
		net_info.current_cell.rsrp = scan_results->current_cell.rsrp;
		request.net_info = &net_info;
	}

	struct nrf_cloud_rest_agnss_result result = {
		agnss_rest_data_buf,
		sizeof(agnss_rest_data_buf),
		0};

#if defined(CONFIG_NRF_CLOUD_REST)
	err = nrf_cloud_rest_agnss_data_get(&rest_ctx, &request, &result);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	err = nrf_cloud_coap_agnss_data_get(&request, &result);
#endif
	if (err) {
		LOG_ERR("nRF Cloud A-GNSS request failed, error: %d", err);
		return;
	}

	LOG_DBG("A-GNSS data requested");

	err = nrf_cloud_agnss_process(result.buf, result.agnss_sz);
	if (err) {
		LOG_ERR("A-GNSS data processing failed, error: %d", err);
		return;
	}

	LOG_DBG("A-GNSS data processed");

#if defined(CONFIG_NRF_CLOUD_PGPS)
	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("Error requesting notification of prediction availability: %d", err);
	}
#endif
}
#endif /* #elif defined(CONFIG_NRF_CLOUD_REST) || defined(CONFIG_NRF_CLOUD_COAP) */
#endif /* defined(CONFIG_NRF_CLOUD_AGNSS) && !defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */

#if defined(CONFIG_NRF_CLOUD_PGPS) && !defined(CONFIG_NRF_CLOUD_MQTT) && \
	!defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
static void method_gnss_pgps_request_work_fn(struct k_work *item)
{
	int err;

	struct nrf_cloud_rest_pgps_request request = {
		.pgps_req = &pgps_request
	};

#if defined(CONFIG_NRF_CLOUD_REST)
	const char *jwt_buf;
	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = NRF_CLOUD_REST_TIMEOUT_NONE,
		.rx_buf = rest_api_recv_buf,
		.rx_buf_len = sizeof(rest_api_recv_buf),
		.fragment_size = 0
	};

	jwt_buf = location_utils_nrf_cloud_jwt_generate();
	if (jwt_buf == NULL) {
		return;
	}
	rest_ctx.auth = (char *)jwt_buf;

	err = nrf_cloud_rest_pgps_data_get(&rest_ctx, &request);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	struct nrf_cloud_pgps_result file_location = {0};
	static char host[64];
	static char path[128];

	memset(host, 0, sizeof(host));
	memset(path, 0, sizeof(path));
	file_location.host = host;
	file_location.host_sz = sizeof(host);
	file_location.path = path;
	file_location.path_sz = sizeof(path);

	err = nrf_cloud_coap_pgps_url_get(&request, &file_location);
#endif
	if (err) {
		nrf_cloud_pgps_request_reset();
		LOG_ERR("nRF Cloud P-GPS request failed, error: %d", err);
		return;
	}

	LOG_DBG("P-GPS data requested");

#if defined(CONFIG_NRF_CLOUD_REST)
	err = nrf_cloud_pgps_process(rest_ctx.response, rest_ctx.response_len);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	err = nrf_cloud_pgps_update(&file_location);
#endif
	if (err) {
		nrf_cloud_pgps_request_reset();
		LOG_ERR("P-GPS data processing failed, error: %d", err);
		return;
	}

	LOG_DBG("P-GPS data processed");

	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("GNSS: Failed to request current prediction, error: %d", err);
	} else {
		LOG_DBG("P-GPS prediction requested");
	}
}
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static bool method_gnss_agnss_required(void)
{
	int32_t time_since_agnss_req;
	bool ephe_or_alm_required = false;

	/* Check if A-GNSS data is needed. */
	for (int i = 0; i < agnss_request.system_count; i++) {
		if (agnss_request.system[i].sv_mask_ephe != 0 ||
		    agnss_request.system[i].sv_mask_alm != 0) {
			ephe_or_alm_required = true;
			break;
		}
	}

	if (agnss_request.data_flags == 0 && !ephe_or_alm_required) {
		LOG_DBG("No A-GNSS data types requested");
		return false;
	}

	/* A-GNSS data is needed, check if enough time has passed since the last A-GNSS data
	 * request.
	 */
	if (agnss_req_timestamp != 0) {
		time_since_agnss_req = k_uptime_get() - agnss_req_timestamp;
		if (time_since_agnss_req < (AGNSS_REQUEST_MIN_INTERVAL * MSEC_PER_SEC)) {
			LOG_DBG("Skipping A-GNSS request, time since last request: %d s",
				time_since_agnss_req / 1000);
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
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

/* Triggers A-GNSS data request and/or injection of P-GPS predictions.
 *
 * Before this function is called, the assistance data need from GNSS must be stored into
 * 'agnss_request'.
 *
 * There are three possible assistance configurations with different behavior:
 *
 * A-GNSS only:
 * Ephemerides, almanacs and additional assistance data is handled by A-GNSS.
 * If any additional assistance data (UTC, Klobuchar, GPS time, integrity or position) is needed,
 * all additional assistance is requested at the same time.
 *
 * A-GNSS and P-GPS:
 * GPS ephemerides are handled by P-GPS.
 * GPS almanacs are handled by A-GNSS, but only if the downloaded P-GPS prediction set is longer
 * than one week.
 * Additional assistance data is handled by A-GNSS.
 * If any additional assistance data (UTC, Klobuchar, GPS time, integrity or position) is needed,
 * all additional assistance is requested at the same time.
 *
 * P-GPS only:
 * Ephemerides are handled by P-GPS.
 * Almanacs are not used.
 * GPS time and position assistance are handled by P-GPS.
 *
 * The frequency of A-GNSS data requests is limited to avoid requesting data repeatedly, for
 * example in case the server is down.
 *
 * Almanacs are not used with P-GPS prediction sets up to one week in length, because GNSS always
 * has valid (and more accurate) ephemerides available. With longer prediction sets, the number of
 * satellite ephemerides in the later prediction periods decreases due to accumulated errors,
 * so having almanacs may be beneficial.
 */
static void method_gnss_assistance_request(void)
{
#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* GPS ephemerides come from P-GPS. */
	pgps_agnss_request.system[0].sv_mask_ephe = agnss_request.system[0].sv_mask_ephe;
	pgps_agnss_request.data_flags = agnss_request.data_flags;
	agnss_request.system[0].sv_mask_ephe = 0;
	if (CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS <= 42) {
		/* GPS almanacs not needed in this configuration. */
		agnss_request.system[0].sv_mask_alm = 0;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_AGNSS)) {
		/* Time and position come from A-GNSS. */
		pgps_agnss_request.data_flags = 0;
	}

	LOG_DBG("P-GPS request sv_mask_ephe: 0x%08x, data_flags 0x%02x",
		(uint32_t)pgps_agnss_request.system[0].sv_mask_ephe, pgps_agnss_request.data_flags);
#endif /* CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_NRF_CLOUD_AGNSS)
	if (agnss_request.data_flags != 0) {
		/* With A-GNSS, if any of the flags in the data_flags field is set, it is
		 * feasible to request everything at the same time, because of the small amount
		 * of data.
		 */
		agnss_request.data_flags =
			NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
			NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST |
			NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
			NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST |
			NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST |
			NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST;
	}

	/* QZSS needs special handling because QZSS ephemerides are valid for a shorter time
	 * than GPS ephemerides, there are only a few QZSS satellites and GNSS reports unused
	 * QZSS satellites always as expired.
	 */
	if (agnss_request.system_count > 1) {
		if (agnss_request.data_flags != 0 ||
		    agnss_request.system[0].sv_mask_ephe != 0 ||
		    agnss_request.system[0].sv_mask_alm != 0) {
			/* QZSS ephemerides are requested always when other assistance data is
			 * needed.
			 */
			agnss_request.system[1].sv_mask_ephe = 0x3ff;
		} else {
			/* No other assistance is needed. Request QZSS ephemerides anyway if
			 * all QZSS ephemerides are expired and QZSS assistance is prioritized.
			 */
			if (!(agnss_request.system[1].sv_mask_ephe == 0x3ff &&
			     IS_ENABLED(CONFIG_LOCATION_METHOD_GNSS_PRIORITIZE_QZSS_ASSISTANCE))) {
				agnss_request.system[1].sv_mask_ephe = 0x0;
			}
		}

		/* QZSS almanacs are requested whenever GPS almanacs are requested. */
		if (agnss_request.system[0].sv_mask_alm != 0) {
			agnss_request.system[1].sv_mask_alm = 0x3ff;
		} else {
			agnss_request.system[1].sv_mask_alm = 0x0;
		}
	}

	LOG_DBG("A-GNSS request: data_flags: 0x%02x", agnss_request.data_flags);
	for (int i = 0; i < agnss_request.system_count; i++) {
		LOG_DBG("A-GNSS request: %s sv_mask_ephe: 0x%llx, sv_mask_alm: 0x%llx",
			get_system_string(agnss_request.system[i].system_id),
			agnss_request.system[i].sv_mask_ephe,
			agnss_request.system[i].sv_mask_alm);
	}

	/* Check if A-GNSS data should be requested. If A-GNSS request is not needed, jump to
	 * P-GPS (if enabled).
	 */
	if (method_gnss_agnss_required()) {
		enum lte_lc_nw_reg_status reg_status = LTE_LC_NW_REG_NOT_REGISTERED;

		lte_lc_nw_reg_status_get(&reg_status);
		if (reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
		    reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
			/* Only store the timestamp when LTE is connected, otherwise it is not
			 * very likely that the A-GNSS request succeeds.
			 */
			agnss_req_timestamp = k_uptime_get();
		}
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
		location_core_event_cb_agnss_request(&agnss_request);
#else
		method_gnss_nrf_cloud_agnss_request();
#endif
	}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
	if (pgps_agnss_request.system[0].sv_mask_ephe != 0) {
		/* When A-GNSS is used, the nRF Cloud library also calls this function after
		 * A-GNSS data has been processed. However, the call happens too late to trigger
		 * the initial P-GPS data download at the correct stage.
		 */
		int err = nrf_cloud_pgps_notify_prediction();

		if (err) {
			LOG_ERR("Error requesting notification of prediction availability: %d",
				err);
		}
	}
#endif /* CONFIG_NRF_CLOUD_PGPS */
}

static void method_gnss_agnss_req_event_handle_work_fn(struct k_work *item)
{
	int err = nrf_modem_gnss_read(&agnss_request,
				      sizeof(agnss_request),
				      NRF_MODEM_GNSS_DATA_AGNSS_REQ);

	if (err) {
		LOG_WRN("Reading A-GNSS req data from GNSS failed, error: %d", err);
		return;
	}

	/* GPS data need is always expected to be present and first in list. */
	__ASSERT(agnss_request.system_count > 0,
		 "GNSS system data need not found");
	__ASSERT(agnss_request.system[0].system_id == NRF_MODEM_GNSS_SYSTEM_GPS,
		 "GPS data need not found");

	method_gnss_assistance_request();
}
#endif /* defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS) */

void method_gnss_event_handler(int event)
{
	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		k_work_submit_to_queue(location_core_work_queue_get(), &method_gnss_pvt_work);
		break;

	case NRF_MODEM_GNSS_EVT_AGNSS_REQ:
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
		if (!agnss_expiry_get_supported) {
			k_work_submit_to_queue(
				location_core_work_queue_get(),
				&method_gnss_agnss_req_event_handle_work);
		}
#endif
		break;
	}
}

int method_gnss_cancel(void)
{
	int err = nrf_modem_gnss_stop();
	int sleeping;
	int rrc_idling;

	if ((err != 0) && (err != -NRF_EPERM)) {
		LOG_ERR("Failed to stop GNSS");
	}

	running = false;

	/* Cancel any work that has not been started yet */
	(void)k_work_cancel(&method_gnss_start_work);

	/* If we are currently not in PSM, i.e., LTE is running, reset the semaphore to unblock
	 * method_gnss_positioning_work_fn() and allow the ongoing location request to terminate.
	 * Otherwise, don't reset the semaphore in order not to lose information about the current
	 * sleep state.
	 */
	sleeping = k_sem_count_get(&entered_psm_mode);
	if (!sleeping) {
		k_sem_reset(&entered_psm_mode);
	}

	/* If we are currently in RRC connected mode, reset the semaphore to unblock
	 * method_gnss_positioning_work_fn() and allow the ongoing location request to terminate.
	 * Otherwise, don't reset the semaphore in order not to lose information about the current
	 * RRC state.
	 */
	rrc_idling = k_sem_count_get(&entered_rrc_idle);
	if (!rrc_idling) {
		k_sem_reset(&entered_rrc_idle);
	}

#if defined(CONFIG_NRF_CLOUD_PGPS)
	int ret;

	ret = nrf_cloud_pgps_preemptive_updates();
	if (ret) {
		LOG_ERR("Error requesting P-GPS pre-emptive updates: %d", ret);
	}
#endif /* CONFIG_NRF_CLOUD_PGPS */

	return err;
}

int method_gnss_timeout(void)
{
	if (insuf_timewin_count == -1 && gnss_config.priority_mode == false) {
		LOG_WRN("GNSS timed out possibly due to too short GNSS time windows");
	}

	return method_gnss_cancel();
}

#if !defined(CONFIG_NRF_CLOUD_AGNSS)
static bool method_gnss_psm_enabled(void)
{
	int ret = 0;
	int tau;
	int active_time;

	ret = lte_lc_psm_get(&tau, &active_time);
	if (ret < 0) {
		LOG_ERR("Cannot get PSM config: %d. Starting GNSS right away.", ret);
		return false;
	}

	LOG_DBG("LTE active time: %d seconds", active_time);

	if (active_time >= 0) {
		return true;
	}

	return false;
}

static bool method_gnss_entered_psm(void)
{
	LOG_DBG("%s", k_sem_count_get(&entered_psm_mode) == 0 ?
		"Waiting for LTE to enter PSM..." :
		"LTE already in PSM");

	/* Wait for the PSM to start. If semaphore is reset during the waiting
	 * period, the position request was canceled.
	 */
	if (k_sem_take(&entered_psm_mode, K_MINUTES(SLEEP_WAIT_BACKSTOP))
		== -EAGAIN) {
		if (!running) { /* Location request was cancelled */
			return false;
		}
		/* We're still running, i.e., the wait for PSM timed out */
		LOG_WRN("PSM is configured, but modem did not enter PSM "
			"in %d minutes. Starting GNSS anyway.",
			SLEEP_WAIT_BACKSTOP);
	}
	k_sem_give(&entered_psm_mode);
	return true;
}

static void method_gnss_modem_sleep_notif_subscribe(uint32_t threshold_ms)
{
	int err;

	err = nrf_modem_at_printf(AT_MDM_SLEEP_NOTIF_START, 0, threshold_ms);
	if (err) {
		LOG_ERR("Cannot subscribe to modem sleep notifications, err %d", err);
	} else {
		LOG_DBG("Subscribed to modem sleep notifications");
	}
}
#endif /* !CONFIG_NRF_CLOUD_AGNSS */

static bool method_gnss_allowed_to_start(void)
{
	enum lte_lc_system_mode mode;

	if (lte_lc_system_mode_get(&mode, NULL) != 0) {
		/* Failed to get system mode, try to start GNSS anyway */
		return true;
	}

	/* Don't care about LTE state if we are in GNSS only mode */
	if (mode == LTE_LC_SYSTEM_MODE_GPS) {
		return true;
	}

	LOG_DBG("%s", k_sem_count_get(&entered_rrc_idle) == 0 ?
		"Waiting for the RRC connection release..." :
		"RRC already in idle mode");

	/* If semaphore is reset during the waiting period, the position request was canceled.*/
	if (k_sem_take(&entered_rrc_idle, K_MINUTES(SLEEP_WAIT_BACKSTOP)) == -EAGAIN) {
		if (!running) { /* Location request was cancelled */
			return false;
		}
		/* We're still running, i.e., the wait for RRC idle timed out */
		LOG_WRN("RRC connection was not released in %d minutes. Starting GNSS anyway.",
			SLEEP_WAIT_BACKSTOP);
		return true;
	}
	k_sem_give(&entered_rrc_idle);

#if !defined(CONFIG_NRF_CLOUD_AGNSS)
	/* If A-GNSS is used, a GNSS fix can be obtained fast even in RRC idle mode (without PSM).
	 * Without A-GNSS, it's practical to wait for the modem to sleep before attempting a fix.
	 */
	if (method_gnss_psm_enabled()) {
		return method_gnss_entered_psm();
	}
#endif
	return true;
}

static uint8_t method_gnss_tracked_satellites(const struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked = 0;

	for (uint32_t i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (pvt_data->sv[i].sv == 0) {
			break;
		}

		tracked++;
	}

	return tracked;
}

static uint8_t method_gnss_tracked_satellites_nonzero_cn0(
		const struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	uint8_t tracked = 0;

	for (uint32_t i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if ((pvt_data->sv[i].sv == 0) || (pvt_data->sv[i].cn0 == 0)) {
			break;
		}

		tracked++;
	}

	return tracked;
}

static void method_gnss_print_pvt(const struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	LOG_DBG("Tracked satellites: %d, fix valid: %s, insuf. time window: %s",
		method_gnss_tracked_satellites(pvt_data),
		pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID ? "true" : "false",
		pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME ?
		"true" : "false");

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
	static struct location_data location_result = { 0 };
	uint8_t satellites_tracked_nonzero_cn0;

	if (!running) {
		/* Cancel has already been called, so ignore the notification. */
		return;
	}

	if (nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT) != 0) {
		LOG_ERR("Failed to read PVT data from GNSS");
		location_core_event_cb_error();
		return;
	}

	/* Only satellites with a reasonable C/N0 count towards the obstructed visibility limit */
	satellites_tracked_nonzero_cn0 = method_gnss_tracked_satellites_nonzero_cn0(&pvt_data);

	method_gnss_print_pvt(&pvt_data);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	location_data_details_gnss.pvt_data = pvt_data;
	location_data_details_gnss.satellites_tracked = method_gnss_tracked_satellites(&pvt_data);
#endif

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

		if (fixes_remaining <= 0) {
			/* We are done, stop GNSS and publish the fix. */
			method_gnss_cancel();
			location_core_event_cb(&location_result);
		}
	} else if (gnss_config.visibility_detection) {
		if (pvt_data.execution_time >= VISIBILITY_DETECTION_EXEC_TIME &&
		    pvt_data.execution_time < (VISIBILITY_DETECTION_EXEC_TIME + MSEC_PER_SEC) &&
		    satellites_tracked_nonzero_cn0 < VISIBILITY_DETECTION_SAT_LIMIT) {
			LOG_DBG("GNSS visibility obstructed, canceling");
			method_gnss_cancel();
			location_core_event_cb_error();
		}
	}

	/* Trigger GNSS priority mode if GNSS indicates that it is not getting long enough time
	 * windows for 5 consecutive epochs. If the priority mode option is not enabled, a trace
	 * is output in case of a timeout to warn that GNSS may be getting too short time windows
	 * to get a fix.
	 */
	if ((pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) &&
	    (insuf_timewin_count >= 0)) {
		insuf_timewin_count++;

		if (insuf_timewin_count == 5) {
			if (gnss_config.priority_mode) {
				LOG_DBG("GNSS is not getting long enough time windows. "
					"Triggering GNSS priority mode.");
				int err = nrf_modem_gnss_prio_mode_enable();

				if (err) {
					LOG_ERR("Unable to trigger GNSS priority mode.");
				}
			}

			/* Special value -1 indicates that the condition has been already triggered.
			 * Allow triggering priority mode only once in order to not block LTE
			 * operation excessively.
			 */
			insuf_timewin_count = -1;
		}
	} else if (insuf_timewin_count > 0) {
		/* GNSS must indicate that it is not getting long enough time windows for 5
		 * consecutive epochs to trigger priority mode.
		 */
		insuf_timewin_count = 0;
	}
}

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_PGPS)
static void method_gnss_pgps_ext_work_fn(struct k_work *item)
{
	location_core_event_cb_pgps_request(&pgps_request);
}
#endif

static void method_gnss_positioning_work_fn(struct k_work *work)
{
	int err = 0;

	if (!method_gnss_allowed_to_start()) {
		/* Location request was cancelled while waiting for RRC idle or PSM. Do nothing. */
		return;
	}

	/* Configure GNSS to continuous tracking mode */
	err = nrf_modem_gnss_fix_interval_set(1);
	if (err == -NRF_EACCES) {
		LOG_WRN("Modem's system or functional mode doesn't allow GNSS usage");
	}

#if defined(CONFIG_NRF_CLOUD_AGNSS_ELEVATION_MASK)
	err |= nrf_modem_gnss_elevation_threshold_set(CONFIG_NRF_CLOUD_AGNSS_ELEVATION_MASK);
#endif

	insuf_timewin_count = 0;

	/* By default we take the first fix. */
	fixes_remaining = 1;

	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

	switch (gnss_config.accuracy) {
	case LOCATION_ACCURACY_LOW:
		use_case |= NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
		break;

	case LOCATION_ACCURACY_NORMAL:
		break;

	case LOCATION_ACCURACY_HIGH:
		/* In high accuracy mode, use the configured fix count. */
		fixes_remaining = gnss_config.num_consecutive_fixes;
		break;
	}

	err |= nrf_modem_gnss_use_case_set(use_case);

	if (err) {
		LOG_ERR("Failed to configure GNSS");
		location_core_event_cb_error();
		running = false;
		return;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS");
		location_core_event_cb_error();
		running = false;
		return;
	}

	location_core_timer_start(gnss_config.timeout);
}

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
/* Processes A-GNSS expiry data from nrf_modem_gnss_agnss_expiry_get() function.
 *
 * This function is only used with modem firmware v1.3.2 or later. With older modem firmware
 * versions, the logic in this function is handled by GNSS.
 *
 * GNSS gives the expiration times in seconds, so logic is needed to process the expiration times
 * and to determine which data (if any) should be requested.
 *
 * Ephemerides:
 * Each satellite has its own ephemeris expiration time. The code goes though all satellites and
 * checks whether its ephemeris has expired or is going to expire. If there are enough satellites
 * with expired ephemerides, ephemerides for all satellites are requested. The time when an
 * ephemeris is considered expired and the threshold for the number of expired satellites is
 * different with A-GNSS and P-GPS. The used constants are described in the beginning of the file.
 *
 * Almanacs:
 * Each satellite has its own almanac expiration time. The code goes though all satellites and
 * checks whether its almanac has expired or is going to expire. If there are enough satellites
 * with expired almanacs, almanacs for all satellites are requested.
 *
 * Other assistance data:
 * For other assistance data, the same A-GNSS expiration threshold is used if expiry time is
 * provided by GNSS. For some data, there is only a flag indicating whether the data is needed or
 * not.
 */
static void method_gnss_agnss_expiry_process(const struct nrf_modem_gnss_agnss_expiry *agnss_expiry)
{
	uint16_t ephe_expiry_threshold;
	uint8_t expired_ephes_min_count;
	uint8_t expired_gps_ephes = 0;
	uint8_t expired_gps_alms = 0;
	bool qzss_supported = false;
	uint32_t expired_qzss_ephe_mask = 0;
	uint32_t expired_qzss_alm_mask = 0;

	memset(&agnss_request, 0, sizeof(agnss_request));

	if (IS_ENABLED(CONFIG_NRF_CLOUD_PGPS)) {
		ephe_expiry_threshold = PGPS_EXPIRY_THRESHOLD;
	} else {
		ephe_expiry_threshold = AGNSS_EXPIRY_THRESHOLD;
	}

	for (int i = 0; i < agnss_expiry->sv_count; i++) {
		if (agnss_expiry->sv[i].system_id == NRF_MODEM_GNSS_SYSTEM_GPS) {
			if (agnss_expiry->sv[i].ephe_expiry <= ephe_expiry_threshold) {
				expired_gps_ephes++;
			}

			if (agnss_expiry->sv[i].alm_expiry <= AGNSS_EXPIRY_THRESHOLD) {
				expired_gps_alms++;
			}
		}

		if (agnss_expiry->sv[i].system_id == NRF_MODEM_GNSS_SYSTEM_QZSS) {
			qzss_supported = true;

			/* GNSS supports QZSS PRNs 193...202. Only part of these are currently
			 * deployed, so even after all available QZSS ephemerides and almanancs
			 * have been injected, GNSS reports the unused ones as expired.
			 */

			/* QZSS ephemerides are valid for a maximum of two hours, so no expiry
			 * is used here.
			 */
			if (agnss_expiry->sv[i].ephe_expiry == 0) {
				expired_qzss_ephe_mask |=
					1 << (agnss_expiry->sv[i].sv_id - FIRST_QZSS_PRN);
			}

			if (agnss_expiry->sv[i].alm_expiry < AGNSS_EXPIRY_THRESHOLD) {
				expired_qzss_alm_mask |=
					1 << (agnss_expiry->sv[i].sv_id - FIRST_QZSS_PRN);
			}
		}
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_PGPS)) {
		expired_ephes_min_count = PGPS_EPHE_MIN_COUNT;
	} else {
		expired_ephes_min_count = AGNSS_EPHE_MIN_COUNT;
	}

	agnss_request.system_count = 1;
	agnss_request.system[0].system_id = NRF_MODEM_GNSS_SYSTEM_GPS;
	if (expired_gps_ephes >= expired_ephes_min_count) {
		agnss_request.system[0].sv_mask_ephe = 0xffffffff;
	}
	if (expired_gps_alms >= AGNSS_ALM_MIN_COUNT) {
		agnss_request.system[0].sv_mask_alm = 0xffffffff;
	}

	if (agnss_expiry->utc_expiry <= AGNSS_EXPIRY_THRESHOLD) {
		agnss_request.data_flags |= NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST;
	}

	if (agnss_expiry->klob_expiry <= AGNSS_EXPIRY_THRESHOLD) {
		agnss_request.data_flags |= NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST;
	}

	if (agnss_expiry->neq_expiry <= AGNSS_EXPIRY_THRESHOLD) {
		agnss_request.data_flags |= NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST;
	}

	if (agnss_expiry->data_flags & NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		agnss_request.data_flags |= NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST;
	}

	if (agnss_expiry->integrity_expiry <= AGNSS_EXPIRY_THRESHOLD) {
		agnss_request.data_flags |= NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST;
	}

	/* Position is reported as being valid for 2h. The uncertainty increases with time,
	 * but in practice the position remains usable for a longer time. Because of this no
	 * margin is used when checking if position assistance is needed.
	 *
	 * Position is only requested when also some other assistance data is needed.
	 */
	if (agnss_expiry->position_expiry == 0 &&
	    (agnss_request.system[0].sv_mask_ephe != 0 ||
	     agnss_request.system[0].sv_mask_alm != 0 ||
	     agnss_request.data_flags != 0)) {
		agnss_request.data_flags |= NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST;
	}

	if (qzss_supported) {
		agnss_request.system_count = 2;
		agnss_request.system[1].system_id = NRF_MODEM_GNSS_SYSTEM_QZSS;
		agnss_request.system[1].sv_mask_ephe = expired_qzss_ephe_mask;
		agnss_request.system[1].sv_mask_alm = expired_qzss_alm_mask;
	}

	LOG_DBG("GPS: Expired ephemerides: %d, almanacs: %d", expired_gps_ephes, expired_gps_alms);

	LOG_DBG("A-GNSS data need: data_flags: 0x%02x", agnss_request.data_flags);
	for (int i = 0; i < agnss_request.system_count; i++) {
		LOG_DBG("A-GNSS data need: %s sv_mask_ephe: 0x%llx, sv_mask_alm: 0x%llx",
			get_system_string(agnss_request.system[i].system_id),
			agnss_request.system[i].sv_mask_ephe,
			agnss_request.system[i].sv_mask_alm);
	}
}

/* Queries assistance data need from GNSS.
 *
 * To maintain backward compatibility with older modem firmware versions, two different methods are
 * supported. If the nrf_modem_gnss_agnss_expiry_get() API is not supported (pre-v1.3.2 modem
 * firmware), GNSS is briefly started to trigger the NRF_MODEM_GNSS_EVT_AGNSS_REQ event from GNSS
 * in case assistance data is currently needed.
 */
static void method_gnss_agnss_req_work_fn(struct k_work *work)
{
	int err;
	struct nrf_modem_gnss_agnss_expiry agnss_expiry;

	err = nrf_modem_gnss_agnss_expiry_get(&agnss_expiry);
	if (err) {
		if (err == -NRF_EOPNOTSUPP) {
			LOG_DBG("nrf_modem_gnss_agnss_expiry_get() not supported by the modem "
				"firmware");

			/* Start and stop GNSS to trigger NRF_MODEM_GNSS_EVT_AGNSS_REQ event if
			 * assistance data is needed.
			 */
			nrf_modem_gnss_start();
			nrf_modem_gnss_stop();
		} else {
			LOG_WRN("nrf_modem_gnss_agnss_expiry_get() failed, err %d", err);
		}

		return;
	}

	agnss_expiry_get_supported = true;

	method_gnss_agnss_expiry_process(&agnss_expiry);

	method_gnss_assistance_request();
}
#endif

int method_gnss_location_get(const struct location_request_info *request)
{
	int err;

	gnss_config = *request->gnss;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	memset(&location_data_details_gnss, 0, sizeof(location_data_details_gnss));
#endif
	/* GNSS event handler is already set once in method_gnss_init(). If no other thread is
	 * using GNSS, setting it again is not needed.
	 */
	err = nrf_modem_gnss_event_handler_set(method_gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return err;
	}

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* P-GPS is only initialized here because initialization may trigger P-GPS data request
	 * which would fail if the device is not registered to a network.
	 */
	static bool initialized;

	if (!initialized) {
		struct nrf_cloud_pgps_init_param param = {
			.event_handler = method_gnss_pgps_handler,
			/* storage is defined by CONFIG_NRF_CLOUD_PGPS_STORAGE */
			.storage_base = 0u,
			.storage_size = 0u
		};

		err = nrf_cloud_pgps_init(&param);
		if (err) {
			LOG_ERR("Error from PGPS init: %d", err);
		} else {
			initialized = true;
		}
	}
#endif
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_submit_to_queue(location_core_work_queue_get(), &method_gnss_agnss_req_work);
	/* Sleep for a while before submitting the next work, otherwise A-GNSS data may not be
	 * downloaded before GNSS is started. If the nrf_modem_gnss_agnss_expiry_get() API is not
	 * supported, GNSS is briefly started and stopped to trigger the
	 * NRF_MODEM_GNSS_EVT_AGNSS_REQ event, which in turn causes the A-GNSS data download work
	 * item to be submitted into the work queue. This all needs to happen before the work item
	 * below is submitted.
	 */
	k_sleep(K_MSEC(100));
#endif

	k_work_submit_to_queue(location_core_work_queue_get(), &method_gnss_start_work);

	running = true;

	return 0;
}

#if defined(CONFIG_LOCATION_DATA_DETAILS)
void method_gnss_details_get(struct location_data_details *details)
{
	details->gnss = location_data_details_gnss;
}
#endif

int method_gnss_init(void)
{
	int err;
	running = false;

	err = nrf_modem_gnss_event_handler_set(method_gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return err;
	}

	k_work_init(&method_gnss_pvt_work, method_gnss_pvt_work_fn);
	k_work_init(&method_gnss_start_work, method_gnss_positioning_work_fn);
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
	k_work_init(&method_gnss_agnss_req_event_handle_work,
		    method_gnss_agnss_req_event_handle_work_fn);
	k_work_init(&method_gnss_agnss_req_work, method_gnss_agnss_req_work_fn);
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	k_work_init(&method_gnss_pgps_ext_work, method_gnss_pgps_ext_work_fn);
#endif
#if !defined(CONFIG_NRF_CLOUD_MQTT) && !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	k_work_init(&method_gnss_pgps_request_work, method_gnss_pgps_request_work_fn);
#endif
	k_work_init(&method_gnss_inject_pgps_work, method_gnss_inject_pgps_work_fn);
	k_work_init(&method_gnss_notify_pgps_work, method_gnss_notify_pgps_work_fn);

#endif

#if !defined(CONFIG_NRF_CLOUD_AGNSS)
	/* Subscribe to sleep notification to monitor when modem enters power saving mode */
	method_gnss_modem_sleep_notif_subscribe(MIN_SLEEP_DURATION_FOR_STARTING_GNSS);
#endif
	lte_lc_register_handler(method_gnss_lte_ind_handler);

#if !defined(CONFIG_LOCATION_METHOD_CELLULAR)
	/* Cellular location method is disabled, but GNSS method uses the cellular scan
	 * functionality for A-GNSS request. The module needs to be initialized explicitly, because
	 * init is not called by core.
	 */
	scan_cellular_init();
#endif

	return 0;
}
