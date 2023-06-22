/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <modem/location.h>
#include <date_time.h>

#include "location_tracking.h"

LOG_MODULE_REGISTER(location_tracking, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

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
	location_initialized = true;

	/* Construct a request for a periodic location report. */
	struct location_config config;

	/* Select methods based on configuration and load default settings */
	/* Methods will be attempted in the order they are listed here */
	enum location_method methods[] = {
		IF_ENABLED(CONFIG_LOCATION_TRACKING_GNSS,	(LOCATION_METHOD_GNSS,))
		IF_ENABLED(CONFIG_LOCATION_TRACKING_WIFI,	(LOCATION_METHOD_WIFI,))
		IF_ENABLED(CONFIG_LOCATION_TRACKING_CELLULAR,	(LOCATION_METHOD_CELLULAR,))
	};

	if (IS_ENABLED(CONFIG_LOCATION_TRACKING_GNSS)) {
		LOG_DBG("Selected the GNSS location tracking method.");
	}
	if (IS_ENABLED(CONFIG_LOCATION_TRACKING_WIFI)) {
		LOG_DBG("Selected the Wi-Fi location tracking method.");
	}
	if (IS_ENABLED(CONFIG_LOCATION_TRACKING_CELLULAR)) {
		LOG_DBG("Selected the Cellular location tracking method.");
	}
	if (ARRAY_SIZE(methods) == 0) {
		LOG_DBG("No positioning methods selected");
		return 0;
	}

	/* Load default settings accordingly */
	location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);

	/* Set the location report interval. */
	config.interval = interval;

	/* If GNSS location method is enabled, configure it
	 * (note: the GNSS location method is enabled by default in this sample)
	 */
	if (IS_ENABLED(CONFIG_LOCATION_TRACKING_GNSS)) {
		/* Set the GNSS timeout and desired accuracy.
		 * The order of config.methods matches the order of methods, so GNSS will always be
		 * the first element of the array if enabled.
		 */
		config.methods[0].gnss.timeout = CONFIG_GNSS_FIX_TIMEOUT_SECONDS * MSEC_PER_SEC;
		config.methods[0].gnss.accuracy = LOCATION_ACCURACY_NORMAL;
	}

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
