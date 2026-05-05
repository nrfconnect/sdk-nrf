/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <modem/location.h>
#include <date_time.h>

#include "cloud_connection.h"
#include "location_tracking.h"

LOG_MODULE_REGISTER(location_tracking, CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL);

static location_update_cb_t location_update_handler;
static bool location_initialized;

static void location_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		LOG_DBG("Location Event: Got location");
		if (location_update_handler) {
			/* Pass received location data along to our handler. */
			location_update_handler(event_data);
		}
		break;

	case LOCATION_EVT_TIMEOUT:
		LOG_DBG("Location Event: Timed out");
		cloud_transport_error_detected();
		break;

	case LOCATION_EVT_ERROR:
		LOG_DBG("Location Event: Error");
		break;

	case LOCATION_EVT_STARTED:
		break;

	default:
		LOG_DBG("Location Event: Unknown event");
		break;
	}
}

int start_location_tracking(location_update_cb_t handler_cb, int interval)
{
	int err;

	if (!IS_ENABLED(CONFIG_LOCATION_TRACKING)) {
		LOG_DBG("Location tracking is not enabled.");
		return 0;
	}

	LOG_DBG("Starting location tracking");

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
	location_initialized = true;

	/* Construct a request for a periodic location report. */
	struct location_config config;

	/* This sample only uses Wi-Fi for location tracking. */
	enum location_method methods[] = {
		LOCATION_METHOD_WIFI
	};

	/* Load default settings accordingly */
	location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);

	/* Set the mode to walk through all methods. This ensures all methods
	 * are tested and demonstrated.
	 * In a real product, the default is the better option to use, since the location
	 * library will try them in priority order.
	 */
	config.mode = LOCATION_REQ_MODE_ALL;

	/* Set the location report interval. */
	config.interval = interval;

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
