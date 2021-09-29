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
#if defined(CONFIG_LOCATION_METHOD_WIFI)
#include "method_wifi.h"
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

#define LOC_CORE_STACK_SIZE 4096
#define LOC_CORE_PRIORITY  5
K_THREAD_STACK_DEFINE(loc_core_stack, LOC_CORE_STACK_SIZE);

/** @brief Work queue for location library. Location methods can run their tasks in it. */
struct k_work_q loc_core_work_q;

/** @brief Handler for periodic location requests. */
static void loc_core_periodic_work_fn(struct k_work *work);

/** @brief Work item for periodic location requests. */
K_WORK_DELAYABLE_DEFINE(loc_periodic_work, loc_core_periodic_work_fn);

/** @brief Handler for timeout. */
static void loc_core_timeout_work_fn(struct k_work *work);

/** @brief Work item for timeout handler. */
K_WORK_DELAYABLE_DEFINE(loc_timeout_work, loc_core_timeout_work_fn);

/* From location.c */
extern struct k_sem loc_core_sem;

/***** Location method configurations *****/

#if defined(CONFIG_LOCATION_METHOD_GNSS)
/** @brief GNSS location method configuration. */
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
/** @brief Cellular location method configuration. */
static const struct loc_method_api method_cellular_api = {
	.method           = LOC_METHOD_CELLULAR,
	.method_string    = "Cellular",
	.init             = method_cellular_init,
	.validate_params  = NULL,
	.location_get     = method_cellular_location_get,
	.cancel           = method_cellular_cancel,
};
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
/** @brief WiFi location method configuration. */
static const struct loc_method_api method_wifi_api = {
	.method           = LOC_METHOD_WIFI,
	.method_string    = "WiFi",
	.init             = method_wifi_init,
	.validate_params  = NULL,
	.location_get     = method_wifi_location_get,
	.cancel           = method_wifi_cancel,
};
#endif

/** @brief Supported location methods. */
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
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	&method_wifi_api,
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

static const char LOC_ACCURACY_LOW_STR[] = "low";
static const char LOC_ACCURACY_NORMAL_STR[] = "normal";
static const char LOC_ACCURACY_HIGH_STR[] = "high";

static const char *loc_core_gnss_accuracy_str(enum loc_accuracy accuracy)
{
	const char *accuracy_str = NULL;

	switch (accuracy) {
	case LOC_ACCURACY_LOW:
		accuracy_str = LOC_ACCURACY_LOW_STR;
		break;
	case LOC_ACCURACY_NORMAL:
		accuracy_str = LOC_ACCURACY_NORMAL_STR;
		break;
	case LOC_ACCURACY_HIGH:
		accuracy_str = LOC_ACCURACY_HIGH_STR;
		break;
	default:
		assert(0);
		break;
	}
	return accuracy_str;
}

int loc_core_init(location_event_handler_t handler)
{
	struct k_work_queue_config cfg = {
		.name = "location_api_workq",
	};
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
		&cfg);

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

	assert(config != NULL);

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

void loc_core_config_log(const struct loc_config *config)
{
	enum loc_method type;
	const struct loc_method_api *method_api;

	LOG_DBG("Location configuration:");

	LOG_DBG("  Methods count: %d", config->methods_count);
	LOG_DBG("  Interval: %d", config->interval);
	LOG_DBG("  List of methods:");

	for (uint8_t i = 0; i < config->methods_count; i++) {
		type = config->methods[i].method;
		method_api = loc_method_api_validity_get(type);

		LOG_DBG("    Method #%d", i);
		if (method_api != NULL) {
			LOG_DBG("      Method type: %s (%d)",
				log_strdup(method_api->method_string), type);
		} else {
			LOG_DBG("      Method type: unknown (%d)", type);
		}

		if (type == LOC_METHOD_GNSS) {
			LOG_DBG("      Timeout: %d", config->methods[i].gnss.timeout);
			LOG_DBG("      Accuracy: %s (%d)",
				log_strdup(loc_core_gnss_accuracy_str(
					config->methods[i].gnss.accuracy)),
				config->methods[i].gnss.accuracy);
			LOG_DBG("      Number of fixes: %d",
				config->methods[i].gnss.num_consecutive_fixes);
		} else if (type == LOC_METHOD_CELLULAR) {
			LOG_DBG("      Timeout: %d", config->methods[i].cellular.timeout);
		} else if (type == LOC_METHOD_WIFI) {
			LOG_DBG("      Timeout: %d", config->methods[i].wifi.timeout);
		}
	}
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

	memset(current_loc_config.methods, 0, sizeof(struct loc_method_config) * LOC_MAX_METHODS);
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
	char latitude_str[12];
	char longitude_str[12];
	char accuracy_str[12];
	enum loc_method requested_loc_method;
	enum loc_method previous_loc_method;
	int err;

	k_work_cancel_delayable(&loc_timeout_work);

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
		sprintf(latitude_str, "%.06f", current_event_data.location.latitude);
		LOG_DBG("  latitude: %s", log_strdup(latitude_str));
		sprintf(longitude_str, "%.06f", current_event_data.location.longitude);
		LOG_DBG("  longitude: %s", log_strdup(longitude_str));
		sprintf(accuracy_str, "%.01f", current_event_data.location.accuracy);
		LOG_DBG("  accuracy: %s m", log_strdup(accuracy_str));
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
		LOG_DBG("  Google maps URL: https://maps.google.com/?q=%s,%s",
			log_strdup(latitude_str), log_strdup(longitude_str));
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
		}
		LOG_ERR("Location acquisition failed and fallbacks are also done");
	}

	event_handler(&current_event_data);

	if (current_loc_config.interval > 0) {
		k_work_schedule_for_queue(
			loc_core_work_queue_get(),
			&loc_periodic_work,
			K_SECONDS(current_loc_config.interval));
	} else {
		memset(current_loc_config.methods, 0,
		       sizeof(struct loc_method_config) * LOC_MAX_METHODS);

		k_sem_give(&loc_core_sem);
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

static void loc_core_timeout_work_fn(struct k_work *work)
{
	enum loc_method current_loc_method =
		current_loc_config.methods[current_loc_method_index].method;

	ARG_UNUSED(work);

	LOG_WRN("Timeout occurred");

	loc_method_api_get(current_loc_method)->cancel();
	loc_core_event_cb_timeout();
}

void loc_core_timer_start(uint16_t timeout)
{
	if (timeout > 0) {
		LOG_DBG("Starting timer with timeout=%d", timeout);
		k_work_schedule_for_queue(
			loc_core_work_queue_get(),
			&loc_timeout_work,
			K_SECONDS(timeout));
	}
}

int loc_core_cancel(void)
{
	int err = 0;
	enum loc_method current_loc_method =
		current_loc_config.methods[current_loc_method_index].method;

	k_work_cancel_delayable(&loc_timeout_work);
	k_work_cancel_delayable(&loc_periodic_work);

	/* Check if location has been requested using one of the methods */
	if (current_loc_method != 0) {
		LOG_DBG("Cancelling location method for '%s' method",
			(char *)loc_method_api_get(current_loc_method)->method_string);
		err = loc_method_api_get(current_loc_method)->cancel();

		/* -EPERM means method wasn't running and this is converted to no error.
		 * This is normal in periodic mode.
		 */
		if (err == -EPERM) {
			err = 0;
		}
	} else {
		LOG_DBG("No location request pending so not cancelling anything");
	}

	memset(current_loc_config.methods, 0, sizeof(struct loc_method_config) * LOC_MAX_METHODS);

	k_sem_give(&loc_core_sem);

	return err;
}
