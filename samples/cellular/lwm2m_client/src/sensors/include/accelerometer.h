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

/**@brief Struct containing current orientation and 3 axis acceleration data. */
struct accelerometer_sensor_data {
	struct sensor_value x; /**< x-axis acceleration [m/s^2]. */
	struct sensor_value y; /**< y-axis acceleration [m/s^2]. */
	struct sensor_value z; /**< z-axis acceleration [m/s^2]. */
};

int accelerometer_read(struct accelerometer_sensor_data *data);

int accelerometer_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ACCELEROMETER_H__ */
