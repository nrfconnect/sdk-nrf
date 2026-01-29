/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EMA_FILTER_H_
#define EMA_FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "accel_to_angle.h"

/**
 * @brief Compute the alpha parameter for the EMA filter.
 */
double ema_filter_alpha_compute(double odr_hz, double tau_s);

/**
 * @brief EMA filter function.
 */
void ema_filter_data_filter(struct accel_to_angle_accel_data *data, void *ctx);

/**
 * @brief EMA filter cleanup function.
 */
void ema_filter_data_filter_clean(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* EMA_FILTER_H_ */
