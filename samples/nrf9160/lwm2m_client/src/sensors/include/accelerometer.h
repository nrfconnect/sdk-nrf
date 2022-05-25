/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ACCELEROMETER_H__
#define ACCELEROMETER_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Orientation states. */
enum accel_orientation_state {
	ORIENTATION_NOT_KNOWN, /**< Initial state. */
	ORIENTATION_NORMAL, /**< Has normal orientation. */
	ORIENTATION_UPSIDE_DOWN, /**< System is upside down. */
	ORIENTATION_ON_SIDE /**< System is placed on its side. */
};

/**@brief Struct containing current orientation and 3 axis acceleration data. */
struct accelerometer_sensor_data {
	struct sensor_value x; /**< x-axis acceleration [m/s^2]. */
	struct sensor_value y; /**< y-axis acceleration [m/s^2]. */
	struct sensor_value z; /**< z-axis acceleration [m/s^2]. */
	enum accel_orientation_state orientation; /**< Current orientation. */
};

int accelerometer_read(struct accelerometer_sensor_data *data);

int accelerometer_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ACCELEROMETER_H__ */
