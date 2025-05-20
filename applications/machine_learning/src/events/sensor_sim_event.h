/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Simulated Sensor event header file.
 */

#ifndef _SENSOR_SIM_EVENT_H_
#define _SENSOR_SIM_EVENT_H_

/**
 * @brief Simulated Sensor Event
 * @defgroup sensor_sim_event Simulated Sensor Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Simulated sensor event. */
struct sensor_sim_event {
	/** Event header. */
	struct app_event_header header;

	/** Label of generated signal. */
	const char *label;
};

APP_EVENT_TYPE_DECLARE(sensor_sim_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SENSOR_SIM_EVENT_H_ */
