/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ENV_SENSORS_H_
#define ENV_SENSORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file env_sensors.h
 * @defgroup env_sensors ENV_SENSORS Environmental sensors interface¨
 * @{
 * @brief Module for interfacing environmental sensors for asset tracker
 *
 * @details Basic basic for interfacing environmental sensors for the
 *          asset tracker application.
 *          Supported sensor types are Temperature, Humidity, Pressure and
 *          air quality sensors.
 *
 */

#include <zephyr/types.h>


/** @brief Environmental sensor types supported */
typedef enum {
	/** Temperature sensor. */
	ENV_SENSOR_TEMPERATURE,
	/** The Humidity sensor. */
	ENV_SENSOR_HUMIDITY,
	/** The Air Pressure sensor. */
	ENV_SENSOR_AIR_PRESSURE,
	/** The Air Quality sensor. */
	ENV_SENSOR_AIR_QUALITY,
} env_sensor_t;

typedef struct {
	/** Sensor type. */
	env_sensor_t type;
	/** Sensor sample value. */
	double value;
} env_sensor_data_t;

/**
 * @brief Get latest sampled temperature data.
 *
 * @param sensor_data Pointer to memory to store latest temperature data.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_get_temperature(env_sensor_data_t *sensor_data);

/**
 * @brief Get latest sampled humidity data.
 *
 * @param sensor_data Pointer to memory to store latest humidity data.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_get_humidity(env_sensor_data_t *sensor_data);

/**
 * @brief Get latest sampled pressure data.
 *
 * @param sensor_data Pointer to memory to store latest pressure data.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_get_pressure(env_sensor_data_t *sensor_data);

/**
 * @brief Get latest sampled air quality data.
 *
 * @param sensor_data Pointer to memory to store latest air quality data.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_get_air_quality(env_sensor_data_t *sensor_data);


/**
 * @brief Initialize and start the environmental sensors.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_init_and_start(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ENV_SENSORS_H_ */
