/*
 * Minimal nRF9151 firmware - LTE + HTTP + GPS + FOTA
 * Target: Communication with Vercel server
 */

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <net/rest_client.h>
#include <nrf_modem_gnss.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>

LOG_MODULE_REGISTER(minimal_client, LOG_LEVEL_INF);

#define VERCEL_SERVER "your-app.vercel.app"  // Replace with your Vercel URL
#define SERVER_PORT 443

/* GPS data structure */
static struct {
	double latitude;
	double longitude;
	float altitude;
	bool valid;
} gps_data;

/* GNSS event handler */
static void gnss_event_handler(int event)
{
	int err;
	struct nrf_modem_gnss_pvt_data_frame pvt_data;

	if (event == NRF_MODEM_GNSS_EVT_PVT) {
		err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
		if (err == 0 && pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
			gps_data.latitude = pvt_data.latitude;
			gps_data.longitude = pvt_data.longitude;
			gps_data.altitude = pvt_data.altitude;
			gps_data.valid = true;
			LOG_INF("GPS: %.6f, %.6f, alt: %.1f",
				gps_data.latitude, gps_data.longitude, gps_data.altitude);
		}
	}
}

/* Initialize GNSS */
static int init_gnss(void)
{
	int err;

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler: %d", err);
		return err;
	}

	/* Configure GNSS for continuous tracking */
	err = nrf_modem_gnss_fix_interval_set(10); // 10 second intervals
	if (err) {
		LOG_ERR("Failed to set GNSS fix interval: %d", err);
		return err;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS: %d", err);
		return err;
	}

	LOG_INF("GNSS started");
	return 0;
}

/* Send data to Vercel server */
static int send_to_server(const char *endpoint, const char *data)
{
	int err;
	struct rest_client_req_context req_ctx = { 0 };
	struct rest_client_resp_context resp_ctx = { 0 };
	char response_buf[512];

	/* Configure REST client */
	req_ctx.connect_socket = -1;
	req_ctx.keep_alive = false;
	req_ctx.tls_peer_verify = REST_CLIENT_TLS_DEFAULT_VERIFY;
	req_ctx.timeout_ms = 10000;

	req_ctx.http_method = HTTP_POST;
	req_ctx.url = endpoint;
	req_ctx.host = VERCEL_SERVER;
	req_ctx.port = SERVER_PORT;
	req_ctx.body = data;

	req_ctx.header_fields = "Content-Type: application/json\r\n";

	resp_ctx.response_buf = response_buf;
	resp_ctx.response_buf_len = sizeof(response_buf);

	err = rest_client_request(&req_ctx, &resp_ctx);
	if (err) {
		LOG_ERR("REST request failed: %d", err);
		return err;
	}

	LOG_INF("Server response (%d): %s", resp_ctx.http_status_code, response_buf);
	return 0;
}

/* Send GPS coordinates to server */
static void send_gps_update(void)
{
	char json_buf[256];

	if (!gps_data.valid) {
		LOG_WRN("GPS data not valid yet");
		return;
	}

	snprintk(json_buf, sizeof(json_buf),
		"{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f}",
		gps_data.latitude, gps_data.longitude, gps_data.altitude);

	send_to_server("/api/location", json_buf);
}

/* LTE connection handler */
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
		    evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
			LOG_INF("Connected to LTE network");
		}
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update");
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	default:
		break;
	}
}

/* Initialize LTE connection */
static int init_lte(void)
{
	int err;

	LOG_INF("Initializing modem library");
	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize modem library: %d", err);
		return err;
	}

	LOG_INF("Connecting to LTE network");
	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init LTE connection: %d", err);
		return err;
	}

	/* Wait for connection */
	LOG_INF("Waiting for network connection...");
	return 0;
}

/* Check for firmware updates (FOTA) */
static void check_fota_update(void)
{
	/* Query server for firmware updates */
	char json_buf[128];
	int current_version = 1; // Increment this with each firmware version

	snprintk(json_buf, sizeof(json_buf), "{\"version\":%d}", current_version);

	/* Server should respond with download URL if update available */
	send_to_server("/api/fota/check", json_buf);

	/* Note: Full FOTA implementation requires:
	 * - Download firmware from server
	 * - Write to secondary MCUboot slot
	 * - Validate and swap boot slots
	 * - Reboot
	 */
}

int main(void)
{
	int err;

	LOG_INF("Minimal nRF9151 Vercel client starting...");

	/* Mark boot as successful for MCUboot */
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
		LOG_ERR("LTE init failed: %d", err);
		return err;
	}

	/* Wait for LTE connection (simplified) */
	k_sleep(K_SECONDS(30));

	/* Initialize GPS */
	err = init_gnss();
	if (err) {
		LOG_ERR("GNSS init failed: %d", err);
		/* Continue without GPS */
	}

	/* Main loop */
	while (1) {
		/* Send GPS update every 60 seconds */
		send_gps_update();

		/* Check for firmware updates every 5 minutes */
		static int loop_count = 0;
		if (++loop_count % 5 == 0) {
			check_fota_update();
		}

		/* Handle audio streaming here when you add BT module
		 * - Read from BT chip
		 * - Send to server
		 * - Receive from server
		 * - Play to BT chip
		 * - Log to SD card
		 */

		k_sleep(K_SECONDS(60));
	}

	return 0;
}
