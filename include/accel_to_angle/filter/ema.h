/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ACCEL_TO_ANGLE_FILTER_EMA_H_
#define ACCEL_TO_ANGLE_FILTER_EMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <accel_to_angle/accel_to_angle.h>

/**@file ema.h
 *
 * @brief Accel to angle library - the EMA filter API.
 * @defgroup accel_to_angle_filter_ema Accel to angle library - EMA filter API
 * Exponential moving average filter API.
 * @{
 */

/**
 * @brief Define an EMA filter instance.
 *
 * The macro defines an EMA filter instance with the specified parameters.
 * The macro allocates the filter context and the filter struct.
 *
 * @param _instance_name Name of the filter instance.
 * @param _odr_hz        Output data rate in Hz. (This value has to be greater than 0.)
 * @param _tau_s         Time constant in seconds. (This value has to be greater than 0.)
 */
#define ACCEL_TO_ANGLE_FILTER_EMA_DEFINE(_instance_name, _odr_hz, _tau_s)	\
	BUILD_ASSERT(_odr_hz > 0.0f,						\
		"The output data rate has to be greater than 0");		\
	BUILD_ASSERT(_tau_s > 0.0f,						\
		"The time constant has to be greater than 0");			\
	static struct accel_to_angle_ema_ctx _instance_name##_ctx = {		\
		.odr_hz = _odr_hz,						\
		.tau_s = _tau_s,						\
		.alpha = 0.0f,							\
	};									\
										\
	static ACCEL_TO_ANGLE_FILTER_DEFINE(					\
		_instance_name,							\
		&_instance_name##_ctx,						\
		filter_ema_data_process_request,				\
		filter_ema_data_clean_request					\
	)

/**
 * @brief Context for the EMA filter.
 *
 * Used internally by this library.
 * The contents of this struct must not be modified by the user.
 * This must be allocated by the user and must be accessible during the whole time
 * the filter is used.
 */
struct accel_to_angle_ema_ctx {
	/** @brief Filtered accelerometer data. */
	struct accel_to_angle_accel_data filtered_data;

	/** @brief Output data rate in Hz.
	 *
	 * This value has to be greater than 0.
	 */
	float odr_hz;

	/** @brief Time constant in seconds.
	 *
	 * This value has to be greater than 0.
	 */
	float tau_s;

	/** @brief Filter coefficient. */
	float alpha;
};

/**
 * @brief Filter EMA.
 *
 * Processes accelerometer data using the EMA filter.
 * The function API is compatible with the @ref accel_to_angle_filter.data_process_request callback.
 *
 * @param[in] ctx  Pointer to the filter context.
 * @param[in] data Pointer to the accelerometer data to process.
 */
void filter_ema_data_process_request(void *ctx, struct accel_to_angle_accel_data *data);

/**
 * @brief Clean up the EMA filter.
 *
 * This function cleans up the EMA filter state.
 * The function API is compatible with the @ref accel_to_angle_filter.data_clean_request callback.
 *
 * @param[in] ctx Pointer to the filter context.
 */
void filter_ema_data_clean_request(void *ctx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ACCEL_TO_ANGLE_FILTER_EMA_H_ */
