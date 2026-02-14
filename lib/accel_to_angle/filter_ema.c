/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <string.h>
#include <accel_to_angle/accel_to_angle.h>
#include <accel_to_angle/filter/ema.h>

static float ema_calc(float alpha, float prev, float cur)
{
	return alpha * cur + (1.0f - alpha) * prev;
}

static float ema_alpha_compute(float odr_hz, float tau_s)
{
	float interval_s;

	__ASSERT((odr_hz > 0.0f) && (tau_s > 0.0f),
					"Invalid ODR or Tau for EMA filter alpha computation");

	interval_s = 1.0f / odr_hz;

	return 1.0f - expf(-interval_s / tau_s);
}

void filter_ema_data_process_request(void *ctx, struct accel_to_angle_accel_data *data)
{
	struct accel_to_angle_ema_ctx *ema_ctx = (struct accel_to_angle_ema_ctx *)ctx;
	struct accel_to_angle_accel_data *filtered_data;
	float alpha;

	__ASSERT_NO_MSG(ctx);
	__ASSERT_NO_MSG(data);

	if (!ema_ctx->alpha) {
		ema_ctx->alpha = ema_alpha_compute(ema_ctx->odr_hz, ema_ctx->tau_s);
	}

	filtered_data = &ema_ctx->filtered_data;
	alpha = ema_ctx->alpha;

	filtered_data->x = ema_calc(alpha, filtered_data->x, data->x);
	filtered_data->y = ema_calc(alpha, filtered_data->y, data->y);
	filtered_data->z = ema_calc(alpha, filtered_data->z, data->z);

	data->x = filtered_data->x;
	data->y = filtered_data->y;
	data->z = filtered_data->z;
}

void filter_ema_data_clean_request(void *ctx)
{
	struct accel_to_angle_ema_ctx *ema_ctx = (struct accel_to_angle_ema_ctx *)ctx;

	__ASSERT_NO_MSG(ctx);

	memset(&ema_ctx->filtered_data, 0, sizeof(ema_ctx->filtered_data));
}
