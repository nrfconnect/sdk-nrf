/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_AGNSS)
#include <net/nrf_cloud_agnss.h>
#endif
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif

#include "location_core.h"

LOG_MODULE_REGISTER(location, CONFIG_LOCATION_LOG_LEVEL);

/* Verify method configurations */

BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_METHOD_GNSS) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_CELLULAR) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_WIFI),
	"At least one location method must be enabled");

#if !defined(CONFIG_LOCATION_METHOD_GNSS)
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_GNSS),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_GNSS must be disabled when "
	"CONFIG_LOCATION_METHOD_GNSS is disabled");
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_GNSS),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_GNSS must be disabled when "
	"CONFIG_LOCATION_METHOD_GNSS is disabled");
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_GNSS),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_GNSS must be disabled when "
	"CONFIG_LOCATION_METHOD_GNSS is disabled");
#endif

#if !defined(CONFIG_LOCATION_METHOD_CELLULAR)
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_CELLULAR),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_CELLULAR must be disabled when "
	"CONFIG_LOCATION_METHOD_CELLULAR is disabled");
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_CELLULAR),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_CELLULAR must be disabled when "
	"CONFIG_LOCATION_METHOD_CELLULAR is disabled");
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_CELLULAR),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_CELLULAR must be disabled when "
	"CONFIG_LOCATION_METHOD_CELLULAR is disabled");
#endif

#if !defined(CONFIG_LOCATION_METHOD_WIFI)
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_WIFI),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_WIFI must be disabled when "
	"CONFIG_LOCATION_METHOD_WIFI is disabled");
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_WIFI),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_WIFI must be disabled when "
	"CONFIG_LOCATION_METHOD_WIFI is disabled");
BUILD_ASSERT(
	!IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_WIFI),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_WIFI must be disabled when "
	"CONFIG_LOCATION_METHOD_WIFI is disabled");
#endif

#if defined(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_NONE)
BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_NONE),
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_NONE must be enabled when "
	"CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_NONE is enabled");
#endif

static bool initialized;

static const char LOCATION_METHOD_CELLULAR_STR[] = "Cellular";
static const char LOCATION_METHOD_GNSS_STR[] = "GNSS";
static const char LOCATION_METHOD_WIFI_STR[] = "Wi-Fi";
static const char LOCATION_METHOD_INTERNAL_WIFI_CELLULAR_STR[] = "Wi-Fi + Cellular";
static const char LOCATION_METHOD_UNKNOWN_STR[] = "Unknown";

int location_init(location_event_handler_t handler)
{
	int err;

	err = location_core_event_handler_set(handler);
	if (err) {
		return err;
	}
	if (initialized) {
		/* Already initialized so library is ready. We just updated the event handler. */
		return 0;
	}

	err = location_core_init();
	if (err) {
		return err;
	}

	initialized = true;

	LOG_DBG("Location library initialized");

	return 0;
}

int location_request(const struct location_config *config)
{
	int err;
	struct location_config default_config = { 0 };

	if (!initialized) {
		LOG_ERR("Location library not initialized when calling %s", __func__);
		return -EPERM;
	}

	/* Go to default config handling if no config given or if no methods given */
	if (config == NULL || config->methods_count == 0) {

		location_config_defaults_set(&default_config, 0, NULL);

		if (config != NULL) {
			/* Top level configs are given and must be taken from given config */
			LOG_DBG("No method configuration given. "
				"Using default method configuration.");
			default_config.interval = config->interval;
			default_config.timeout = config->timeout;
			default_config.mode = config->mode;
		} else {
			LOG_DBG("No configuration given. Using default configuration.");
		}

		config = &default_config;
	}

	location_core_config_log(config);

	err = location_core_validate_params(config);
	if (err) {
		LOG_ERR("Invalid parameters given.");
		return err;
	}

	err = location_core_location_get(config);

	return err;
}

int location_request_cancel(void)
{
	if (!initialized) {
		LOG_ERR("Location library not initialized when calling %s", __func__);
		return -EPERM;
	}

	return location_core_cancel();
}

static void location_config_method_defaults_set(
	struct location_method_config *method,
	enum location_method method_type)
{
	__ASSERT_NO_MSG(method != NULL);

	method->method = method_type;
	if (method_type == LOCATION_METHOD_GNSS) {
#if defined(CONFIG_LOCATION_METHOD_GNSS)
		method->gnss.timeout = CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_TIMEOUT;

		if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_ACCURACY_LOW)) {
			method->gnss.accuracy = LOCATION_ACCURACY_LOW;
		} else if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_ACCURACY_HIGH)) {
			method->gnss.accuracy = LOCATION_ACCURACY_HIGH;
		} else { /* IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_ACCURACY_NORMAL) */
			method->gnss.accuracy = LOCATION_ACCURACY_NORMAL;
		}

		method->gnss.num_consecutive_fixes =
			CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_NUM_CONSECUTIVE_FIXES;
		method->gnss.visibility_detection =
			IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_VISIBILITY_DETECTION);
		method->gnss.priority_mode =
			IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_PRIORITY_MODE);
#endif
	} else if (method_type == LOCATION_METHOD_CELLULAR) {
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
		method->cellular.timeout = CONFIG_LOCATION_REQUEST_DEFAULT_CELLULAR_TIMEOUT;
		method->cellular.service = LOCATION_SERVICE_ANY;
		method->cellular.cell_count = CONFIG_LOCATION_REQUEST_DEFAULT_CELLULAR_CELL_COUNT;
#endif
	} else if (method_type == LOCATION_METHOD_WIFI) {
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		method->wifi.timeout = CONFIG_LOCATION_REQUEST_DEFAULT_WIFI_TIMEOUT;
		method->wifi.service = LOCATION_SERVICE_ANY;
#endif
	}
}

void location_config_defaults_set(
	struct location_config *config,
	uint8_t methods_count,
	enum location_method *method_types)
{
	enum location_method method_types_tmp[CONFIG_LOCATION_METHODS_LIST_SIZE];

	if (config == NULL) {
		LOG_ERR("Configuration must not be NULL");
		return;
	}

	if (methods_count > CONFIG_LOCATION_METHODS_LIST_SIZE) {
		LOG_ERR("Maximum number of methods (%d) exceeded: %d",
			CONFIG_LOCATION_METHODS_LIST_SIZE, methods_count);
		return;
	}

	memset(config, 0, sizeof(struct location_config));
	config->interval = CONFIG_LOCATION_REQUEST_DEFAULT_INTERVAL;
	config->timeout = CONFIG_LOCATION_REQUEST_DEFAULT_TIMEOUT;
	config->mode = LOCATION_REQ_MODE_FALLBACK;

	/* Handle Kconfig's for method priorities */
	if (method_types == NULL) {
		methods_count = 0;
		if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_GNSS)) {
			method_types_tmp[0] = LOCATION_METHOD_GNSS;
			methods_count++;
		} else if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_WIFI)) {
			method_types_tmp[0] = LOCATION_METHOD_WIFI;
			methods_count++;
		} else { /* IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST_CELLULAR) */
			method_types_tmp[0] = LOCATION_METHOD_CELLULAR;
			methods_count++;
		}

		if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_GNSS)) {
			method_types_tmp[1] = LOCATION_METHOD_GNSS;
			methods_count++;
		} else if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_WIFI)) {
			method_types_tmp[1] = LOCATION_METHOD_WIFI;
			methods_count++;
		} else if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND_CELLULAR)) {
			method_types_tmp[1] = LOCATION_METHOD_CELLULAR;
			methods_count++;
		}

		if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_GNSS)) {
			method_types_tmp[2] = LOCATION_METHOD_GNSS;
			methods_count++;
		} else if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_WIFI)) {
			method_types_tmp[2] = LOCATION_METHOD_WIFI;
			methods_count++;
		} else if (IS_ENABLED(CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD_CELLULAR)) {
			method_types_tmp[2] = LOCATION_METHOD_CELLULAR;
			methods_count++;
		}
		method_types = method_types_tmp;
	}
	config->methods_count = methods_count;

	for (int i = 0; i < methods_count; i++) {
		location_config_method_defaults_set(&config->methods[i], method_types[i]);
	}
}

const char *location_method_str(enum location_method method)
{
	switch (method) {
	case LOCATION_METHOD_CELLULAR:
		return LOCATION_METHOD_CELLULAR_STR;

	case LOCATION_METHOD_GNSS:
		return LOCATION_METHOD_GNSS_STR;

	case LOCATION_METHOD_WIFI:
		return LOCATION_METHOD_WIFI_STR;

	default:
		/* Wi-Fi + Cellular method cannot be checked in switch-case because
		 * it's not defined in the enum
		 */
		if (method == LOCATION_METHOD_INTERNAL_WIFI_CELLULAR) {
			return LOCATION_METHOD_INTERNAL_WIFI_CELLULAR_STR;
		}
		return LOCATION_METHOD_UNKNOWN_STR;
	}
}

int location_agnss_data_process(const char *buf, size_t buf_len)
{
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_AGNSS)
	int err;

	if (!buf) {
		LOG_ERR("A-GNSS data buffer cannot be a NULL pointer.");
		return -EINVAL;
	}
	if (!buf_len) {
		LOG_ERR("A-GNSS data buffer length cannot be zero.");
		return -EINVAL;
	}

	err = nrf_cloud_agnss_process(buf, buf_len);
	if (err) {
		LOG_ERR("A-GNSS data processing failed, error: %d", err);
	}

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* Ephemerides are handled by P-GPS, so request the P-GPS library to inject current
	 * ephemerides as well.
	 */
	nrf_cloud_pgps_notify_prediction();
#endif

	return err;
#endif /* CONFIG_LOCATION_SERVICE_EXTERNAL && CONFIG_NRF_CLOUD_AGNSS */
	return -ENOTSUP;
}

int location_pgps_data_process(const char *buf, size_t buf_len)
{
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_PGPS)
	int err;

	if (!buf) {
		LOG_ERR("P-GPS data buffer cannot be a NULL pointer.");
		return -EINVAL;
	}

	if (!buf_len) {
		LOG_ERR("P-GPS data buffer length cannot be zero.");
		return -EINVAL;
	}

	err = nrf_cloud_pgps_process(buf, buf_len);
	if (err) {
		nrf_cloud_pgps_request_reset();
		LOG_ERR("P-GPS data processing failed, error: %d", err);
	}

	return err;
#endif /* CONFIG_LOCATION_SERVICE_EXTERNAL && CONFIG_NRF_CLOUD_PGPS */
	return -ENOTSUP;
}

void location_cloud_location_ext_result_set(
	enum location_ext_result result,
	struct location_data *location)
{
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) &&\
	(defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI))
	location_core_cloud_location_ext_result_set(result, location);
#endif
}
