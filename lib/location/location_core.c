/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>

#include "location_core.h"
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

/** Event handler. */
static location_event_handler_t event_handler;

/***** Variables for storing information on currently handled location request *****/

/** Event data for currently ongoing location request. */
static struct location_event_data current_event_data;

/** Configuration given for currently ongoing location request. */
static struct location_config current_config;

/** Index to the current_config.methods for the currently used method. */
static int current_method_index;

/***** Work queue and work item definitions *****/

#define LOCATION_CORE_STACK_SIZE 4096
#define LOCATION_CORE_PRIORITY  5
K_THREAD_STACK_DEFINE(location_core_stack, LOCATION_CORE_STACK_SIZE);

/** Work queue for location library. Location methods can run their tasks in it. */
static struct k_work_q location_core_work_q;

/** Handler for periodic location requests. */
static void location_core_periodic_work_fn(struct k_work *work);

/** Work item for periodic location requests. */
K_WORK_DELAYABLE_DEFINE(location_periodic_work, location_core_periodic_work_fn);

/** Handler for timeout. */
static void location_core_timeout_work_fn(struct k_work *work);

/** Work item for timeout handler. */
K_WORK_DELAYABLE_DEFINE(location_timeout_work, location_core_timeout_work_fn);

/** Semaphore protecting the use of location requests. */
K_SEM_DEFINE(location_core_sem, 1, 1);

/***** Location method configurations *****/

#if defined(CONFIG_LOCATION_METHOD_GNSS)
/** GNSS location method configuration. */
static const struct location_method_api method_gnss_api = {
	.method           = LOCATION_METHOD_GNSS,
	.method_string    = "GNSS",
	.init             = method_gnss_init,
	.location_get     = method_gnss_location_get,
	.cancel           = method_gnss_cancel,
};
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
/** Cellular location method configuration. */
static const struct location_method_api method_cellular_api = {
	.method           = LOCATION_METHOD_CELLULAR,
	.method_string    = "Cellular",
	.init             = method_cellular_init,
	.location_get     = method_cellular_location_get,
	.cancel           = method_cellular_cancel,
};
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
/** Wi-Fi location method configuration. */
static const struct location_method_api method_wifi_api = {
	.method           = LOCATION_METHOD_WIFI,
	.method_string    = "Wi-Fi",
	.init             = method_wifi_init,
	.location_get     = method_wifi_location_get,
	.cancel           = method_wifi_cancel,
};
#endif

/** Supported location methods. */
static const struct location_method_api *methods_supported[] = {
#if defined(CONFIG_LOCATION_METHOD_GNSS)
	&method_gnss_api,
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	&method_cellular_api,
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	&method_wifi_api,
#endif
	NULL
};

static void location_core_current_event_data_init(enum location_method method)
{
	memset(&current_event_data, 0, sizeof(current_event_data));

	current_event_data.location.method = method;
}

static void location_core_current_config_clear(void)
{
	memset(&current_config, 0, sizeof(current_config));
}

static const struct location_method_api *location_method_api_get(enum location_method method)
{
	const struct location_method_api *method_api = NULL;

	for (int i = 0; methods_supported[i] != NULL; i++) {
		if (method == methods_supported[i]->method) {
			method_api = methods_supported[i];
			break;
		}
	}

	return method_api;
}

static const char LOCATION_ACCURACY_LOW_STR[] = "low";
static const char LOCATION_ACCURACY_NORMAL_STR[] = "normal";
static const char LOCATION_ACCURACY_HIGH_STR[] = "high";

static const char *location_core_gnss_accuracy_str(enum location_accuracy accuracy)
{
	const char *accuracy_str = NULL;

	switch (accuracy) {
	case LOCATION_ACCURACY_LOW:
		accuracy_str = LOCATION_ACCURACY_LOW_STR;
		break;
	case LOCATION_ACCURACY_NORMAL:
		accuracy_str = LOCATION_ACCURACY_NORMAL_STR;
		break;
	case LOCATION_ACCURACY_HIGH:
		accuracy_str = LOCATION_ACCURACY_HIGH_STR;
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	return accuracy_str;
}

static const char LOCATION_SERVICE_ANY_STR[] = "Any";
static const char LOCATION_SERVICE_NRF_CLOUD_STR[] = "nRF Cloud";
static const char LOCATION_SERVICE_HERE_STR[] = "HERE";

static const char *location_core_service_str(enum location_service service)
{
	const char *service_str = NULL;

	switch (service) {
	case LOCATION_SERVICE_ANY:
		service_str = LOCATION_SERVICE_ANY_STR;
		break;
	case LOCATION_SERVICE_NRF_CLOUD:
		service_str = LOCATION_SERVICE_NRF_CLOUD_STR;
		break;
	case LOCATION_SERVICE_HERE:
		service_str = LOCATION_SERVICE_HERE_STR;
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
	return service_str;
}

int location_core_event_handler_set(location_event_handler_t handler)
{
	if (handler == NULL) {
		LOG_ERR("No event handler given");
		return -EINVAL;
	}

	event_handler = handler;

	return 0;
}

int location_core_init(void)
{
	int err;
	struct k_work_queue_config cfg = {
		.name = "location_api_workq",
	};

	/* location_core_work_q shall not be used in method init functions.
	 * It's initialized after methods because a second initialization after
	 * a failing one would result in two calls to k_work_queue_start,
	 * which is not allowed.
	 */
	for (int i = 0; methods_supported[i] != NULL; i++) {
		err = methods_supported[i]->init();
		if (err) {
			LOG_ERR("Failed to initialize '%s' method",
				methods_supported[i]->method_string);
			return err;
		}
		LOG_DBG("Initialized '%s' method successfully",
			methods_supported[i]->method_string);
	}

	k_work_queue_start(
		&location_core_work_q,
		location_core_stack,
		K_THREAD_STACK_SIZEOF(location_core_stack),
		LOCATION_CORE_PRIORITY,
		&cfg);

	return 0;
}

int location_core_validate_params(const struct location_config *config)
{
	const struct location_method_api *method_api;

	__ASSERT_NO_MSG(config != NULL);

	if (config->methods_count > CONFIG_LOCATION_METHODS_LIST_SIZE) {
		LOG_ERR("Maximum number of methods (%d) exceeded: %d",
			CONFIG_LOCATION_METHODS_LIST_SIZE, config->methods_count);
		return -EINVAL;
	}

	if ((config->interval > 0) && (config->interval < 10)) {
		LOG_ERR("Interval for periodic location updates must be longer than 10 seconds");
		return -EINVAL;
	}

	for (int i = 0; i < config->methods_count; i++) {
		/* Check if the method is valid */
		method_api = location_method_api_get(config->methods[i].method);
		if (method_api == NULL) {
			LOG_ERR("Location method (%d) not supported", config->methods[i].method);
			return -EINVAL;
		}
	}
	return 0;
}

void location_core_config_log(const struct location_config *config)
{
	enum location_method type;
	const struct location_method_api *method_api;

	LOG_DBG("Location configuration:");

	LOG_DBG("  Methods count: %d", config->methods_count);
	LOG_DBG("  Interval: %d", config->interval);
	LOG_DBG("  Mode: %d", config->mode);
	LOG_DBG("  List of methods:");

	for (uint8_t i = 0; i < config->methods_count; i++) {
		type = config->methods[i].method;
		method_api = location_method_api_get(type);

		LOG_DBG("    Method #%d", i);
		if (method_api != NULL) {
			LOG_DBG("      Method type: %s (%d)",
				method_api->method_string, type);
		} else {
			LOG_DBG("      Method type: Unknown (%d)", type);
		}

		if (type == LOCATION_METHOD_GNSS) {
			LOG_DBG("      Timeout: %dms", config->methods[i].gnss.timeout);
			LOG_DBG("      Accuracy: %s (%d)",
				location_core_gnss_accuracy_str(config->methods[i].gnss.accuracy),
				config->methods[i].gnss.accuracy);
		} else if (type == LOCATION_METHOD_CELLULAR) {
			LOG_DBG("      Timeout: %dms", config->methods[i].cellular.timeout);
			LOG_DBG("      Service: %s (%d)",
				location_core_service_str(config->methods[i].cellular.service),
				config->methods[i].cellular.service);
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		} else if (type == LOCATION_METHOD_WIFI) {
			LOG_DBG("      Timeout: %dms", config->methods[i].wifi.timeout);
			LOG_DBG("      Service: %s (%d)",
				location_core_service_str(config->methods[i].wifi.service),
				config->methods[i].wifi.service);
#endif
		}
	}
}

static void location_core_current_config_set(const struct location_config *config)
{
	__ASSERT_NO_MSG(config->methods_count <= CONFIG_LOCATION_METHODS_LIST_SIZE);

	if (config == &current_config) {
		/* This is hit in periodic mode */
		return;
	}

	location_core_current_config_clear();

	memcpy(&current_config, config, sizeof(struct location_config));
}

static int location_core_location_get_pos(const struct location_config *config)
{
	int err;
	enum location_method requested_method;

	location_core_current_config_set(config);
	/* Location request starts from the first method */
	current_method_index = 0;
	requested_method = config->methods[current_method_index].method;
	LOG_DBG("Requesting location with '%s' method",
		(char *)location_method_api_get(requested_method)->method_string);
	location_core_current_event_data_init(requested_method);
	err = location_method_api_get(requested_method)->location_get(
		&config->methods[current_method_index]);

	return err;
}

int location_core_location_get(const struct location_config *config)
{
	int err;

	/* Waiting one second if previous operation was just finished and not quite ready yet */
	err = k_sem_take(&location_core_sem, K_SECONDS(1));
	if (err) {
		LOG_ERR("Location request already ongoing");
		return -EBUSY;
	}

	return location_core_location_get_pos(config);
}

void location_core_event_cb_error(void)
{
	current_event_data.id = LOCATION_EVT_ERROR;

	location_core_event_cb(NULL);
}

void location_core_event_cb_timeout(void)
{
	current_event_data.id = LOCATION_EVT_TIMEOUT;

	location_core_event_cb(NULL);
}

#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
void location_core_event_cb_agps_request(const struct nrf_modem_gnss_agps_data_frame *request)
{
	struct location_event_data agps_request_event_data;

	agps_request_event_data.id = LOCATION_EVT_GNSS_ASSISTANCE_REQUEST;
	agps_request_event_data.agps_request = *request;
	event_handler(&agps_request_event_data);
}
#endif

#if defined(CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL)
void location_core_event_cb_pgps_request(const struct gps_pgps_request *request)
{
	struct location_event_data pgps_request_event_data;

	pgps_request_event_data.id = LOCATION_EVT_GNSS_PREDICTION_REQUEST;
	pgps_request_event_data.pgps_request = *request;
	event_handler(&pgps_request_event_data);
}
#endif

void location_core_event_cb(const struct location_data *location)
{
	char latitude_str[12];
	char longitude_str[12];
	char accuracy_str[12];
	enum location_method requested_method;
	enum location_method previous_method;
	int err;

	k_work_cancel_delayable(&location_timeout_work);

	if (location != NULL) {
		/* Location was acquired properly */
		current_event_data.id = LOCATION_EVT_LOCATION;
		current_event_data.location = *location;

		LOG_DBG("Location acquired successfully:");
		LOG_DBG("  method: %s (%d)", (char *)location_method_api_get(
			current_event_data.location.method)->method_string,
			current_event_data.location.method);
		/* Logging v1 doesn't support double and float logging. Logging v2 would support
		 * but that's up to application to configure.
		 */
		sprintf(latitude_str, "%.06f", current_event_data.location.latitude);
		LOG_DBG("  latitude: %s", latitude_str);
		sprintf(longitude_str, "%.06f", current_event_data.location.longitude);
		LOG_DBG("  longitude: %s", longitude_str);
		sprintf(accuracy_str, "%.01f", current_event_data.location.accuracy);
		LOG_DBG("  accuracy: %s m", accuracy_str);
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
			latitude_str, longitude_str);
		if (current_config.mode == LOCATION_REQ_MODE_ALL) {
			/* Get possible next method */
			previous_method = current_event_data.location.method;
			current_method_index++;
			if (current_method_index < current_config.methods_count) {
				requested_method =
					current_config.methods[current_method_index].method;
				LOG_INF("LOCATION_REQ_MODE_ALL: acquired location using '%s', "
					"trying with '%s' next",
					(char *)location_method_api_get(
						previous_method)->method_string,
					(char *)location_method_api_get(
						requested_method)->method_string);

				event_handler(&current_event_data);

				/* Run next method on the list */
				location_core_current_event_data_init(requested_method);
				err = location_method_api_get(requested_method)->location_get(
					&current_config.methods[current_method_index]);
				return;
			}
			LOG_INF("LOCATION_REQ_MODE_ALL: all methods done");

			/* Start from the beginning if in interval mode
			 * for possible cancel within restarting interval.
			 */
			current_method_index = 0;
		}
	} else {
		/* Get possible next method to be run */
		previous_method = current_event_data.location.method;
		current_method_index++;
		if (current_method_index < current_config.methods_count) {
			requested_method = current_config.methods[current_method_index].method;

			LOG_WRN("Failed to acquire location using '%s', "
				"trying with '%s' next",
				(char *)location_method_api_get(previous_method)->method_string,
				(char *)location_method_api_get(requested_method)->method_string);

			if (current_config.mode == LOCATION_REQ_MODE_ALL) {
				/* In ALL mode, events are sent for all methods and thus
				 * also for failure events
				 */
				event_handler(&current_event_data);
			}

			location_core_current_event_data_init(requested_method);
			err = location_method_api_get(requested_method)->location_get(
				&current_config.methods[current_method_index]);
			return;
		}
		LOG_ERR("Location acquisition failed and fallbacks are also done");
	}

	event_handler(&current_event_data);

	if (current_config.interval > 0) {
		k_work_schedule_for_queue(
			location_core_work_queue_get(),
			&location_periodic_work,
			K_SECONDS(current_config.interval));
	} else {
		location_core_current_config_clear();

		k_sem_give(&location_core_sem);
	}
}

struct k_work_q *location_core_work_queue_get(void)
{
	return &location_core_work_q;
}

static void location_core_periodic_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	location_core_location_get_pos(&current_config);
}

static void location_core_timeout_work_fn(struct k_work *work)
{
	enum location_method current_method =
		current_config.methods[current_method_index].method;

	ARG_UNUSED(work);

	LOG_WRN("Timeout occurred");

	location_method_api_get(current_method)->cancel();
	location_core_event_cb_timeout();
}

void location_core_timer_start(int32_t timeout)
{
	if (timeout != SYS_FOREVER_MS && timeout > 0) {
		LOG_DBG("Starting timer with timeout=%d", timeout);

		/* Using different work queue that the actual methods are using.
		 * In this case using system work queue while methods use location_core_work_q.
		 * If timeout is handled in the same work queue as the methods use for
		 * their operation, blocking waiting of semaphores will block the timeout from
		 * expiring and canceling methods.
		 */
		k_work_schedule(&location_timeout_work, K_MSEC(timeout));
	}
}

void location_core_timer_stop(void)
{
	k_work_cancel_delayable(&location_timeout_work);
}

int location_core_cancel(void)
{
	int err = 0;
	enum location_method current_method =
		current_config.methods[current_method_index].method;

	k_work_cancel_delayable(&location_timeout_work);
	k_work_cancel_delayable(&location_periodic_work);

	/* Check if location has been requested using one of the methods */
	if (current_method != 0) {
		LOG_DBG("Cancelling location method for '%s' method",
			(char *)location_method_api_get(current_method)->method_string);
		err = location_method_api_get(current_method)->cancel();

		/* -EPERM means method wasn't running and this is converted to no error.
		 * This is normal in periodic mode.
		 */
		if (err == -EPERM) {
			err = 0;
		}
	} else {
		LOG_DBG("No location request pending so not cancelling anything");
	}

	location_core_current_config_clear();

	k_sem_give(&location_core_sem);

	return err;
}
