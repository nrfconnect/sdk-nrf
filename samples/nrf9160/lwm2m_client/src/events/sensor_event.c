/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor_event.h"

#include <stdio.h>

static const char *sensor_type_to_string(enum sensor_type type)
{
	switch (type) {
	case TEMPERATURE_SENSOR:
		return "Temperature sensor";

	case PRESSURE_SENSOR:
		return "Pressure sensor";

	case HUMIDITY_SENSOR:
		return "Humidity sensor";

	case GAS_RESISTANCE_SENSOR:
		return "Gas resistance sensor";

	case LIGHT_SENSOR:
		return "Light sensor";

	case COLOUR_SENSOR:
		return "Colour sensor";

	default:
		return "";
	}
}

static void log_sensor_event(const struct app_event_header *aeh)
{
	struct sensor_event *event = cast_sensor_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh,
			"%s sensor event: sensor_value: val1 = %d, val2 = %d; unsigned_value = %d",
			sensor_type_to_string(event->type), event->sensor_value.val1,
			event->sensor_value.val2, event->unsigned_value);
}

APP_EVENT_TYPE_DEFINE(sensor_event,
			      log_sensor_event,
			      NULL,
			      APP_EVENT_FLAGS_CREATE());
