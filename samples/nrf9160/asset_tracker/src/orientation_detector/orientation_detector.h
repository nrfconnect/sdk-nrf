/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 *
 * @brief   Orientation detector module.
 *
 * Module that uses accelerometer data to detect its orientation.
 */

#ifndef ORIENTATION_DETECTOR_H__
#define ORIENTATION_DETECTOR_H__

#include <sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Orientation states. */
enum orientation_state {
	ORIENTATION_NOT_KNOWN,   /**< Initial state. */
	ORIENTATION_NORMAL,      /**< Has normal orientation. */
	ORIENTATION_UPSIDE_DOWN, /**< System is upside down. */
	ORIENTATION_ON_SIDE      /**< System is placed on its side. */
};


/**@brief Struct containing current orientation and 3 axis acceleration data. */
struct orientation_detector_sensor_data {
	double x;			    /**< X-axis acceleration [m/s^2]. */
	double y;			    /**< y-axis acceleration [m/s^2]. */
	double z;			    /**< z-axis acceleration [m/s^2]. */
	enum orientation_state orientation; /**< Current orientation. */
};


/**@brief Initializes the orientation detector.
 *
 * @param[in] accel_device	Pointer to sensor device structure
 * providing data on accelerometer sensor channels.
 */
void orientation_detector_init(struct device *accel_device);


/**@brief Function to poll the orientation detector.
 *
 * @param[in] sensor_data        Pointer to structure where the orientation data
 * is stored.
 *
 * @return 0 if the operation is successful, negative error code otherwise.
 */
int orientation_detector_poll(
	struct orientation_detector_sensor_data *sensor_data);


/**@brief Determines and stores the static offset of the accelerometer that's
 * used to infer device orientation. The device has to be placed on a flat
 * surface facing up when this function is called.
 *
 * @return 0 if the operation is successful, negative error code otherwise.
 */
int orientation_detector_calibrate(void);

#ifdef __cplusplus
}
#endif

#endif /* ORIENTATION_DETECTOR_H__ */
