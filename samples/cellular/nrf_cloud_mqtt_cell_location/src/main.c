/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/location.h>
#include <modem/nrf_modem_lib.h>
#include <date_time.h>
#include <zephyr/logging/log.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_defs.h>
#include <app_version.h>

LOG_MODULE_REGISTER(nrf_cloud_mqtt_cell_location_sample,
		    CONFIG_NRF_CLOUD_MQTT_CELL_LOCATION_SAMPLE_LOG_LEVEL);

static K_SEM_DEFINE(location_event, 0, 1);

static K_SEM_DEFINE(lte_connected, 0, 1);

static K_SEM_DEFINE(time_update_finished, 0, 1);

static K_SEM_DEFINE(cloud_ready, 0, 1);

/* Structure to hold configuration flags. */
static struct nrf_cloud_location_config config = {
	.hi_conf = IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_HICONF_VAL),
	.fallback = IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_FALLBACK_VAL),
	.do_reply = IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_DOREPLY_VAL)};

static void date_time_evt_handler(const struct date_time_evt *evt)
{
	k_sem_give(&time_update_finished);
}

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_INF("Connected to network");
			k_sem_give(&lte_connected);
		}
		break;
	default:
		break;
	}
}

static void print_location(double latitude, double longitude, uint32_t accuracy)
{
	LOG_INF("Lat: %f, Lon: %f, Uncertainty: %u m", latitude, longitude, accuracy);
	LOG_INF("Google maps URL: https://maps.google.com/?q=%.06f,%.06f", latitude, longitude);
}

static void cloud_service_location_ready_cb(const struct nrf_cloud_location_result *const result)
{
	if (!result) {
		return;
	}

	if (result->err == NRF_CLOUD_ERROR_NONE) {
		const char *type_str = (result->type == LOCATION_TYPE_SINGLE_CELL)  ? "single-cell"
				       : (result->type == LOCATION_TYPE_MULTI_CELL) ? "multi-cell"
										    : "unknown";
		LOG_INF("Cellular location request fulfilled with %s", type_str);
		print_location(result->lat, result->lon, result->unc);

	} else {
		LOG_ERR("Unable to determine location from cloud service, error: %d", result->err);
	}
}

static void handle_cloud_location_request(const struct lte_lc_cells_info *cell_info)
{
	int err = 0;

	/* Send location request to nRF Cloud */
	err = nrf_cloud_location_request(cell_info, NULL, &config, cloud_service_location_ready_cb);
	if (err == -EACCES) {
		LOG_ERR("Cloud connection is not established");
	} else if (err) {
		LOG_ERR("Failed to request positioning data, error: %d", err);
	}
}

static void location_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		LOG_INF("Got location from location library");
		print_location((double)event_data->location.latitude,
			       (double)event_data->location.longitude,
			       (uint32_t)event_data->location.accuracy);
		break;

	case LOCATION_EVT_TIMEOUT:
		LOG_DBG("Getting location timed out");
		break;

	case LOCATION_EVT_ERROR:
		LOG_DBG("Getting location failed");
		break;

	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
		LOG_DBG("Getting location assistance requested (A-GNSS)");
		break;

	case LOCATION_EVT_GNSS_PREDICTION_REQUEST:
		LOG_DBG("Getting location assistance requested (P-GPS)");
		break;

	case LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST:
		LOG_DBG("Cloud location request received from location library");
		handle_cloud_location_request(event_data->cloud_location_request.cell_data);
		break;

	default:
		LOG_ERR("Getting location: Unknown event");
		break;
	}

	k_sem_give(&location_event);
}

static void location_event_wait(void)
{
	k_sem_take(&location_event, K_FOREVER);
}

/**
 * @brief Retrieve location with default configuration.
 *
 * @details This is achieved by not passing configuration at all to location_request().
 */
static void location_default_get(void)
{
	int err;

	LOG_INF("Requesting location with the default configuration...");

	err = location_request(NULL);
	if (err) {
		LOG_ERR("Requesting location failed, error: %d", err);
		return;
	}

	location_event_wait();
}

/* Handler for events from nRF Cloud Lib. */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_ERR("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_INF("Connection to nRF Cloud ready");
		k_sem_give(&cloud_ready);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_ERR("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		break;
	case NRF_CLOUD_EVT_RX_DATA_DISCON:
		LOG_ERR("Device was removed from your account.");
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_ERR("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
	case NRF_CLOUD_EVT_FOTA_START:
	case NRF_CLOUD_EVT_FOTA_DONE:
	case NRF_CLOUD_EVT_FOTA_ERROR:
		LOG_DBG("Cloud event: %d", nrf_cloud_evt->type);
		break;
	default:
		LOG_DBG("Unknown event type: %d", nrf_cloud_evt->type);
		break;
	}
}

int main(void)
{
	int err;

	LOG_INF("nRF Cloud MQTT Cellular Location Sample, version: %s", APP_VERSION_STRING);

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Modem library initialization failed, error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		/* Registering early for date_time event handler to avoid missing
		 * the first event after LTE is connected.
		 */
		date_time_register_handler(date_time_evt_handler);
	}

	LOG_INF("Connecting to LTE...");

	lte_lc_register_handler(lte_event_handler);

	lte_lc_connect();

	k_sem_take(&lte_connected, K_FOREVER);

	/* Waiting for valid time. */
	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		LOG_INF("Waiting for current time");

		/* Wait for an event from the Date Time library. */
		k_sem_take(&time_update_finished, K_MINUTES(10));

		if (!date_time_is_valid()) {
			LOG_ERR("Failed to get current time. Continuing anyway.");
		}
	}

	/* nRF Cloud MQTT requires login */
	struct nrf_cloud_init_param params = {.event_handler = cloud_event_handler,
					      .application_version = APP_VERSION_STRING};
	err = nrf_cloud_init(&params);
	if (err) {
		LOG_ERR("nRF Cloud library could not be initialized, error: %d", err);
		return err;
	}

	err = nrf_cloud_connect();
	if (err != 0) {
		LOG_ERR("Could not connect to nRF Cloud, error: %d", err);
		return err;
	}

	k_sem_take(&cloud_ready, K_FOREVER);

	err = location_init(location_event_handler);
	if (err) {
		LOG_ERR("Initializing the Location library failed, error: %d", err);
		return -1;
	}

	location_default_get();

	return 0;
}
