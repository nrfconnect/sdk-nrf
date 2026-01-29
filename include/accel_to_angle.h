/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ACCEL_TO_ANGLE_H_
#define ACCEL_TO_ANGLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**@file accel_to_angle.h
 *
 * @brief Library Accel to angle.
 * @defgroup accel_to_angle Accel to angle library
 * @{
 */

/** Number of the accelerometer axes. */
#define ACCEL_TO_ANGLE_AXIS_NUM 3

/** Accelerometer data from 3 axes from the sensor. */
struct accel_to_angle_accel_data {
	double x;
	double y;
	double z;
};

/** Rotation data calculated from accelerometer data. */
struct accel_to_angle_rot_data {
	double pitch;
	double roll;
};

/** Context for the EMA filter.
 * Used internally by the accel_to_angle module.
 * The contents of this struct shall not be modified by the user.
 * This shall be allocated by the user and shall be accessible during the whole time
 * the filter is used.
 */
struct accel_to_angle_ema_ctx {
	struct accel_to_angle_accel_data filtered_data[ACCEL_TO_ANGLE_AXIS_NUM];
	double alpha;
};

/** Filter callback type.
 *
 * The filter function callback is responsible for filtering the provided accelerometer data.
 * The filtered data shall be stored back in the provided data pointer.
 *
 * The filter context pointer is provided for the filter implementation to use.
 *
 * @param[in, out] data Pointer to the accelerometer data to be filtered.
 * @param[in] filter_ctx Pointer to the filter context.
 */
typedef void (*accel_to_angle_filter)(struct accel_to_angle_accel_data *data, void *filter_ctx);

/** Filter cleanup callback type.
 *
 * The filter cleanup function callback is responsible for cleaning up
 * the filter context used by the filter implementation.
 * The callback is called by the accel_to_angle_state_clean() function.
 *
 * This function is optional and may be NULL if no cleanup is needed.
 *
 * @param[in, out] filter_ctx Pointer to the filter context.
 */
typedef void (*accel_to_angle_filter_clean)(void *filter_ctx);

/** Context for the accel_to_angle module.
 * The contents of this struct shall not be modified by the user.
 * This shall be allocated by the user and shall be accessible during the whole time
 * the accel_to_angle module is used.
 */
struct accel_to_angle_ctx {
	accel_to_angle_filter filter_cb;
	accel_to_angle_filter_clean filter_clean_cb;
	void *filter_ctx;
	struct accel_to_angle_rot_data rot_last;
	struct accel_to_angle_rot_data rot_diff;
};

/**
 * @brief Initialize Accel to angle library.
 *
 * The function initializes the accel_to_angle context.
 * The filter use is not set by default.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int accel_to_angle_init(struct accel_to_angle_ctx *ctx);

/**
 * @brief Set a custom filter.
 *
 * The function sets a custom filter for the accel_to_angle module.
 * The filter function callback is called converting accelerometer data
 * to floating point values in g units.
 *
 * The filter context pointer is passed to the filter callback when called.
 * The filter context pointer can be NULL if not needed by the filter implementation.
 * The filter cleanup callback is called when accel_to_angle_state_clean() is called.
 * The filter cleanup callback can be NULL if no cleanup is needed.
 *
 * In order to disable filtering, set the filter callback to NULL.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 * @param[in] filter_cb Pointer to the filter callback function.
 * @param[in] filter_clean_cb Pointer to the filter cleanup callback function.
 * @param[in] filter_ctx Pointer to context for the filter.
 */
void accel_to_angle_custom_filter_set(struct accel_to_angle_ctx *ctx,
				      accel_to_angle_filter filter_cb,
				      accel_to_angle_filter_clean filter_clean_cb,
				      void *filter_ctx);

/**
 * @brief Set Exponential Moving Average (EMA) filter.
 *
 * The function sets an EMA filter for the accel_to_angle module.
 * The filter context struct shall be allocated by the user.
 * The filter context shall be accessible during the whole time the filter is used.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 * @param[in, out] filter_ctx Pointer to the EMA filter context.
 * @param[in] odr_hz Output data rate of the accelerometer in Hz.
 * @param[in] tau_s Time constant of the EMA filter in seconds.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int accel_to_angle_ema_filter_set(struct accel_to_angle_ctx *ctx,
				  struct accel_to_angle_ema_ctx *filter_ctx,
				  double odr_hz,
				  double tau_s);

/**
 * @brief Calculate rotation from accelerometer data.
 *
 * The function calculates rotation (pitch and roll) from accelerometer data.
 * The accelerometer data is provided as an array the size of ACCEL_TO_ANGLE_AXIS_NUM.
 * The order of the axes in the array is X, Y, Z.
 * The calculated rotation is stored in the provided accel_to_angle_rot_data struct.
 *
 * If a filter is set in the accel_to_angle context, it is applied to the
 * accelerometer data before calculating the rotation.
 * Otherwise, the raw accelerometer data is used.
 *
 * The output of the function is in degrees.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 * @param[in] vals Array of sensor values containing accelerometer data for X, Y, Z axes.
 * @param[out] rot Pointer to the rotation data struct to store the calculated rotation.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int accel_to_angle_calc(struct accel_to_angle_ctx *ctx,
			struct sensor_value vals[ACCEL_TO_ANGLE_AXIS_NUM],
			struct accel_to_angle_rot_data *rot);

/**
 * @brief Check for significant rotation change.
 *
 * The function checks if the change in rotation exceeds the specified threshold
 * on at least the specified number of axes.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 * @param[in] rot Pointer to the current rotation data.
 * @param[in] rot_threshold_deg Rotation threshold in degrees.
 * @param[in] axis_threshold_num Number of axes that must exceed the threshold to consider
 *                               the rotation change significant.
 *
 * @retval true If the rotation change exceeds the threshold on the specified number of axes.
 * @retval false Otherwise.
 */
bool accel_to_angle_diff_check(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_rot_data *rot,
			       double rot_threshold_deg,
			       size_t axis_threshold_num);

/**
 * @brief Clean the internal state of the accel_to_angle module.
 *
 * The function resets the last rotation and cumulative difference rotation
 * stored in the accel_to_angle context.
 *
 * This function also calls the filter cleanup callback if it is set.
 *
 * The function can be used to reinitialize the state of rotation change detection.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 */
void accel_to_angle_state_clean(struct accel_to_angle_ctx *ctx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ACCEL_TO_ANGLE_H_ */
