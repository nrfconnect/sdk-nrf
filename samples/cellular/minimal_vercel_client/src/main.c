/*
 * Minimal nRF9151 Vercel Client
 * Architecture based on nrf_cloud_multi_service, adapted for Vercel REST API
 *
 * Key improvements over basic version:
 * - Uses Location library (GNSS + Cellular + WiFi fallback)
 * - Proper date/time synchronization
 * - Connection state management
 * - Periodic sampling pattern from multi_service
 * - Better error handling
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/location.h>
#include <net/rest_client.h>
#include <date_time.h>
#include <zephyr/dfu/mcuboot.h>
#include <stdio.h>

LOG_MODULE_REGISTER(vercel_client, LOG_LEVEL_INF);

#define VERCEL_SERVER CONFIG_VERCEL_SERVER_HOSTNAME
#define SERVER_PORT 443
#define GPS_UPDATE_INTERVAL_SEC CONFIG_GPS_UPDATE_INTERVAL_SEC
#define FOTA_CHECK_INTERVAL_MIN CONFIG_FOTA_CHECK_INTERVAL_MIN

/* Application state */
static struct {
	bool lte_connected;
	bool location_tracking_active;
	int64_t last_fota_check_time;
} app_state;

/* Timers */
static K_TIMER_DEFINE(location_update_timer, NULL, NULL);

/* ========================================================================
 * HTTP/HTTPS Client (simplified from multi_service message_queue pattern)
 * ======================================================================== */

/**
 * Send HTTP POST request to Vercel server
 * Based on modem_shell/rest_shell.c pattern
 */
static int http_post_json(const char *endpoint, const char *json_data)
{
	int err;
	struct rest_client_req_context req_ctx = { 0 };
	struct rest_client_resp_context resp_ctx = { 0 };
	char response_buf[512];

	if (!app_state.lte_connected) {
		LOG_WRN("LTE not connected, skipping request");
		return -ENOTCONN;
	}

	/* Set request defaults */
	rest_client_request_defaults_set(&req_ctx);

	req_ctx.http_method = HTTP_POST;
	req_ctx.url = endpoint;
	req_ctx.host = VERCEL_SERVER;
	req_ctx.port = SERVER_PORT;
	req_ctx.body = json_data;
	req_ctx.tls_peer_verify = REST_CLIENT_TLS_DEFAULT_VERIFY;
	req_ctx.timeout_ms = 10000;
	req_ctx.header_fields = "Content-Type: application/json\r\n";

	resp_ctx.response_buf = response_buf;
	resp_ctx.response_buf_len = sizeof(response_buf);

	LOG_DBG("POST %s: %s", endpoint, json_data);

	err = rest_client_request(&req_ctx, &resp_ctx);
	if (err) {
		LOG_ERR("REST request failed: %d", err);
		return err;
	}

	LOG_INF("Server response (%d): %.*s",
		resp_ctx.http_status_code,
		MIN(resp_ctx.response_len, 100),
		response_buf);

	return resp_ctx.http_status_code == 200 ? 0 : -EIO;
}

/* ========================================================================
 * Location Tracking (pattern from location_tracking.c)
 * ======================================================================== */

/**
 * Location event handler
 * Similar to location_event_handler in multi_service/location_tracking.c
 */
static void location_event_handler(const struct location_event_data *event_data)
{
	char json_buf[256];
	int64_t timestamp;

	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		LOG_INF("Location acquired: method=%d", event_data->method);

		/* Get timestamp */
		if (date_time_now(&timestamp)) {
			timestamp = 0;
		}

		/* Format location data as JSON */
		if (event_data->method == LOCATION_METHOD_GNSS) {
			snprintk(json_buf, sizeof(json_buf),
				"{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,"
				"\"accuracy\":%.1f,\"method\":\"gnss\",\"ts\":%lld}",
				event_data->location.latitude,
				event_data->location.longitude,
				event_data->location.details.gnss.pvt_data.altitude,
				event_data->location.accuracy,
				timestamp);
		} else {
			snprintk(json_buf, sizeof(json_buf),
				"{\"lat\":%.6f,\"lon\":%.6f,"
				"\"accuracy\":%.1f,\"method\":\"cellular\",\"ts\":%lld}",
				event_data->location.latitude,
				event_data->location.longitude,
				event_data->location.accuracy,
				timestamp);
		}

		/* Send to server */
		http_post_json("/api/location", json_buf);
		break;

	case LOCATION_EVT_TIMEOUT:
		LOG_WRN("Location request timed out (method=%d)", event_data->method);
		break;

	case LOCATION_EVT_ERROR:
		LOG_ERR("Location error");
		break;

	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
		LOG_DBG("GNSS assistance requested");
		/* Could forward to Vercel for A-GNSS data if implemented */
		break;

	default:
		break;
	}
}

/**
 * Initialize location tracking
 * Pattern from start_location_tracking() in multi_service
 */
static int init_location_tracking(void)
{
	int err;

	LOG_INF("Initializing location tracking");

	/* Enable GNSS on modem */
	if (IS_ENABLED(CONFIG_LTE_LINK_CONTROL)) {
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS);
		if (err) {
			LOG_ERR("Failed to activate GNSS: %d", err);
			return err;
		}
	}

	/* Initialize Location library */
	err = location_init(location_event_handler);
	if (err) {
		LOG_ERR("Location init failed: %d", err);
		return err;
	}

	/* Configure location request */
	struct location_config config;
	enum location_method methods[] = {
		LOCATION_METHOD_GNSS,
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		LOCATION_METHOD_WIFI,
#endif
		LOCATION_METHOD_CELLULAR,
	};

	location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);

	/* Periodic updates */
	config.interval = GPS_UPDATE_INTERVAL_SEC;

	/* GNSS configuration */
	config.methods[0].gnss.timeout = 60 * MSEC_PER_SEC;
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_NORMAL;

	/* Start periodic location updates */
	err = location_request(&config);
	if (err) {
		LOG_ERR("Location request failed: %d", err);
		return err;
	}

	app_state.location_tracking_active = true;
	LOG_INF("Location tracking started (interval=%ds)", GPS_UPDATE_INTERVAL_SEC);

	return 0;
}

/* ========================================================================
 * FOTA Support (pattern from fota_support_coap.c)
 * ======================================================================== */

/**
 * Check for firmware updates
 * Simplified from fota_support_coap.c polling mechanism
 */
static void check_fota_update(void)
{
	char json_buf[128];
	int64_t now;
	static const int version = 1; /* Increment with each release */

	/* Rate limit checks */
	if (date_time_now(&now) == 0) {
		int64_t elapsed_min = (now - app_state.last_fota_check_time) / 60;
		if (elapsed_min < FOTA_CHECK_INTERVAL_MIN) {
			return;
		}
		app_state.last_fota_check_time = now;
	}

	LOG_INF("Checking for firmware updates (current version=%d)", version);

	snprintk(json_buf, sizeof(json_buf), "{\"version\":%d}", version);

	if (http_post_json("/api/fota/check", json_buf) == 0) {
		/* TODO: Parse response and download firmware if available
		 * See samples/cellular/http_update/application_update for full implementation
		 *
		 * Steps:
		 * 1. Parse JSON response for update_available, url, version
		 * 2. Use fota_download library to download firmware
		 * 3. Write to MCUboot secondary slot
		 * 4. Validate and reboot
		 */
		LOG_DBG("FOTA check completed");
	}
}

/* ========================================================================
 * LTE Connection Management (pattern from cloud_connection.c)
 * ======================================================================== */

/**
 * LTE event handler
 * Simplified from lte_lc_handler in cloud_connection.c
 */
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
		    evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
			if (!app_state.lte_connected) {
				LOG_INF("LTE network connected");
				app_state.lte_connected = true;

				/* Initialize date/time (important for timestamping) */
				date_time_update_async(NULL);

				/* Start location tracking once connected */
				if (!app_state.location_tracking_active) {
					init_location_tracking();
				}
			}
		} else {
			if (app_state.lte_connected) {
				LOG_WRN("LTE network disconnected");
				app_state.lte_connected = false;
			}
		}
		break;

	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU=%d, active_time=%d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;

	case LTE_LC_EVT_EDRX_UPDATE:
		LOG_DBG("eDRX parameter update");
		break;

	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;

	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("Cell update: ID=0x%08X, TAC=0x%04X",
			evt->cell.id, evt->cell.tac);
		break;

	default:
		break;
	}
}

/**
 * Initialize LTE connection
 */
static int init_lte(void)
{
	int err;

	LOG_INF("Initializing modem");
	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Modem init failed: %d", err);
		return err;
	}

	LOG_INF("Connecting to LTE network...");
	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("LTE init failed: %d", err);
		return err;
	}

	return 0;
}

/* ========================================================================
 * Main Application Thread (pattern from application.c)
 * ======================================================================== */

/**
 * Main application thread
 * Similar to application_thread_fn in multi_service/application.c
 */
static void application_thread(void)
{
	int err;
	int loop_count = 0;

	LOG_INF("nRF9151 Vercel Client starting...");
	LOG_INF("Server: %s", VERCEL_SERVER);

	/* Confirm boot for MCUboot */
	if (!boot_is_img_confirmed()) {
		err = boot_write_img_confirmed();
		if (err) {
			LOG_ERR("Failed to confirm boot: %d", err);
		} else {
			LOG_INF("Boot confirmed");
		}
	}

	/* Initialize LTE */
	err = init_lte();
	if (err) {
		LOG_ERR("Cannot continue without LTE");
		return;
	}

	/* Main loop */
	while (1) {
		/* Wait for LTE connection */
		if (!app_state.lte_connected) {
			LOG_DBG("Waiting for LTE connection...");
			k_sleep(K_SECONDS(5));
			continue;
		}

		/* Check for FOTA updates periodically */
		if (++loop_count % (FOTA_CHECK_INTERVAL_MIN * 60 / 30) == 0) {
			check_fota_update();
		}

		/* Sleep 30 seconds between loop iterations
		 * Location library handles periodic GPS updates automatically
		 */
		k_sleep(K_SECONDS(30));
	}
}

/* ========================================================================
 * Main Entry Point
 * ======================================================================== */

int main(void)
{
	/* Initialize state */
	app_state.lte_connected = false;
	app_state.location_tracking_active = false;
	app_state.last_fota_check_time = 0;

	/* Run application thread */
	application_thread();

	return 0;
}
