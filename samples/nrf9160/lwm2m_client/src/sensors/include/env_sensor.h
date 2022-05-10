/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ENV_SENSOR_H__
#define ENV_SENSOR_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read the current temperature. Unit: [degree C].
 *
 * @param[out] temp_val Pointer to sensor_value struct where the temperature
 *             value is stored.
 * @return int 0 if successful, negative error code if not.
 */
int env_sensor_read_temperature(struct sensor_value *temp_val);

/**
 * @brief Read the current air pressure. Unit: [kPa].
 *
 * @param[out] press_val Pointer to sensor_value struct where the air pressure
 *             value is stored.
 * @return int 0 if successful, negative error code if not.
 */
int env_sensor_read_pressure(struct sensor_value *press_val);

/**
 * @brief Read the current air humidity. Unit: [%]
 *
 * @param[out] humid_val Pointer to sensor_value struct where the air humidity
 *             value is stored.
 * @return int 0 if successful, negative error code if not.
 */
int env_sensor_read_humidity(struct sensor_value *humid_val);

/**
 * @brief Read the current gas resistance. Can be used as a
 *        measure for air quality. Unit: [Ohm].
 *
 * @param[out] gas_res_val Pointer to sensor_value struct where the gas
 *             resistance value is stored.
 * @return int 0 if successful, negative error code if not.
 */
int env_sensor_read_gas_resistance(struct sensor_value *gas_res_val);

/**
 * @brief Initialize the environment sensor.
 *
 * @return int 0 if successful, negative error code if not.
 */
int env_sensor_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENV_SENSOR_H__ */
