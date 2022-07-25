/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
#include <net/nrf_cloud_agps.h>
#endif
#if defined(CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL)
#include <net/nrf_cloud_pgps.h>
#endif

#include "location_core.h"

LOG_MODULE_REGISTER(location, CONFIG_LOCATION_LOG_LEVEL);

BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_METHOD_GNSS) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_CELLULAR) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_WIFI),
	"At least one location method must be enabled");

static bool initialized;

static const char LOCATION_METHOD_CELLULAR_STR[] = "Cellular";
static const char LOCATION_METHOD_GNSS_STR[] = "GNSS";
static const char LOCATION_METHOD_WIFI_STR[] = "Wi-Fi";
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
	enum location_method methods[] = {
#if defined(CONFIG_LOCATION_METHOD_GNSS)
		LOCATION_METHOD_GNSS,
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		LOCATION_METHOD_WIFI,
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
		LOCATION_METHOD_CELLULAR,
#endif
		};

	if (!initialized) {
		LOG_ERR("Location library not initialized when calling %s", __func__);
		return -EPERM;
	}

	/* Go to default config handling if no config given or if no methods given */
	if (config == NULL || config->methods_count == 0) {

		location_config_defaults_set(&default_config, ARRAY_SIZE(methods), methods);

		if (config != NULL) {
			/* Top level configs are given and must be taken from given config */
			LOG_DBG("No method configuration given. "
				"Using default method configuration.");
			default_config.interval = config->interval;
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
		method->gnss.timeout = 120 * MSEC_PER_SEC;
		method->gnss.accuracy = LOCATION_ACCURACY_NORMAL;
		method->gnss.num_consecutive_fixes = 3;
		method->gnss.visibility_detection = false;
	} else if (method_type == LOCATION_METHOD_CELLULAR) {
		method->cellular.timeout = 30 * MSEC_PER_SEC;
		method->cellular.service = LOCATION_SERVICE_ANY;
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	} else if (method_type == LOCATION_METHOD_WIFI) {
		method->wifi.timeout = 30 * MSEC_PER_SEC;
		method->wifi.service = LOCATION_SERVICE_ANY;
#endif
	}
}

void location_config_defaults_set(
	struct location_config *config,
	uint8_t methods_count,
	enum location_method *method_types)
{
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
	config->methods_count = methods_count;
	config->mode = LOCATION_REQ_MODE_FALLBACK;
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
		return LOCATION_METHOD_UNKNOWN_STR;
	}
}

int location_agps_data_process(const char *buf, size_t buf_len)
{
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
	if (!buf) {
		LOG_ERR("A-GPS data buffer cannot be a NULL pointer.");
		return -EINVAL;
	}
	if (!buf_len) {
		LOG_ERR("A-GPS data buffer length cannot be zero.");
		return -EINVAL;
	}
	return nrf_cloud_agps_process(buf, buf_len);
#endif
	return -ENOTSUP;
}

int location_pgps_data_process(const char *buf, size_t buf_len)
{
#if defined(CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL)
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
#endif
	return -ENOTSUP;
}
