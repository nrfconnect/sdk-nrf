/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>

#include "location_core.h"
#include "location_utils.h"
#if defined(CONFIG_LOCATION_METHOD_GNSS)
#include "method_gnss.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
#include "scan_cellular.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
#include "scan_wifi.h"
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
#include "method_cloud_location.h"
#endif

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

/***** Variables for storing information on currently handled location request *****/

static struct location_request_info loc_req_info;

/***** Work queue and work item definitions *****/

#define LOCATION_CORE_STACK_SIZE CONFIG_LOCATION_WORKQUEUE_STACK_SIZE
#define LOCATION_CORE_PRIORITY  5
K_THREAD_STACK_DEFINE(location_core_stack, LOCATION_CORE_STACK_SIZE);

/** Work queue for location library. Location methods can run their tasks in it. */
static struct k_work_q location_core_work_q;

/** Handler for periodic location requests. */
static void location_core_periodic_work_fn(struct k_work *work);

/** Work item for periodic location requests. */
K_WORK_DELAYABLE_DEFINE(location_periodic_work, location_core_periodic_work_fn);

/** Handler for method timeout. */
static void location_core_method_timeout_work_fn(struct k_work *work);

/** Work item for method timeout handler. */
K_WORK_DELAYABLE_DEFINE(location_core_method_timeout_work, location_core_method_timeout_work_fn);

/** Handler for timeout. */
static void location_core_timeout_work_fn(struct k_work *work);

/** Work item for location request timeout handler. */
K_WORK_DELAYABLE_DEFINE(location_core_timeout_work, location_core_timeout_work_fn);

/** Location event callback. */
static void location_core_event_cb_fn(struct k_work *work);

/** Work item for location event callback. */
K_WORK_DEFINE(location_event_cb_work, location_core_event_cb_fn);

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
	.timeout          = method_gnss_timeout,
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	.details_get      = method_gnss_details_get,
#endif
};
#endif

#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
/** Cellular location method configuration. */
static const struct location_method_api method_cellular_api = {
	.method           = LOCATION_METHOD_CELLULAR,
	.method_string    = "Cellular",
	.init             = scan_cellular_init,
	.location_get     = method_cloud_location_get,
	.cancel           = method_cloud_location_cancel,
	.timeout          = method_cloud_location_cancel,
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	.details_get      = scan_cellular_details_get,
#endif
};
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
/** Wi-Fi location method configuration. */
static const struct location_method_api method_wifi_api = {
	.method           = LOCATION_METHOD_WIFI,
	.method_string    = "Wi-Fi",
	.init             = scan_wifi_init,
	.location_get     = method_cloud_location_get,
	.cancel           = method_cloud_location_cancel,
	.timeout          = method_cloud_location_cancel,
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	.details_get      = scan_wifi_details_get,
#endif

/** Threshold for cloud location method to select Wi-Fi vs. cellular into the returned event. */
#define METHOD_WIFI_ACCURACY_THRESHOLD 100
};
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
/**
 * Configuration for cloud location method, that is, combined Wi-Fi and cellular location method.
 * This is an internal method to handle combining sending of ncellmeas and Wi-Fi scan results
 * into a single cloud request. This method cannot be chosen in the library API.
 */
static const struct location_method_api method_cloud_location_api = {
	.method           = LOCATION_METHOD_WIFI_CELLULAR,
	.method_string    = "Wi-Fi + Cellular",
	.init             = method_cloud_location_init,
	.location_get     = method_cloud_location_get,
	.cancel           = method_cloud_location_cancel,
	.timeout          = method_cloud_location_cancel,
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	.details_get      = method_cloud_location_details_get,
#endif
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
#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
	&method_cloud_location_api,
#endif
	NULL
};

static void location_core_current_event_data_init(enum location_method method)
{
	memset(&loc_req_info.current_event_data, 0, sizeof(loc_req_info.current_event_data));

	loc_req_info.current_method = method;
	loc_req_info.elapsed_time_method_start_timestamp = k_uptime_get();
}

static void location_core_current_config_clear(void)
{
	memset(&loc_req_info.config, 0, sizeof(loc_req_info.config));
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

#if defined(CONFIG_LOG)

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

#endif

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
		if (methods_supported[i]->init == NULL) {
			continue;
		}
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

	for (int i = 0; i < config->methods_count; i++) {
		if (config->methods[i].method == LOCATION_METHOD_WIFI_CELLULAR) {
			LOG_ERR("LOCATION_METHOD_WIFI_CELLULAR cannot be given in location config");
			return -EINVAL;
		}
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
#if defined(CONFIG_LOG)
	enum location_method type;
	const struct location_method_api *method_api;

	LOG_DBG("Location configuration:");

	LOG_DBG("  Methods count: %d", config->methods_count);
	LOG_DBG("  Interval: %d", config->interval);
	LOG_DBG("  Timeout: %dms", config->timeout);
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
			LOG_DBG("      Cell count: %d", config->methods[i].cellular.cell_count);
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		} else if (type == LOCATION_METHOD_WIFI) {
			LOG_DBG("      Timeout: %dms", config->methods[i].wifi.timeout);
			LOG_DBG("      Service: %s (%d)",
				location_core_service_str(config->methods[i].wifi.service),
				config->methods[i].wifi.service);
#endif
		}
	}
#endif
}

static void location_core_current_config_set(const struct location_config *config)
{
	__ASSERT_NO_MSG(config->methods_count <= CONFIG_LOCATION_METHODS_LIST_SIZE);

	if (config == &loc_req_info.config) {
		/* This is hit in periodic mode */
		return;
	}

	memcpy(&loc_req_info.config, config, sizeof(loc_req_info.config));
}

static int location_core_location_get_pos(void)
{
	int err;
	enum location_method requested_method;

	location_core_current_config_set(&loc_req_info.config);
	/* Location request starts from the first method */
	loc_req_info.timeout_uptime = (loc_req_info.config.timeout != SYS_FOREVER_MS) ?
		k_uptime_get() + loc_req_info.config.timeout : SYS_FOREVER_MS;
	loc_req_info.execute_fallback = true;
	loc_req_info.current_method_index = 0;
	requested_method = loc_req_info.methods[loc_req_info.current_method_index];
	LOG_DBG("Requesting location with '%s' method",
		(char *)location_method_api_get(requested_method)->method_string);
	location_core_current_event_data_init(requested_method);

	err = location_method_api_get(requested_method)->location_get(&loc_req_info);
	if (err != 0) {
		return err;
	}

	if (IS_ENABLED(CONFIG_LOCATION_DATA_DETAILS)) {
		struct location_event_data request_started = {
			.id = LOCATION_EVT_STARTED,
			.method = requested_method
		};

		location_utils_event_dispatch(&request_started);
	}

	if (loc_req_info.config.timeout != SYS_FOREVER_MS &&
	    loc_req_info.config.timeout > 0) {
		LOG_DBG("Starting request timer with timeout=%d", loc_req_info.config.timeout);

		k_work_schedule(&location_core_timeout_work, K_MSEC(loc_req_info.config.timeout));
	}

	return 0;
}

static void location_request_info_create(const struct location_config *config)
{
	/* Initialize to big values so subtracting the indices from each other gives
	 * a big absolute value if one of the methods is not requested at all.
	 */
	int method_cellular_index = 1000;
	int method_wifi_index = 100000;
	bool combined = false;
	bool combine_wifi_cell = false;

	memset(&loc_req_info, 0, sizeof(loc_req_info));
	loc_req_info.config = *config;

	/* Pick up information from config to the location request information for quick access */
	for (int i = 0; i < config->methods_count; i++) {
		if (loc_req_info.config.methods[i].method == LOCATION_METHOD_CELLULAR) {
			loc_req_info.cellular = &loc_req_info.config.methods[i].cellular;
			method_cellular_index = i;
		} else if (loc_req_info.config.methods[i].method == LOCATION_METHOD_WIFI) {
			loc_req_info.wifi = &loc_req_info.config.methods[i].wifi;
			method_wifi_index = i;
		} else if (loc_req_info.config.methods[i].method == LOCATION_METHOD_GNSS) {
			loc_req_info.gnss = &loc_req_info.config.methods[i].gnss;
		}
	}

	/* Wi-Fi and cellular are not combined if LOCATION_REQ_MODE_ALL is used */
	if (loc_req_info.config.mode == LOCATION_REQ_MODE_FALLBACK) {
		/* Wi-Fi and cellular are combined if they are one after the other in method list */
		if (abs(method_wifi_index - method_cellular_index) == 1) {
			__ASSERT_NO_MSG(loc_req_info.cellular != NULL);
			__ASSERT_NO_MSG(loc_req_info.wifi != NULL);

			/* If Wi-Fi and cellular are using same service */
			if (loc_req_info.cellular->service == loc_req_info.wifi->service) {
				combine_wifi_cell = true;
			} else {
				LOG_DBG("Wi-Fi and cellular methods have different service "
					"so they are not combined");
			}
		} else if (loc_req_info.cellular != NULL && loc_req_info.wifi != NULL) {
			LOG_DBG("Wi-Fi and cellular methods are not one after the other "
				"in method list so they are not combined");
		}
	}

	/* Compose a list of methods that are really used, including combined internal method */
	for (int i = 0; i < loc_req_info.config.methods_count; i++) {
		if (combine_wifi_cell &&
		    (loc_req_info.config.methods[i].method == LOCATION_METHOD_WIFI ||
		     loc_req_info.config.methods[i].method == LOCATION_METHOD_CELLULAR)) {

			if (!combined) {
				LOG_INF("Wi-Fi and cellular methods combined");
				loc_req_info.methods[loc_req_info.methods_count] =
					LOCATION_METHOD_WIFI_CELLULAR;
				loc_req_info.methods_count++;
				combined = true;
			}
		} else {
			loc_req_info.methods[loc_req_info.methods_count] =
				loc_req_info.config.methods[i].method;
			loc_req_info.methods_count++;
		}
	}

#if defined(CONFIG_LOG)
	if (combined) {
		/* Log the updated method list */
		LOG_DBG("Updated location method list:");
		for (int i = 0; i < loc_req_info.methods_count; i++) {
			enum location_method type = loc_req_info.methods[i];
			const struct location_method_api *method_api =
				location_method_api_get(type);

			if (method_api != NULL) {
				LOG_DBG("    Method #%d type: %s (%d)",
					i, method_api->method_string, type);
			} else {
				LOG_DBG("    Method #%d type: Unknown (%d)", i, type);
			}
		}
	}
#endif
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

	location_request_info_create(config);

	return location_core_location_get_pos();
}

void location_core_event_cb_error(void)
{
	loc_req_info.current_event_data.id = LOCATION_EVT_ERROR;

	location_core_event_cb(NULL);
}

void location_core_event_cb_timeout(void)
{
	loc_req_info.current_event_data.id = LOCATION_EVT_TIMEOUT;

	location_core_event_cb(NULL);
}

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_AGNSS)
void location_core_event_cb_agnss_request(const struct nrf_modem_gnss_agnss_data_frame *request)
{
	struct location_event_data agnss_request_event_data = {
		.id = LOCATION_EVT_GNSS_ASSISTANCE_REQUEST,
		.method = LOCATION_METHOD_GNSS,
		.agnss_request = *request
	};

	LOG_DBG("Request A-GNSS data from application");
	location_utils_event_dispatch(&agnss_request_event_data);
}
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_PGPS)
void location_core_event_cb_pgps_request(const struct gps_pgps_request *request)
{
	struct location_event_data pgps_request_event_data = {
		.id = LOCATION_EVT_GNSS_PREDICTION_REQUEST,
		.method = LOCATION_METHOD_GNSS,
		.pgps_request = *request
	};

	LOG_DBG("Request P-GPS data from application: "
		"prediction_count %d, prediction_period_min %d, gps_day %d, time_of_day %d",
		request->prediction_count,
		request->prediction_period_min,
		request->gps_day,
		request->gps_time_of_day);

	location_utils_event_dispatch(&pgps_request_event_data);
}
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
void location_core_event_cb_cloud_location_request(struct location_data_cloud *request)
{
	struct location_event_data cloud_location_request_event_data = { 0 };

	cloud_location_request_event_data.id = LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST;

#if defined(CONFIG_LOCATION_METHOD_CELLULAR) && defined(CONFIG_LOCATION_METHOD_WIFI)
	/* For external service, we always determine Wi-Fi is used although it could be cellular */
	cloud_location_request_event_data.method =
		(request->wifi_data != NULL) ? LOCATION_METHOD_WIFI : LOCATION_METHOD_CELLULAR;
#else
	cloud_location_request_event_data.method = loc_req_info.current_method;
#endif
	loc_req_info.current_event_data.method = cloud_location_request_event_data.method;

#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	cloud_location_request_event_data.cloud_location_request.cell_data = request->cell_data;
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	cloud_location_request_event_data.cloud_location_request.wifi_data = request->wifi_data;
#endif
	location_utils_event_dispatch(&cloud_location_request_event_data);
}

void location_core_cloud_location_ext_result_set(
	enum location_ext_result result,
	struct location_data *location)
{
	if (k_sem_count_get(&location_core_sem) > 0) {
		LOG_WRN("Cloud positioning result set called but no location request pending");
		return;
	}

	LOG_DBG("Cloud positioning result set with result=%s",
		result == LOCATION_EXT_RESULT_SUCCESS ? "success" :
		result == LOCATION_EXT_RESULT_UNKNOWN ? "unknown" : "error");

	switch (result) {
	case LOCATION_EXT_RESULT_SUCCESS:
		loc_req_info.current_event_data.id = LOCATION_EVT_LOCATION;
		loc_req_info.current_event_data.location = *location;
		break;
	case LOCATION_EXT_RESULT_UNKNOWN:
		loc_req_info.current_event_data.id = LOCATION_EVT_RESULT_UNKNOWN;
		break;
	case LOCATION_EXT_RESULT_ERROR:
	default:
		loc_req_info.current_event_data.id = LOCATION_EVT_ERROR;
		break;
	}

	k_work_submit_to_queue(
		location_core_work_queue_get(),
		&location_event_cb_work);
}
#endif

static void location_core_event_details_get(struct location_event_data *event)
{
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	if (location_method_api_get(loc_req_info.current_method)->details_get != NULL) {

		struct location_data_details *details;

		if (event->id == LOCATION_EVT_LOCATION) {
			details = &event->location.details;
		} else {
			details = &event->error.details;
		}

		location_method_api_get(loc_req_info.current_method)->details_get(details);

		details->elapsed_time_method = (uint32_t)
			(k_uptime_get() - loc_req_info.elapsed_time_method_start_timestamp);
	}
#endif
}

static void location_core_event_cb_fn(struct k_work *work)
{
	char latitude_str[12];
	char longitude_str[12];
	char accuracy_str[12];
	enum location_method requested_method;
	int err;

	k_work_cancel_delayable(&location_core_method_timeout_work);
	loc_req_info.current_event_data.method = loc_req_info.current_method;

	/* Update the event structure with the details of the current method */
	location_core_event_details_get(&loc_req_info.current_event_data);

	if (loc_req_info.current_event_data.id == LOCATION_EVT_LOCATION) {
		/* Location was acquired properly.
		 * Caller sets loc_req_info.current_event_data.location
		 */

		LOG_DBG("Location acquired successfully:");
		LOG_DBG("  method: %s (%d)", (char *)location_method_api_get(
			loc_req_info.current_event_data.method)->method_string,
			loc_req_info.current_event_data.method);
		/* Logging v1 doesn't support double and float logging. Logging v2 would support
		 * but that's up to application to configure.
		 */
		sprintf(latitude_str, "%.06f", loc_req_info.current_event_data.location.latitude);
		LOG_DBG("  latitude: %s", latitude_str);
		sprintf(longitude_str, "%.06f", loc_req_info.current_event_data.location.longitude);
		LOG_DBG("  longitude: %s", longitude_str);
		sprintf(accuracy_str, "%.01f",
			(double)loc_req_info.current_event_data.location.accuracy);
		LOG_DBG("  accuracy: %s m", accuracy_str);
		if (loc_req_info.current_event_data.location.datetime.valid) {
			LOG_DBG("  date: %04d-%02d-%02d",
				loc_req_info.current_event_data.location.datetime.year,
				loc_req_info.current_event_data.location.datetime.month,
				loc_req_info.current_event_data.location.datetime.day);
			LOG_DBG("  time: %02d:%02d:%02d.%03d UTC",
				loc_req_info.current_event_data.location.datetime.hour,
				loc_req_info.current_event_data.location.datetime.minute,
				loc_req_info.current_event_data.location.datetime.second,
				loc_req_info.current_event_data.location.datetime.ms);
		}
		LOG_DBG("  Google maps URL: https://maps.google.com/?q=%s,%s",
			latitude_str, longitude_str);
		if (loc_req_info.config.mode == LOCATION_REQ_MODE_ALL) {
			/* Get possible next method */
			loc_req_info.current_method_index++;
			if (loc_req_info.current_method_index < loc_req_info.methods_count) {
				requested_method =
					loc_req_info.methods[loc_req_info.current_method_index];
				LOG_INF("LOCATION_REQ_MODE_ALL: acquired location using '%s', "
					"trying with '%s' next",
					(char *)location_method_api_get(
						loc_req_info.current_method)->method_string,
					(char *)location_method_api_get(
						requested_method)->method_string);

				location_utils_event_dispatch(&loc_req_info.current_event_data);

				/* Run next method on the list */
				location_core_current_event_data_init(requested_method);
				err = location_method_api_get(requested_method)->location_get(
					&loc_req_info);
				return;
			}
			LOG_INF("LOCATION_REQ_MODE_ALL: all methods done");

			/* Start from the beginning if in interval mode
			 * for possible cancel within restarting interval.
			 */
			loc_req_info.current_method_index = 0;
		}
	} else if (loc_req_info.execute_fallback) {
		/* Get possible next method to be run */
		loc_req_info.current_method_index++;
		if (loc_req_info.current_method_index < loc_req_info.methods_count) {
			requested_method = loc_req_info.methods[loc_req_info.current_method_index];

			LOG_INF("Location retrieval %s using '%s', trying with '%s' next",
				loc_req_info.current_event_data.id != LOCATION_EVT_RESULT_UNKNOWN ?
					"failed" : "completed",
				(char *)location_method_api_get(
					loc_req_info.current_method)->method_string,
				(char *)location_method_api_get(requested_method)->method_string);

			if (loc_req_info.config.mode == LOCATION_REQ_MODE_ALL) {
				/* In ALL mode, events are sent for all methods and thus
				 * also for failure events
				 */
				location_utils_event_dispatch(&loc_req_info.current_event_data);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
			} else {
				/* Details had been set into the error information */
				struct location_data_details *details =
					&loc_req_info.current_event_data.error.details;

				struct location_event_data fallback = {
					.id = LOCATION_EVT_FALLBACK,
					.method = loc_req_info.current_method,
					.fallback = {
						.next_method = requested_method,
						.cause = loc_req_info.current_event_data.id,
						.details = *details,
					}
				};
				location_utils_event_dispatch(&fallback);
#endif
			}

			location_core_current_event_data_init(requested_method);
			err = location_method_api_get(requested_method)->location_get(
				&loc_req_info);
			return;
		}
		if (loc_req_info.current_event_data.id != LOCATION_EVT_RESULT_UNKNOWN) {
			LOG_ERR("Location acquisition failed and fallbacks are also done");
		} else {
			LOG_DBG("Location acquisition completed and fallbacks are also done");
		}
	}

	location_utils_event_dispatch(&loc_req_info.current_event_data);

	k_work_cancel_delayable(&location_core_timeout_work);

	if (loc_req_info.config.interval > 0) {
		k_work_schedule_for_queue(
			location_core_work_queue_get(),
			&location_periodic_work,
			K_SECONDS(loc_req_info.config.interval));
	} else {
		location_core_current_config_clear();

		k_sem_give(&location_core_sem);
	}
}

void location_core_event_cb(const struct location_data *location)
{
	if (location) {
		loc_req_info.current_event_data.id = LOCATION_EVT_LOCATION;
		loc_req_info.current_event_data.location = *location;
	}

	if (k_work_busy_get(&location_event_cb_work) == 0) {
		/* If work item is idle, schedule it */
		k_work_submit_to_queue(
			location_core_work_queue_get(),
			&location_event_cb_work);
	} else {
		LOG_INF("Event is already scheduled so ignoring event %d",
			loc_req_info.current_event_data.id);
	}
}

struct k_work_q *location_core_work_queue_get(void)
{
	return &location_core_work_q;
}

static void location_core_periodic_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	location_core_location_get_pos();
}

static void location_core_method_timeout_work_fn(struct k_work *work)
{
	enum location_method current_method =
		loc_req_info.methods[loc_req_info.current_method_index];

	ARG_UNUSED(work);

	LOG_INF("Method specific timeout expired");

	location_method_api_get(current_method)->timeout();
	location_core_event_cb_timeout();
}

static void location_core_timeout_work_fn(struct k_work *work)
{
	enum location_method current_method =
		loc_req_info.methods[loc_req_info.current_method_index];

	ARG_UNUSED(work);

	LOG_INF("Timeout for entire location request expired");

	location_method_api_get(current_method)->timeout();
	/* config->timeout needs to expire without fallbacks */

	loc_req_info.current_event_data.id = LOCATION_EVT_TIMEOUT;
	loc_req_info.execute_fallback = false;

	location_core_event_cb(NULL);
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
		k_work_schedule(&location_core_method_timeout_work, K_MSEC(timeout));
	}
}

int location_core_cancel(void)
{
	int err = 0;
	enum location_method current_method =
		loc_req_info.methods[loc_req_info.current_method_index];

	k_work_cancel_delayable(&location_core_method_timeout_work);
	k_work_cancel_delayable(&location_core_timeout_work);
	k_work_cancel_delayable(&location_periodic_work);
	k_work_cancel(&location_event_cb_work);

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
