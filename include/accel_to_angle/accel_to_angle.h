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

/**
 * @brief Define an accel_to_angle context instance.
 *
 * The macro defines an accel_to_angle context instance with the specified filter.
 *
 * The filter pointer can be NULL if filtering is not required.
 * Use ACCEL_TO_ANGLE_FILTER_DEFINE macro to define the filter configuration.
 *
 * @param _instance_name Name of the accel_to_angle context instance.
 * @param _filter Pointer to the filter struct.
 */
#define ACCEL_TO_ANGLE_CTX_DEFINE(_instance_name, _filter)	\
	struct accel_to_angle_ctx _instance_name = {		\
		.filter = _filter,				\
		.pr_last = {0},					\
		.pr_diff = {0},					\
	}

/**
 * @brief Define a filter configuration instance.
 *
 * The macro defines a filter configuration instance with the specified parameters.
 *
 * The filter context pointer can be NULL if not needed by the filter implementation.
 * The filter process and clean request pointers can be NULL if not needed.
 *
 * @param _instance_name Name of the filter instance.
 * @param _ctx Pointer to the filter context.
 * @param _process_request Pointer to the data process request callback function.
 * @param _clean_request Pointer to the data clean request callback function.
 */
#define ACCEL_TO_ANGLE_FILTER_DEFINE(_instance_name, _ctx, _process_request, _clean_request)	\
	struct accel_to_angle_filter _instance_name = {						\
		.ctx = _ctx,									\
		.data_process_request = _process_request,					\
		.data_clean_request = _clean_request						\
	}

/**
 * @brief Accelerometer data from 3 axes from the sensor.
 */
struct accel_to_angle_accel_data {
	float x;
	float y;
	float z;
};

/**
 * @brief Pitch and roll data calculated from accelerometer data.
 */
struct accel_to_angle_pr_data {
	float pitch;
	float roll;
};

/**
 * @brief Filter configuration for the accel_to_angle module.
 *
 * The filter struct shall be allocated by the user and
 * shall be accessible during the whole time the accel_to_angle module is used.
 *
 * Every field is optional and can be set to NULL if not needed.
 */
struct accel_to_angle_filter {
	void *ctx;
	void (*data_process_request)(struct accel_to_angle_accel_data *data, void *filter_ctx);
	void (*data_clean_request)(void *filter_ctx);
};

/**
 * @brief Context for the accel_to_angle module.
 *
 * The contents of this struct shall not be modified by the user.
 * This shall be allocated by the user and shall be accessible during the whole time
 * the accel_to_angle module is used.
 */
struct accel_to_angle_ctx {
	struct accel_to_angle_filter *filter;
	struct accel_to_angle_pr_data pr_last;
	struct accel_to_angle_pr_data pr_diff;
};

/**
 * @brief Set a filter.
 *
 * The function sets a filter for the accel_to_angle module.
 *
 * The filter context pointer is passed to the filter callbacks when called.
 * The filter context pointer can be NULL if not needed by the filter implementation.
 *
 * The filter @ref accel_to_angle_filter.data_process_request callback is called after
 * converting accelerometer data to floating point values in g units.
 *
 * The filter @ref accel_to_angle_filter.data_clean_request callback is called when
 * @ref accel_to_angle_state_clean() is called.
 * The filter @ref accel_to_angle_filter.data_clean_request callback can be NULL if no cleanup
 * is needed.
 *
 * The filter struct shall be allocated by the user.
 * The filter struct shall be accessible during the whole time the library is used.
 * The filter struct can be NULL to disable filtering.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 * @param[in] filter Pointer to the filter configuration struct.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int accel_to_angle_filter_set(struct accel_to_angle_ctx *ctx,
			      struct accel_to_angle_filter *filter);

/**
 * @brief Calculate pitch and roll angles from accelerometer data.
 *
 * The function calculates pitch and roll angles from accelerometer data.
 * The accelerometer data is provided as an array the size of ACCEL_TO_ANGLE_AXIS_NUM.
 * The order of the axes in the array is X, Y, Z.
 * The calculated pitch and roll angles are stored in the provided @ref accel_to_angle_pr_data
 * struct.
 *
 * If a filter is set in the accel_to_angle context, it is applied to the
 * accelerometer data before calculating the rotation.
 * Otherwise, the raw accelerometer data is used.
 *
 * The output of the function is in degrees.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 * @param[in] vals Array of sensor values containing accelerometer data for X, Y, Z axes.
 * @param[out] pr Pointer to the pr data struct to store the calculated rotation.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int accel_to_angle_calc(struct accel_to_angle_ctx *ctx,
			struct sensor_value vals[ACCEL_TO_ANGLE_AXIS_NUM],
			struct accel_to_angle_pr_data *pr);

/**
 * @brief Check for significant rotation change.
 *
 * The function checks if the change in rotation exceeds the specified threshold
 * on at least the specified number of axes.
 *
 * @param[in, out] ctx Pointer to the accel_to_angle context.
 * @param[in] pr Pointer to the current pitch and roll data.
 * @param[in] pr_threshold_deg Rotation threshold in degrees.
 * @param[in] axis_threshold_num Number of axes that must exceed the threshold to consider
 *                               the rotation change significant.
 *
 * @retval true If the rotation change exceeds the threshold on the specified number of axes.
 * @retval false Otherwise.
 */
bool accel_to_angle_diff_check(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_pr_data *pr,
			       float pr_threshold_deg,
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
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int accel_to_angle_state_clean(struct accel_to_angle_ctx *ctx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ACCEL_TO_ANGLE_H_ */
