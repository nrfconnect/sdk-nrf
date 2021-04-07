/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _SENSOR_SIM_EVENT_H_
#define _SENSOR_SIM_EVENT_H_

/**
 * @brief Simulated Sensor Event
 * @defgroup sensor_sim_event Simulated Sensor Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Simulated sensor event. */
struct sensor_sim_event {
	struct event_header header; /**< Event header. */

	const char *label; /**< Label of generated signal. */
};

EVENT_TYPE_DECLARE(sensor_sim_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SENSOR_SIM_EVENT_H_ */
