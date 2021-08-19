/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <assert.h>
#include <logging/log.h>
#include <modem/location.h>

#include "loc_core.h"
#if defined(CONFIG_LOCATION_METHOD_GNSS)
#include "method_gnss.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
#include "method_cellular.h"
#endif

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

static location_event_handler_t event_handler;

/***** Variables for storing information on currently handled location request *****/

/** @brief Event data for currently ongoing location request. */
static struct loc_event_data current_event_data;

/** @brief Configuration given for currently ongoing location request. */
static struct loc_config current_loc_config = { 0 };

/** @brief Method specific configuration.
 * @details This is for reserving memory and this table is referred to in
 * current_loc_config.methods.
 */
static struct loc_method_config current_loc_config_methods[LOC_MAX_METHODS] = { 0 };

/** @brief Index to the current_loc_config_methods for the currently used method. */
static int current_loc_method_index;

/***** Work queue and work item definitions *****/

#define LOC_CORE_STACK_SIZE 2048
#define LOC_CORE_PRIORITY  5
K_THREAD_STACK_DEFINE(loc_core_stack, LOC_CORE_STACK_SIZE);

/** @brief Work queue for location library. Location methods can run their tasks in it. */
struct k_work_q loc_core_work_q;

/** @brief Handler for periodic location requests. */
static void loc_core_periodic_work_fn(struct k_work *work);

/** @brief Work item for periodic location requests. */
K_WORK_DELAYABLE_DEFINE(loc_periodic_work, loc_core_periodic_work_fn);

#if defined(CONFIG_LOCATION_METHOD_GNSS)
static const struct loc_method_api method_gnss_api = {
	.method           = LOC_METHOD_GNSS,
	.method_string    = "GNSS",
	.init             = method_gnss_init,
	.validate_params  = NULL,
	.location_get     = method_gnss_location_get,
	.cancel           = method_gnss_cancel,
};
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
static const struct loc_method_api method_cellular_api = {
	.method           = LOC_METHOD_CELLULAR,
	.method_string    = "Cellular",
	.init             = method_cellular_init,
	.validate_params  = NULL,
	.location_get     = method_cellular_location_get,
	.cancel           = method_cellular_cancel,
};
#endif

static const struct loc_method_api *methods_supported[] = {
#if defined(CONFIG_LOCATION_METHOD_GNSS)
	&method_gnss_api,
#else
	NULL,
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	&method_cellular_api,
#else
	NULL,
#endif
};

static void loc_core_current_event_data_init(enum loc_method method)
{
	memset(&current_event_data, 0, sizeof(current_event_data));

	current_event_data.method = method;
}

/** @brief Returns API for given location method.
 *
 * @param method Method.
 *
 * @return API of the location method. NULL if given method is not supported.
 */
static const struct loc_method_api *loc_method_api_validity_get(enum loc_method method)
{
	const struct loc_method_api *method_api = NULL;

	for (int i = 0; i < LOC_MAX_METHODS; i++) {
		if (method == methods_supported[i]->method) {
			method_api = methods_supported[i];
			break;
		}
	}
	return method_api;
}

/** @brief Returns API for given location method.
 *
 * @param method Method. Must be valid location method or otherwise asserts.
 *
 * @return API of the location method.
 */
static const struct loc_method_api *loc_method_api_get(enum loc_method method)
{
	const struct loc_method_api *method_api;

	method_api = loc_method_api_validity_get(method);

	/* This function is not supposed to be called when API is not found so
	 * to find issues elsewhere we'll assert here
	 */
	assert(method_api != NULL);
	return method_api;
}

int loc_core_init(location_event_handler_t handler)
{
	int err;

	if (handler == NULL) {
		LOG_ERR("No event handler given");
		return -EINVAL;
	}

	event_handler = handler;

	k_work_queue_start(
		&loc_core_work_q,
		loc_core_stack,
		K_THREAD_STACK_SIZEOF(loc_core_stack),
		LOC_CORE_PRIORITY,
		NULL);

	for (int i = 0; i < LOC_MAX_METHODS; i++) {
		if (methods_supported[i] != NULL) {
			err = methods_supported[i]->init();
			if (err) {
				LOG_ERR("Failed to initialize '%s' method",
					methods_supported[i]->method_string);
			} else {
				LOG_DBG("Initialized '%s' method successfully",
					methods_supported[i]->method_string);
			}
		}
	}

	current_loc_config.methods = current_loc_config_methods;

	return 0;
}

int loc_core_validate_params(const struct loc_config *config)
{
	const struct loc_method_api *method_api;
	int err;

	if (config->methods_count > LOC_MAX_METHODS) {
		LOG_ERR("Maximum number of methods (%d) exceeded: %d",
			LOC_MAX_METHODS, config->methods_count);
		return -EINVAL;
	}

	if ((config->interval > 0) && (config->interval < 10)) {
		LOG_ERR("Interval for periodic location updates must be 10...65535 seconds");
		return -EINVAL;
	}

	for (int i = 0; i < config->methods_count; i++) {
		/* Check if the method is valid */
		method_api = loc_method_api_validity_get(config->methods[i].method);
		if (method_api == NULL) {
			LOG_ERR("Location method (%d) not supported", config->methods[i].method);
			return -EINVAL;
		}
		/* Validate method specific parameters */
		if (methods_supported[i]->validate_params) {
			err = methods_supported[i]->validate_params(&config->methods[i]);
			if (err) {
				return err;
			}
		}
	}
	return 0;
}

static void current_loc_config_set(const struct loc_config *config)
{
	assert(config->methods_count <= LOC_MAX_METHODS);

	if (config == &current_loc_config) {
		/* This is hit in periodic mode */
		return;
	}

	current_loc_config.methods_count = config->methods_count;
	current_loc_config.interval = config->interval;

	memset(current_loc_config.methods, 0,
		sizeof(struct loc_method_config) * LOC_MAX_METHODS);
	memcpy(current_loc_config.methods, config->methods,
		sizeof(struct loc_method_config) * config->methods_count);
}

int loc_core_location_get(const struct loc_config *config)
{
	int err;
	enum loc_method requested_loc_method;

	current_loc_config_set(config);
	/* Location request starts from the first method */
	current_loc_method_index = 0;
	requested_loc_method = config->methods[current_loc_method_index].method;
	LOG_DBG("Requesting location with '%s' method",
		(char *)loc_method_api_get(requested_loc_method)->method_string);
	loc_core_current_event_data_init(requested_loc_method);
	err = loc_method_api_get(requested_loc_method)->location_get(
		&config->methods[current_loc_method_index]);

	return err;
}

void loc_core_event_cb_error(void)
{
	current_event_data.id = LOC_EVT_ERROR;

	loc_core_event_cb(NULL);
}

void loc_core_event_cb_timeout(void)
{
	current_event_data.id = LOC_EVT_TIMEOUT;

	loc_core_event_cb(NULL);
}

void loc_core_event_cb(const struct loc_location *location)
{
	char temp_str[16];
	enum loc_method requested_loc_method;
	enum loc_method previous_loc_method;
	int err;

	if (location != NULL) {
		/* Location was acquired properly, finish location request */
		current_event_data.id = LOC_EVT_LOCATION;
		current_event_data.location = *location;

		LOG_DBG("Location acquired successfully:");
		LOG_DBG("  method: %s (%d)",
			(char *)loc_method_api_get(current_event_data.method)->method_string,
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
	} else {
		/* Do fallback to next preferred method */
		previous_loc_method = current_event_data.method;
		current_loc_method_index++;
		if (current_loc_method_index < current_loc_config.methods_count) {
			requested_loc_method =
				current_loc_config.methods[current_loc_method_index].method;

			LOG_WRN("Failed to acquire location using '%s', "
				"trying with '%s' next",
				(char *)loc_method_api_get(previous_loc_method)->method_string,
				(char *)loc_method_api_get(requested_loc_method)->method_string);

			loc_core_current_event_data_init(requested_loc_method);
			err = loc_method_api_get(requested_loc_method)->location_get(
				&current_loc_config.methods[current_loc_method_index]);
			return;
		} else {
			LOG_ERR("Location acquisition failed and fallbacks are also done");
		}
	}

	event_handler(&current_event_data);

	if (current_loc_config.interval > 0) {
		k_work_schedule_for_queue(
			loc_core_work_queue_get(),
			&loc_periodic_work,
			K_SECONDS(current_loc_config.interval));
	}
}

struct k_work_q *loc_core_work_queue_get(void)
{
	return &loc_core_work_q;
}

static void loc_core_periodic_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	loc_core_location_get(&current_loc_config);
}

int loc_core_cancel(void)
{
	int err = 0;
	enum loc_method current_loc_method =
		current_loc_config.methods[current_loc_method_index].method;

	/* Check if location has been requested using one of the methods */
	if (current_loc_method != 0) {
		/* Run method cancel only if periodic work is not busy as otherwise
		 * we are just waiting for something to start running. */
		if (!k_work_delayable_busy_get(&loc_periodic_work)) {
			err = loc_method_api_get(current_loc_method)->cancel();
		}
	}

	if (k_work_delayable_busy_get(&loc_periodic_work) > 0) {
		k_work_cancel_delayable(&loc_periodic_work);
	}
	return err;
}
