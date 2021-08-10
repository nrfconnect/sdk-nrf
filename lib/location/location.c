/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <nrf_modem_gnss.h>
#include <logging/log.h>
#include <modem/location.h>
#include "method_gnss.h"
#include "location.h"

LOG_MODULE_REGISTER(location, CONFIG_LOCATION_LOG_LEVEL);

location_event_handler_t event_handler;
struct loc_event_data event_data;

extern struct k_work gnss_fix_work;
extern struct k_work gnss_timeout_work;

void event_data_init(enum loc_event_id event_id, enum loc_method method)
{
	memset(&event_data, 0, sizeof(event_data));

	event_data.id = event_id;
	event_data.method = method;
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

	err = nrf_modem_gnss_init();
	if (err) {
		LOG_ERR("Failed to initialize GNSS interface, error %d", err);
		return -err;
	}

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return -err;
	}

	k_work_init(&gnss_fix_work, gnss_fix_work_fn);
	k_work_init(&gnss_timeout_work, gnss_timeout_work_fn);

	initialized = true;

	LOG_DBG("Library initialized");

	return 0;
}

int location_request(const struct loc_config *config)
{
	int err;
	struct loc_method_config selected_method = { 0 };

	if ((config->interval > 0) && (config->interval < 10)) {
		LOG_ERR("Interval for periodic location updates must be 10...65535 seconds.");
		return -EINVAL;
	}

	for (int i = 0; i < LOC_MAX_METHODS; i++) {
		switch (config->methods[i].method) {
		case LOC_METHOD_CELL_ID:
			LOG_DBG("Cell ID not yet supported");
			break;

		case LOC_METHOD_GNSS:
			LOG_DBG("GNSS selected");
			selected_method = config->methods[i];
			break;

		default:
			break;
		}

		if (selected_method.method) {
			continue;
		}
	}

	if (!selected_method.method) {
		LOG_ERR("No location method found");
		return -EINVAL;
	}

	/* TODO: Add protection so that only one request is handled at a time */

	err = gnss_configure_and_start(&selected_method.config.gnss, config->interval);

	return err;
}

int location_request_cancel(void)
{
	return -1;
}
