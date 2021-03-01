/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

/**
 * @brief Sensor Event
 * @defgroup sensor_event Sensor Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Sensor event. */
struct sensor_event {
	struct event_header header; /**< Event header. */

	const char *descr; /**< Description of the sensor event. */
	struct event_dyndata dyndata; /**< Sensor data. Provided as floating-point values. */
};

EVENT_TYPE_DYNDATA_DECLARE(sensor_event);

/** @brief Get size of sensor data.
 *
 * @param[in] event       Pointer to the sensor_event.
 *
 * @return Size of the sensor data, expressed as a number of floating-point values.
 */
static inline size_t sensor_event_get_data_cnt(const struct sensor_event *event)
{
	__ASSERT_NO_MSG((event->dyndata.size % sizeof(float)) == 0);

	return (event->dyndata.size / sizeof(float));
}

/** @brief Get pointer to the sensor data.
 *
 * @param[in] event       Pointer to the sensor_event.
 *
 * @return Pointer to the sensor data.
 */
static inline float *sensor_event_get_data_ptr(const struct sensor_event *event)
{
	return (float *)event->dyndata.data;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SENSOR_EVENT_H_ */
