/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>

#include <assert.h>

#include <nrf_modem_gnss.h>
#include <logging/log.h>

#include <net/multicell_location.h>
#include <modem/location.h>

#if defined(CONFIG_LOCATION_METHOD_GNSS)
#include "method_gnss.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
#include "method_cellular.h"
#endif

#include "location.h"

LOG_MODULE_REGISTER(location, CONFIG_LOCATION_LOG_LEVEL);

struct loc_event_data current_event_data;
static location_event_handler_t event_handler;
static int current_location_method_index = -1;
static struct loc_config current_loc_config;

#if defined(CONFIG_LOCATION_METHOD_GNSS)
const static struct location_method_api method_gnss_api = {
	.method_string    = "GNSS",
	.init             = method_gnss_init,
	.location_request = method_gnss_configure_and_start,
	.cancel_request   = method_gnss_cancel,
};
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
const static struct location_method_api method_cellular_api = {
	.method_string    = "Cellular",
	.init             = method_cellular_init,
	.location_request = method_cellular_configure_and_start,
	.cancel_request   = method_cellular_cancel,
};
#endif

static struct location_method_supported methods_supported[] = {
#if defined(CONFIG_LOCATION_METHOD_GNSS)
	{LOC_METHOD_GNSS, &method_gnss_api},
#else
	{0, NULL},
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	{LOC_METHOD_CELLULAR, &method_cellular_api},
#else
	{0, NULL},
#endif
};

static void loc_core_current_event_data_init(enum loc_method method)
{
	memset(&current_event_data, 0, sizeof(current_event_data));

	current_event_data.method = method;
}

const struct location_method_api *location_method_api_get(enum loc_method method)
{
	const struct location_method_api *method_api = NULL;

	for (int i = 0; i < LOC_MAX_METHODS; i++) {
		if (method == methods_supported[i].method) {
			method_api = methods_supported[i].api;
			break;
		}
	}
	/* This function is not supposed to be called when API is not found so
	 * to find issues elsewhere we'll assert here
	 */
	/* TODO: MOSH will hit this if some methods are disabled from compilation
	 *       and two methods are given
	 */
	assert(method_api != NULL);
	return method_api;
}

int location_init(location_event_handler_t handler)
{
	int err;
	static bool initialized;

	if (initialized) {
		/* Already initialized */
		return -EPERM;
	}

	if (handler == NULL) {
		LOG_ERR("No event handler given");
		return -EINVAL;
	}

	event_handler = handler;

	for (int i = 0; i < LOC_MAX_METHODS; i++) {
		if (methods_supported[i].method != 0) {
			err = methods_supported[i].api->init();
			if (err) {
				LOG_ERR("Failed to initialize '%s' method",
					methods_supported[i].api->method_string);
			} else {
				LOG_DBG("Initialized '%s' method successfully",
					methods_supported[i].api->method_string);
			}
		}
	}

	initialized = true;

	LOG_DBG("Library initialized");

	return 0;
}

int location_request(const struct loc_config *config)
{
	int err;
	enum loc_method requested_location_method;

	/* Location request starts from the first method */
	current_location_method_index = 0;
	current_loc_config = *config;

	/* TODO: Add protection so that only one request is handled at a time */

	/* TODO: Validate location method */
	/* TODO: Configuration validation into own function and own method specific validations */
	if ((config->interval > 0) && (config->interval < 10)) {
		LOG_ERR("Interval for periodic location updates must be 10...65535 seconds.");
		return -EINVAL;
	}

	requested_location_method = config->methods[current_location_method_index].method;
	loc_core_current_event_data_init(requested_location_method);
	err = location_method_api_get(requested_location_method)->location_request(
		&config->methods[current_location_method_index], config->interval);

	return err;
}

void event_location_callback_error()
{
	current_event_data.id = LOC_EVT_ERROR;

	event_location_callback(NULL);
}

void event_location_callback_timeout()
{
	current_event_data.id = LOC_EVT_TIMEOUT;

	event_location_callback(NULL);
}

void event_location_callback(const struct loc_location *location)
{
	char temp_str[16];
	enum loc_method requested_location_method;
	enum loc_method previous_location_method;
	int err;
	bool retry = false;

	if (location != NULL) {
		/* Location was acquired properly, finish location request */
		current_event_data.id = LOC_EVT_LOCATION;
		current_event_data.location = *location;

		LOG_DBG("Location acquired successfully:");
		LOG_DBG("  method: %s (%d)",
			(char *)location_method_api_get(current_event_data.method)->method_string,
			current_event_data.method);
		/* Logging v1 doesn't support double and float logging. Logging v2 would support
		 * but that's up to application to configure.
		 */
		sprintf(temp_str, "%.06f", current_event_data.location.latitude);
		LOG_DBG("  latitude: %s", log_strdup(temp_str));
		sprintf(temp_str, "%.06f", current_event_data.location.longitude);
		LOG_DBG("  longitude: %s", log_strdup(temp_str));
		sprintf(temp_str, "%.01f", current_event_data.location.accuracy);
		LOG_DBG("  accuracy: %s m", log_strdup(temp_str));
		if (current_event_data.location.datetime.valid) {
			LOG_DBG("  date: %04d-%02d-%02d",
				current_event_data.location.datetime.year,
				current_event_data.location.datetime.month,
				current_event_data.location.datetime.day);
			LOG_DBG("  time: %02d:%02d:%02d.%03d UTC",
				current_event_data.location.datetime.hour,
				current_event_data.location.datetime.minute,
				current_event_data.location.datetime.second,
				current_event_data.location.datetime.ms);
		}

		event_handler(&current_event_data);

		return;
	}

	/* Do fallback to next preferred method */
	previous_location_method = current_event_data.method;
	current_location_method_index++;
	if (current_location_method_index < LOC_MAX_METHODS) {
		requested_location_method =
			current_loc_config.methods[current_location_method_index].method;

		/* TODO: Should we have NONE method in the API? */
		if (requested_location_method != 0) {
			LOG_INF("Failed to acquire location using '%s', trying with '%s' next",
				(char *)location_method_api_get(previous_location_method)
					->method_string,
				(char *)location_method_api_get(requested_location_method)
					->method_string);

			retry = true;
			loc_core_current_event_data_init(requested_location_method);
			err = location_method_api_get(requested_location_method)->location_request(
				&current_loc_config.methods[current_location_method_index],
				current_loc_config.interval);
		}
	}

	if (!retry) {
		LOG_ERR("Location acquisition failed and no further trials will be made");
	}
}

int location_request_cancel(void)
{
	int err = -EPERM;

	/* Check if location has been requested using one of the methods */
	if (current_location_method_index >= 0) {
		enum loc_method current_location_method =
		current_loc_config.methods[current_location_method_index].method;

		err = location_method_api_get(current_location_method)->cancel_request();
	}
	
	return err;
}
