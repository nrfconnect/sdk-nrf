/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _APP_SENSOR_EVENT_H_
#define _APP_SENSOR_EVENT_H_

/**
 * @file
 * @defgroup app_sensor_event App Sensor Event
 * @{
 * @brief App Sensor Event.
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sensor start event. */
struct sensor_start_event {
	struct app_event_header header; /**< Event header. */

	const char *descr; /**< Description of the sensor. */
	uint32_t delay; /**< Initial sampling delay in milliseconds. */
	uint32_t period; /**< Sampling period in milliseconds. */
};

APP_EVENT_TYPE_DECLARE(sensor_start_event);


/** @brief Sensor stop event. */
struct sensor_stop_event {
	struct app_event_header header; /**< Event header. */

	const char *descr; /**< Description of the sensor. */
};

APP_EVENT_TYPE_DECLARE(sensor_stop_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _APP_SENSOR_EVENT_H_ */
