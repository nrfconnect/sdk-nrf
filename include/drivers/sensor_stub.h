/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_STUB_H_
#define _SENSOR_STUB_H_

/**
 * @file sensor_stub.h
 *
 * @brief Public API for the sensor_stub driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>

/**
 * @defgroup sensor_stub Simple simulated sensor driver.
 * @{
 * @brief Simple simulated sensor device driver.
 *
 * The simulator redirects the _fetch and _get functions from the implemented sensor
 * directly to the user functions.
 */

/**
 * @brief Get user data
 *
 * The data pointer to free usage by generator functions.
 *
 * @param dev Device handler.
 * @return The data pointer.
 */
void *sensor_stub_udata_get(const struct device *dev);

/**
 * @brief Set user data
 *
 * Set the user data to be used by generator functions.
 *
 * @param dev   Device handler.
 * @param udata User data pointer.
 */
void sensor_stub_udata_set(const struct device *dev, void *udata);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_STUB_H_ */
