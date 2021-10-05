/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SENSOR_EVENT_H__
#define SENSOR_EVENT_H__

#include <zephyr.h>
#include <net/lwm2m.h>
#include <event_manager.h>
#include <event_manager_profiler.h>
#include <drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_type {
	LIGHT_SENSOR,
	COLOUR_SENSOR,
	TEMPERATURE_SENSOR,
	HUMIDITY_SENSOR,
	PRESSURE_SENSOR,
	GAS_RESISTANCE_SENSOR
};

struct sensor_event {
	struct event_header header;

	enum sensor_type type;
	struct sensor_value sensor_value;
	uint32_t unsigned_value;
};

EVENT_TYPE_DECLARE(sensor_event);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_EVENT_H__ */
