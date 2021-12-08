/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ACCEL_EVENT_H__
#define ACCEL_EVENT_H__

#include <zephyr.h>
#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#include "accelerometer.h"

#ifdef __cplusplus
extern "C" {
#endif

struct accel_event {
	struct event_header header;

	struct accelerometer_sensor_data data;
	enum accel_orientation_state orientation;
};

EVENT_TYPE_DECLARE(accel_event);

#ifdef __cplusplus
}
#endif

#endif /* ACCEL_EVENT_H__ */
