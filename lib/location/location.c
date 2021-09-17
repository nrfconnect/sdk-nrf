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

LOG_MODULE_REGISTER(location, CONFIG_LOCATION_LOG_LEVEL);

BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOCATION_METHOD_GNSS) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_CELLULAR) ||
	IS_ENABLED(CONFIG_LOCATION_METHOD_WLAN),
	"At least one location method must be enabled");

/** @brief Semaphore protecting the use of location requests. */
K_SEM_DEFINE(loc_core_sem, 1, 1);

int location_init(location_event_handler_t handler)
{
	int err;
	static bool initialized;

	if (initialized) {
		/* Already initialized */
		return -EPERM;
	}

	err = loc_core_init(handler);
	if (err) {
		return err;
	}

	initialized = true;

	LOG_DBG("Location library initialized");

	return 0;
}

int location_request(const struct loc_config *config)
{
	int err;
	struct loc_config default_config = { 0 };
	struct loc_method_config methods[2] = { 0 };

	/* Go to default config handling if no config given or if no methods given */
	if (config == NULL || config->methods_count == 0) {

		loc_config_defaults_set(&default_config, 2, methods);
		loc_config_method_defaults_set(&methods[0], LOC_METHOD_GNSS);
		loc_config_method_defaults_set(&methods[1], LOC_METHOD_CELLULAR);

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

	loc_core_config_log(config);

	err = loc_core_validate_params(config);
	if (err) {
		LOG_ERR("Invalid parameters given.");
		return err;
	}

	err = k_sem_take(&loc_core_sem, K_SECONDS(1));
	if (err) {
		LOG_ERR("Location request already ongoing");
		return -EBUSY;
	}

	err = loc_core_location_get(config);

	return err;
}

int location_request_cancel(void)
{
	int err;

	err = loc_core_cancel();
	return err;
}

void loc_config_defaults_set(
	struct loc_config *config,
	uint8_t methods_count,
	struct loc_method_config *methods)
{
	memset(config, 0, sizeof(struct loc_config));
	if (methods_count > 0) {
		memset(methods, 0, sizeof(struct loc_method_config) * methods_count);
		config->methods = methods;
	}

	config->methods_count = methods_count;
	config->interval = 0;
}

void loc_config_method_defaults_set(struct loc_method_config *method, enum loc_method method_type)
{
	method->method = method_type;
	if (method_type == LOC_METHOD_GNSS) {
		method->gnss.timeout = 120;
		method->gnss.accuracy = LOC_ACCURACY_NORMAL;
		method->gnss.num_consecutive_fixes = 2;
	} else if (method_type == LOC_METHOD_CELLULAR) {
		method->cellular.timeout = 30;
	} else if (method_type == LOC_METHOD_WLAN) {
		method->wlan.timeout = 30;
	}
}
