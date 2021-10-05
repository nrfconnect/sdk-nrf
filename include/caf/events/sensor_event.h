/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

/**
 * @file
 * @defgroup caf_sensor_event CAF Sensor Event
 * @{
 * @brief CAF Sensor Event.
 */

#include <event_manager.h>
#include <event_manager_profiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sensor states. */
enum sensor_state {
	/** Initial state of the sensor. The state is used only before sensor is initialized and
	 *  it should not be broadcasted using @ref sensor_state_event.
	 */
	SENSOR_STATE_DISABLED,

	/** Sensor is sleeping and no sampling is performed. */
	SENSOR_STATE_SLEEP,

	/** Sensor is sampled periodically. */
	SENSOR_STATE_ACTIVE,

	/** Sensor error occurred. The sensor is no longer sampled. */
	SENSOR_STATE_ERROR,

	/** Number of sensor states. */
	SENSOR_STATE_COUNT
};

/** @brief Sensor state event.
 *
 * The sensor state event is submitted when state of a sensor changes.
 *
 * The description field is a pointer to a string that is used to identify the sensor by the
 * application. The Common Application Framework does not impose any standard way of describing
 * sensors. Format and content of the sensor description is defined by the application.
 *
 * @warning The sensor state event related to the given sensor must use the same description as
 *          #sensor_event related to the sensor.
 */
struct sensor_state_event {
	struct event_header header; /**< Event header. */

	const char *descr; /**< Description of the sensor. */
	enum sensor_state state; /**< New state of the sensor. */
};

EVENT_TYPE_DECLARE(sensor_state_event);

/** @brief Sensor event.
 *
 * The sensor event is submitted when a sensor is sampled.
 *
 * The description field is a pointer to a string that is used to identify the sensor by the
 * application. The Common Application Framework does not impose any standard way of describing
 * sensors. Format and content of the sensor description is defined by the application.
 *
 * The dyndata contains sensor readouts represented as array of floating-point values. Content of
 * the array depends only on selected sensor. For example an accelerometer may report acceleration
 * in X, Y and Z axis as three floating-point values. @ref sensor_event_get_data_cnt and @ref
 * sensor_event_get_data_ptr can be used to access the sensor data provided by a given sensor event.
 *
 * @warning The sensor event related to the given sensor must use the same description as
 *          #sensor_state_event related to the sensor.
 */
struct sensor_event {
	struct event_header header; /**< Event header. */

	const char *descr; /**< Description of the sensor. */
	struct event_dyndata dyndata; /**< Sensor data. Provided as floating-point values. */
};

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

#ifdef __cplusplus
extern "C" {
#endif

EVENT_TYPE_DYNDATA_DECLARE(sensor_event);

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_EVENT_H_ */
