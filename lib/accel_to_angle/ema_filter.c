/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <string.h>
#include "accel_to_angle.h"

static double ema_calc(double alpha, double prev, double cur)
{
	return alpha * cur + (1.0 - alpha) * prev;
}

double ema_filter_alpha_compute(double odr_hz, double tau_s)
{
	double interval_s = 1.0 / odr_hz;

	return 1.0 - exp(-interval_s / tau_s);
}

void ema_filter_data_filter(struct accel_to_angle_accel_data *data, void *ctx)
{
	struct accel_to_angle_ema_ctx *ema_ctx = (struct accel_to_angle_ema_ctx *)ctx;
	struct accel_to_angle_accel_data *filtered_data;
	double alpha;

	filtered_data = ema_ctx->filtered_data;
	alpha = ema_ctx->alpha;

	filtered_data->x = ema_calc(alpha, filtered_data->x, data->x);
	filtered_data->y = ema_calc(alpha, filtered_data->y, data->y);
	filtered_data->z = ema_calc(alpha, filtered_data->z, data->z);

	data->x = filtered_data->x;
	data->y = filtered_data->y;
	data->z = filtered_data->z;
}

void ema_filter_data_filter_clean(void *ctx)
{
	struct accel_to_angle_ema_ctx *ema_ctx = (struct accel_to_angle_ema_ctx *)ctx;

	memset(ema_ctx->filtered_data, 0, sizeof(ema_ctx->filtered_data));
}
