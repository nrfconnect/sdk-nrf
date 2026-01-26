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
 * @defgroup accel_to_angle Accel to angle library API
 * @brief Accel to angle library API.
 *
 * This API represents a library that converts the three-dimensional acceleration into
 * the two-dimensional angle change (so-called pitch and roll angles).
 * @{
 */

/** Number of the accelerometer axes. */
#define ACCEL_TO_ANGLE_AXIS_NUM 3

/**
 * @brief Define a context instance that is described by the @ref accel_to_angle_ctx structure.
 *
 * The macro defines an @ref accel_to_angle_ctx context instance with the specified filter.
 * The context instance must be defined, as all API functions of this library require it.
 *
 * The filter pointer can be NULL if filtering is not required.
 * Use the @ref ACCEL_TO_ANGLE_FILTER_DEFINE macro to define the filter instance.
 *
 * @param _instance_name Name of the @ref accel_to_angle_ctx context instance.
 * @param _filter        Pointer to the filter instance described by the
 *                       @ref accel_to_angle_filter structure.
 */
#define ACCEL_TO_ANGLE_CTX_DEFINE(_instance_name, _filter)	\
	struct accel_to_angle_ctx _instance_name = {		\
		.filter = _filter,				\
		.pr_last = {0},					\
		.pr_diff = {0},					\
	}

/**
 * @brief Define a filter instance.
 *
 * The macro defines a filter instance with the specified parameters.
 *
 * The filter context pointer can be NULL if not needed by the filter implementation.
 * The clean request pointer can be NULL if not needed.
 *
 * @param _instance_name   Name of the filter instance.
 * @param _ctx             Pointer to the filter context.
 * @param _process_request Pointer to the data process request callback function.
 * @param _clean_request   Pointer to the data clean request callback function.
 */
#define ACCEL_TO_ANGLE_FILTER_DEFINE(_instance_name, _ctx, _process_request, _clean_request)	\
	BUILD_ASSERT(_process_request != NULL, "No process request callback provided");		\
	struct accel_to_angle_filter _instance_name = {						\
		.ctx = _ctx,									\
		.data_process_request = _process_request,					\
		.data_clean_request = _clean_request						\
	}

/** @brief Accelerometer data from 3 axes from the sensor. */
struct accel_to_angle_accel_data {
	/** @brief Acceleration along the X axis in G units. */
	float x;

	/** @brief Acceleration along the Y axis in G units. */
	float y;

	/** @brief Acceleration along the Z axis in G units. */
	float z;
};

/** @brief Pitch and roll data calculated from accelerometer data. */
struct accel_to_angle_pr_data {
	/** @brief Pitch angle in degrees. */
	float pitch;

	/** @brief Roll angle in degrees. */
	float roll;
};

/**
 * @brief Descriptor of the filter instance.
 *
 * The filter struct shall be allocated by the user and
 * shall be accessible during the whole time this library is used.
 *
 * Every field is optional and can be set to NULL if not needed.
 *
 * You can use the @ref ACCEL_TO_ANGLE_FILTER_DEFINE macro to define the filter.
 */
struct accel_to_angle_filter {
	/** @brief Pointer to the filter context. */
	void *ctx;

	/** @brief Data process request callback.
	 *
	 * The function processes the accelerometer data.
	 * The function is called by this library when processing the
	 * accelerometer data in the @ref accel_to_angle_calc() function.
	 *
	 * @param filter_ctx Pointer to the filter context provided in this struct.
	 * @param data       Pointer to the accelerometer data to process.
	 */
	void (*data_process_request)(void *filter_ctx, struct accel_to_angle_accel_data *data);

	/** @brief Data clean request callback.
	 *
	 * The function cleans up the filter state.
	 * The function is called by this library when the
	 * @ref accel_to_angle_state_clean() is called.
	 *
	 * @param filter_ctx Pointer to the filter context provided in this struct.
	 */
	void (*data_clean_request)(void *filter_ctx);
};

/**
 * @brief Descriptor of the library context instance.
 *
 * The contents of this structure shall not be modified by the user.
 * This shall be allocated by the user and shall be accessible during the whole time
 * the library is used.
 */
struct accel_to_angle_ctx {
	/** @brief Pointer to the filter instance. */
	struct accel_to_angle_filter *filter;

	/** @brief Last pitch and roll data. */
	struct accel_to_angle_pr_data pr_last;

	/** @brief Cumulative difference between pitch and roll data calculations. */
	struct accel_to_angle_pr_data pr_diff;
};

/**
 * @brief Set a filter.
 *
 * The function sets a filter in the @ref accel_to_angle_ctx context instance.
 *
 * The @ref accel_to_angle_filter.ctx context pointer is passed to the filter callbacks when called.
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
 * The filter instance must be allocated by the user.
 * The filter instance must be accessible during the whole time the library is used.
 * The filter instance can be NULL to disable filtering.
 *
 * @param[in] ctx    Pointer to the @ref accel_to_angle_ctx context.
 * @param[in] filter Pointer to the filter instance.
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
 * The accelerometer data is provided as an array the size of @ref ACCEL_TO_ANGLE_AXIS_NUM.
 * The order of the axes in the array is X, Y, Z.
 * The calculated pitch and roll angles are stored in the provided @ref accel_to_angle_pr_data
 * struct.
 *
 * If a filter is set in the @ref accel_to_angle_ctx context, it is applied to the
 * accelerometer data before calculating the rotation.
 * Otherwise, the raw accelerometer data is used.
 *
 * @param[in] ctx  Pointer to the @ref accel_to_angle_ctx context.
 * @param[in] vals Array of sensor values containing accelerometer data for X, Y, Z axes.
 * @param[out] pr  Pointer to the structure with pitch and roll data where the calculated
 *                 rotation (in degrees) is stored.
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
 * If the threshold is exceeded the function clears the cumulative difference in the context.
 *
 * @param[in] ctx                Pointer to the @ref accel_to_angle_ctx context.
 * @param[in] pr                 Pointer to the current pitch and roll data.
 * @param[in] pr_threshold_deg   Rotation threshold in degrees for pitch and roll.
 * @param[in] axis_threshold_num Number of axes that must exceed the threshold to consider
 *                               the rotation change significant.
 *                               The value can be either 1 or 2.
 *
 * @retval true If the rotation change exceeds the threshold on the specified number of axes.
 * @retval false Otherwise.
 */
bool accel_to_angle_diff_check(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_pr_data *pr,
			       struct accel_to_angle_pr_data *pr_threshold_deg,
			       uint8_t axis_threshold_num);

/**
 * @brief Clean the internal state of the context instance.
 *
 * If the threshold is exceeded, the function resets the cumulative difference in the context,
 * and the angle difference starts accumulating again from zero.
 *
 * This function also calls the @ref accel_to_angle_filter.data_clean_request callback
 * if it is set in the context.
 *
 * The function can be used to reinitialize the state of rotation change detection.
 *
 * @param[in] ctx Pointer to the @ref accel_to_angle_ctx context.
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
