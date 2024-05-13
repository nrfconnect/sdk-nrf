/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <caf/events/sensor_data_aggregator_event.h>

#define module da_release

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_DATA_AGGREGATOR_RELEASE_LOG_LEVEL);


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_data_aggregator_event(aeh)) {
		const struct sensor_data_aggregator_event *event =
			cast_sensor_data_aggregator_event(aeh);
		struct sensor_data_aggregator_release_buffer_event *release_evt;

		release_evt = new_sensor_data_aggregator_release_buffer_event();
		release_evt->samples = event->samples;
		release_evt->sensor_descr = event->sensor_descr;

		APP_EVENT_SUBMIT(release_evt);
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, sensor_data_aggregator_event);
