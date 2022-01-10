/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_FORCE_ACTIVE_EVENT_H_
#define _SENSOR_FORCE_ACTIVE_EVENT_H_

/**
 * @file
 * @defgroup caf_sensor_force_active_event CAF Sensor force active event
 * @{
 * @brief CAF Sensor event to force it sampling even if idle state is detected.
 */

#include <event_manager.h>

/**
 * @brief Force sensor to stay active
 *
 * The event received by sensor manager that forbids the sensor
 * to stop sampling when the idle state is detected.
 */
struct sensor_force_active_event {
	/** Event header. */
	struct event_header header;
	/** Description of the sensor. */
	const char *descr;
	/** Force the sensor to be active */
	uint8_t force_active;
};

EVENT_TYPE_DECLARE(sensor_force_active_event);

/**
 * @brief Force the given sensor to stay in active mode
 *
 * Function submit an event that forces the given sensor to stay in active mode.
 * @param descr        The description of the sensor.
 * @param force_active Force the sensor to stay active or release it.
 */
static inline void sensor_force_active(const char *descr, uint8_t force_active)
{
	struct sensor_force_active_event *event = new_sensor_force_active_event();

	event->descr = descr;
	event->force_active = force_active;
	EVENT_SUBMIT(event);
}

/** @} */
#endif /* _SENSOR_FORCE_ACTIVE_EVENT_H_ */
