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

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ACCELEROMETER_AXIS_COUNT 3

/** @brief Sensor event types submitted by the Sensor module. */
enum sensor_module_event_type {
	/** Accelerometer reported activity.
	 *  Acceleration exceeded the configured activity threshold.
	 */
	SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED,

	/** Accelerometer reported inactivity.
	 *  Acceleration stayed below the threshold for a given time.
	 */
	SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED,

	/** Impact detected.
	 *  Payload is of type @ref sensor_module_data (impact).
	 */
	SENSOR_EVT_MOVEMENT_IMPACT_DETECTED,

	/** Environmental sensors have been sampled.
	 *  Payload is of type @ref sensor_module_data (sensors).
	 */
	SENSOR_EVT_ENVIRONMENTAL_DATA_READY,

	/** Battery fuel gauge data has been sampled.
	 *  Payload is of type @ref sensor_module_data (bat).
	 */
	SENSOR_EVT_FUEL_GAUGE_READY,

	/** Environmental sensors are not supported on the current board. */
	SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED,

	/** The sensor module has performed all procedures to prepare for
	 *  a shutdown of the system. The event carries the ID (id) of the module.
	 */
	SENSOR_EVT_SHUTDOWN_READY,

	/** An irrecoverable error has occurred in the cloud module. Error details are
	 *  attached in the event structure.
	 */
	SENSOR_EVT_ERROR
};

/** @brief Structure used to provide environmental data. */
struct sensor_module_data {
	/** Uptime when the data was sampled. */
	int64_t timestamp;
	/** Temperature in Celsius degrees. */
	double temperature;
	/** Humidity in percentage. */
	double humidity;
	/** Atmospheric pressure in kilopascal. */
	double pressure;
	/** BSEC air quality in Indoor-Air-Quality (IAQ) index.
	 *  If -1, the value is not provided.
	 */
	int bsec_air_quality;
};

/** @brief Structure used to provide acceleration data. */
struct sensor_module_accel_data {
	/** Uptime when the data was sampled. */
	int64_t timestamp;
	/** Acceleration in X, Y and Z planes in m/s2. */
	double values[ACCELEROMETER_AXIS_COUNT];
};

/** @brief Structure used to provide impact data. */
struct sensor_module_impact_data {
	/** Uptime when the data was sampled. */
	int64_t timestamp;
	/** Acceleration on impact, measured in G. */
	double magnitude;
};

/** @brief Structure used to provide battery level. */
struct sensor_module_batt_lvl_data {
	/** Uptime when the data was sampled. */
	int64_t timestamp;
	/** Battery level in percentage. */
	int battery_level;
};

/** @brief Sensor module event. */
struct sensor_module_event {
	/** Sensor module application event header. */
	struct app_event_header header;
	/** Sensor module event type. */
	enum sensor_module_event_type type;
	union {
		/** Variable that contains sensor readings. */
		struct sensor_module_data sensors;
		/** Variable that contains acceleration data. */
		struct sensor_module_accel_data accel;
		/** Variable that contains impact data. */
		struct sensor_module_impact_data impact;
		/** Variable that contains battery level data. */
		struct sensor_module_batt_lvl_data bat;
		/** Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		/** Code signifying the cause of error. */
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(sensor_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SENSOR_MODULE_EVENT_H_ */
