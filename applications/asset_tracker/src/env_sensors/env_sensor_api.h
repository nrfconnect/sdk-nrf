/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ENV_SENSOR_API_H_
#define ENV_SENSOR_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file env_sensor_api.h
 * @defgroup env_sensor_api ENV_API Environmental sensor api¨
 * @{
 * @brief API for interfacing envirnmental sensors
 *
 * @details Basic api for interfacing environmental sensors.
 *          Supported sensor types are Temperature, Humidity,
 *          Pressure and Air quality sensors.
 *
 */

#include <zephyr/types.h>

/** @brief Environmental sensor types supported by the API */
typedef enum {
	/** Temperature sensor */
	ENV_SENSOR_TEMPERATURE,
	/** The Humidity sensor */
	ENV_SENSOR_HUMIDITY,
	/** The Air Pressure sensor */
	ENV_SENSOR_AIR_PRESSURE,
	/** The Air Quality sensors */
	ENV_SENSOR_AIR_QUALITY,
} env_sensor_t;

typedef struct {
	u64_t id;
	env_sensor_t type;
	double value;
	int timestamp;
} env_sensor_data_t;

int env_sensors_get_temperature(env_sensor_data_t *sensor_data,
				u64_t *sensor_id);

int env_sensors_get_humidity(env_sensor_data_t *sensor_data, u64_t *sensor_id);

int env_sensors_get_pressure(env_sensor_data_t *sensor_data, u64_t *sensor_id);

int env_sensors_get_air_quality(env_sensor_data_t *sensor_data,
				u64_t *sensor_id);

int env_sensors_init(void);

int env_sensors_start_polling(void);

int env_sensors_stop_polling(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ENV_SENSOR_API_H_ */
