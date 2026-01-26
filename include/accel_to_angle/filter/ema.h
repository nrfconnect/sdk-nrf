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
 * @brief Library Accel to angle Exponential Moving Average filter.
 * @defgroup accel_to_angle_filter_ema Accel to angle Exponential Moving Average filter
 * @{
 */

/**
 * @brief Define an EMA filter instance.
 *
 * The macro defines an EMA filter instance with the specified parameters.
 * The macro allocates the filter context and the filter struct.
 *
 * @param _instance_name Name of the filter instance.
 * @param _odr_hz Output data rate in Hz.
 * @param _tau Time constant.
 */
#define ACCEL_TO_ANGLE_FILTER_EMA_DEFINE(_instance_name, _odr_hz, _tau)	\
	static struct accel_to_angle_ema_ctx _instance_name##_ctx = {	\
		.odr_hz = _odr_hz,					\
		.tau_s = _tau,						\
		.alpha = 0.0f,						\
	};								\
									\
	static ACCEL_TO_ANGLE_FILTER_DEFINE(				\
		_instance_name,						\
		&_instance_name##_ctx,					\
		filter_ema_data_process_request,			\
		filter_ema_data_clean_request				\
	)

/**
 * @brief Context for the EMA filter.
 *
 * Used internally by the accel_to_angle module.
 * The contents of this struct shall not be modified by the user.
 * This shall be allocated by the user and shall be accessible during the whole time
 * the filter is used.
 */
struct accel_to_angle_ema_ctx {
	struct accel_to_angle_accel_data filtered_data;
	float odr_hz;
	float tau_s;
	float alpha;
};

/**
 * @brief EMA filter function.
 *
 * Processes accelerometer data using the EMA filter.
 * The function API is compatible with the accel_to_angle_filter
 * @ref accel_to_angle_filter.data_process_request callback.
 */
void filter_ema_data_process_request(struct accel_to_angle_accel_data *data, void *ctx);

/**
 * @brief EMA filter cleanup function.
 *
 * Cleans up the EMA filter state.
 * The function API is compatible with the accel_to_angle_filter
 * @ref accel_to_angle_filter.data_clean_request callback.
 */
void filter_ema_data_clean_request(void *ctx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ACCEL_TO_ANGLE_FILTER_EMA_H_ */
