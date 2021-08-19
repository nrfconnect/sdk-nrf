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

	err = k_sem_take(&loc_core_sem, K_SECONDS(1));
	if (err) {
		LOG_ERR("Location request already ongoing");
		return -EBUSY;
	}

	err = loc_core_validate_params(config);
	if (err) {
		LOG_ERR("Invalid parameters given.");
		return err;
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
