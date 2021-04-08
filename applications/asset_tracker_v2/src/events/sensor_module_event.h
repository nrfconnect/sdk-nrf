/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_MODULE_EVENT_H_
#define _SENSOR_MODULE_EVENT_H_

/**
 * @brief Sensor module event
 * @defgroup sensor_module_event Sensor module event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACCELEROMETER_AXIS_COUNT 3

/** @brief Sensor event types su bmitted by Sensor module. */
enum sensor_module_event_type {
	SENSOR_EVT_MOVEMENT_DATA_READY,
	SENSOR_EVT_ENVIRONMENTAL_DATA_READY,
	SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED,
	SENSOR_EVT_SHUTDOWN_READY,
	SENSOR_EVT_ERROR
};

struct sensor_module_data {
	int64_t timestamp;
	double temperature;
	double humidity;
};

struct sensor_module_accel_data {
	int64_t timestamp;
	double values[ACCELEROMETER_AXIS_COUNT];
};

/** @brief Sensor event. */
struct sensor_module_event {
	struct event_header header;
	enum sensor_module_event_type type;
	union {
		struct sensor_module_data sensors;
		struct sensor_module_accel_data accel;
		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		int err;
	} data;
};

EVENT_TYPE_DECLARE(sensor_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SENSOR_MODULE_EVENT_H_ */
