/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_TEMP_SENSOR_H_
#define MOCK_TEMP_SENSOR_H_

#include <zephyr/drivers/sensor.h>
#include <stdbool.h>

/**
 * @brief Set the mock temperature sensor value.
 *
 * @param val Pointer to sensor_value to return on channel_get
 */
void mock_temp_sensor_set_value(struct sensor_value *val);

/**
 * @brief Set the result code for sample_fetch operations.
 *
 * @param result Error code to return from sample_fetch (0 for success)
 */
void mock_temp_sensor_set_fetch_result(int result);

/**
 * @brief Set the result code for channel_get operations.
 *
 * @param result Error code to return from channel_get (0 for success)
 */
void mock_temp_sensor_set_channel_get_result(int result);

#endif /* MOCK_TEMP_SENSOR_H_ */
