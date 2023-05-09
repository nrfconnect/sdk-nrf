/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *@brief External sensors library header.
 */

#ifndef EXT_SENSORS_H__
#define EXT_SENSORS_H__

/**@file
 *
 * @defgroup External sensors ext_sensors
 * @brief    Module that manages external sensors.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Number of accelerometer channels. */
#define ACCELEROMETER_CHANNELS 3

/** @brief Enum containing callback events from library. */
enum ext_sensor_evt_type {
	/** Event that is sent if acceleration is detected */
	EXT_SENSOR_EVT_ACCELEROMETER_ACT_TRIGGER,
	/** Event that is sent if inactivity is detected */
	EXT_SENSOR_EVT_ACCELEROMETER_INACT_TRIGGER,
	/** ADXL372 high-G accelerometer */
	EXT_SENSOR_EVT_ACCELEROMETER_IMPACT_TRIGGER,

	/** Event propagated when an error has occurred with any of the accelerometers. */
	EXT_SENSOR_EVT_ACCELEROMETER_ERROR,
	/** Event propagated when an error has occurred with the temperature sensor. */
	EXT_SENSOR_EVT_TEMPERATURE_ERROR,
	/** Event propagated when an error has occurred with the humidity sensor. */
	EXT_SENSOR_EVT_HUMIDITY_ERROR,
	/** Event propagated when an error has occurred with the pressure sensor. */
	EXT_SENSOR_EVT_PRESSURE_ERROR,
	/** Event propagated when an error has occurred with the virtual air quality sensor. */
	EXT_SENSOR_EVT_AIR_QUALITY_ERROR
};

/** @brief Structure containing external sensor data. */
struct ext_sensor_evt {
	/** Sensor type. */
	enum ext_sensor_evt_type type;
	/** Event data. */
	union {
		/** Array of external sensor values. */
		double value_array[ACCELEROMETER_CHANNELS];
		/** Single external sensor value. */
		double value;
	};
};

/** @brief External sensors library asynchronous event handler.
 *
 *  @param[in] evt The event and any associated parameters.
 */
typedef void (*ext_sensor_handler_t)(
	const struct ext_sensor_evt *const evt);

/**
 * @brief Initializes the library, sets callback handler.
 *
 * @param[in] handler Pointer to callback handler.
 *
 * @return 0 on success or negative error value on failure.
 */
int ext_sensors_init(ext_sensor_handler_t handler);

/**
 * @brief Get temperature from library.
 *
 * @param[out] temp Pointer to variable containing temperature in celcius.
 *
 * @return 0 on success or negative error value on failure.
 */
int ext_sensors_temperature_get(double *temp);

/**
 * @brief Get humidity from library.
 *
 * @param[out] humid Pointer to variable containing humidity in percentage.
 *
 * @return 0 on success or negative error value on failure.
 */
int ext_sensors_humidity_get(double *humid);

/**
 * @brief Get pressure from library.
 *
 * @param[out] press Pointer to variable containing atmospheric pressure in kilopascal.
 *
 * @return 0 on success or negative error value on failure.
 */
int ext_sensors_pressure_get(double *press);

/**
 * @brief Get air quality. Air quality calculations are only available when enabling
 *	  the Bosch BSEC library.
 *
 * @param[out] bsec_air_quality Pointer to variable containing air quality reading in
 *			        Indoor-Air-Quality (IAQ). This number is calculated by internal
 *			        algorithms in the Bosch BSEC library and is a product of multiple
 *			        readings from several sensors over time.
 *			        The IAQ value ranges from 0 (clean air) to 500
 *				(heavily polluted air)
 *
 * @return 0 on success or negative error value on failure.
 * @retval -ENOTSUP if getting air quality is not supported.
 */
int ext_sensors_air_quality_get(uint16_t *bsec_air_quality);

/**
 * @brief Set the threshold that triggers callback on accelerometer data.
 *
 * @param[in] threshold Variable that sets the accelerometer threshold value
 *				in m/s2. Must be a value larger than 0 and smaller than the
 *				configured range (default 2G: ~= 19.6133 m/s2).
 * @param[in] upper Flag indicating if the given threshold is for
 *				activity detection (true) or inactivity detection (false).
 *
 * @return 0 on success or negative error value on failure.
 */
int ext_sensors_accelerometer_threshold_set(double threshold, bool upper);

/**
 * @brief Set the timeout for the inactivity detection of the accelerometer.
 *
 * @param[in] inact_time Variable that sets the accelerometer inactivity timeout
 *				in s. Must be a value larger than 0 and smaller than the
 *				configured range (default ODR=12.5Hz, max timeout: 5242.880)
 *
 */
int ext_sensors_inactivity_timeout_set(double inact_time);

/**
 * @brief Enable or disable accelerometer trigger handler.
 *
 * @param[in] enable Flag that enables or disables callback triggers from the accelerometer.
 *
 * @return 0 on success or negative error value on failure.
 */
int ext_sensors_accelerometer_trigger_callback_set(bool enable);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif
