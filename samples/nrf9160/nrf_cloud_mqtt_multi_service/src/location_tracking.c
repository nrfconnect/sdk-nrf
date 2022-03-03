/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <modem/location.h>
#include <logging/log.h>
#include <date_time.h>
#include <net/nrf_cloud_agps.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>

#include "location_tracking.h"

LOG_MODULE_REGISTER(location_tracking, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

static location_update_cb_t location_update_handler;

/**
 * @brief Configure the antenna for GNSS!
 *
 * This must be done before the modem becomes active, which is
 * ensured by the NRF_MODEM_LIB_ON_INIT call below.
 */
static void gnss_antenna_configure(int ret, void *ctx)
{
	if (ret != 0) {
		return;
	}

	int err;

	if (strlen(CONFIG_GNSS_AT_MAGPIO) > 0) {
		err = nrf_modem_at_printf("%s", CONFIG_GNSS_AT_MAGPIO);
		if (err) {
			printk("Failed to set MAGPIO configuration\n");
		}
	}

	if (strlen(CONFIG_GNSS_AT_COEX0) > 0) {
		err = nrf_modem_at_printf("%s", CONFIG_GNSS_AT_COEX0);
		if (err) {
			printk("Failed to set COEX0 configuration\n");
		}
	}
}
NRF_MODEM_LIB_ON_INIT(location_tracking_init_hook, gnss_antenna_configure, NULL);

void location_agps_data_handler(const char *buf, size_t len)
{
	/* We don't actually check whether the passed-in payload contains AGPS data!
	 * Instead, nrf_cloud_agps_process() will check for us, ignoring payloads that
	 * don't contain AGPS data, and processing (and sending to the modem) payloads that do.
	 */

	int err = nrf_cloud_agps_process(buf, len);

	if (err) {
		if (err != -EBADMSG) {
			LOG_WRN("Unable to process A-GPS data, error: %d", err);
		} else {
			/* The passed in MQTT message was likely just not AGPS-related. */
			LOG_DBG("Failed to extract A-GPS data from passed in MQTT message, got "
				"-EBADMSG. This is normal behavior if a non-AGPS message was "
				"received.");
		}
	} else {
		LOG_DBG("A-GPS data processed");
	}
}

static void location_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		LOG_DBG("Location Event: Got location");
		if (location_update_handler) {
			/* Pass received location data along to our handler. */
			location_update_handler(event_data->location);
		}
		break;

	case LOCATION_EVT_TIMEOUT:
		LOG_DBG("Location Event: Timed out");
		break;

	case LOCATION_EVT_ERROR:
		LOG_DBG("Location Event: Error");
		break;

	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
		LOG_DBG("Location Event: GNSS assistance requested");
		break;

	default:
		LOG_DBG("Location Event: Unknown event");
		break;
	}
}

int start_location_tracking(location_update_cb_t handler_cb, int interval)
{
	int err;

	if (!date_time_is_valid()) {
		LOG_WRN("Date and time unknown. Location Services results may suffer");
	}

	/* Update the location update handler. */
	location_update_handler = handler_cb;

	/* Initialize the Location Services Library. */
	err = location_init(location_event_handler);
	if (err) {
		LOG_ERR("Initializing the Location library failed, error: %d", err);
		return err;
	}

	/* Construct a request for a periodic location report. */
	struct location_config config;

	/* First try GNSS, fallback to cellular. */
	enum location_method methods[] = {LOCATION_METHOD_GNSS, LOCATION_METHOD_CELLULAR};

	/* Load default settings. */
	location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);

	/* Set the location report interval. */
	config.interval = interval;

	/* Set the GNSS timeout and desired accuracy. */
	config.methods[0].gnss.timeout = CONFIG_GNSS_FIX_TIMEOUT_SECONDS;
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_NORMAL;

	/* Submit request for periodic location report.
	 * This will cause the configured location_event_handler to start being called with
	 * location data.
	 */
	err = location_request(&config);
	if (err) {
		LOG_ERR("Requesting location failed, error: %d\n", err);
		return err;
	}
	return 0;
}
