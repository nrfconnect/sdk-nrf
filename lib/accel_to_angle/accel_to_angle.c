/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <string.h>
#include "accel_to_angle.h"
#include "ema_filter.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(accel_to_angle, CONFIG_ACCEL_TO_ANGLE_LOG_LEVEL);

/* Constants which might not be present in all math.h configurations */
#define PI_CONSTANT   3.14159265358979323846
#define G_CONSTANT    9.80665

static void accel_normalize(struct accel_to_angle_accel_data *data)
{
	double mag = sqrt((data->x * data->x) + (data->y * data->y) + (data->z * data->z));

	if (mag == 0.0) {
		LOG_WRN("Zero magnitude accelerometer data, cannot normalize");
		data->x = 0.0;
		data->y = 0.0;
		data->z = 0.0;
		return;
	}

	data->x /= mag;
	data->y /= mag;
	data->z /= mag;
}

static double rad_to_deg_convert(double rad)
{
	return rad * 180 / PI_CONSTANT;
}

static double rot_pitch_compute(double ax, double ay, double az)
{
	return atan2(-ax, sqrt((ay * ay) + (az * az)));
}

static double rot_roll_compute(double ax, double ay, double az)
{
	return atan2(ay, az);
}

static void rot_compute(struct accel_to_angle_accel_data *accel,
			struct accel_to_angle_rot_data *rot)
{
	if ((accel->x == 0.0) && (accel->y == 0.0) && (accel->z == 0.0)) {
		LOG_WRN("Zero accelerometer data, cannot compute rotation");
		rot->pitch = 0.0;
		rot->roll = 0.0;
		return;
	}

	rot->pitch = rad_to_deg_convert(rot_pitch_compute(accel->x, accel->y, accel->z));
	rot->roll = rad_to_deg_convert(rot_roll_compute(accel->x, accel->y, accel->z));

	LOG_DBG("Rot data (computed): Pitch: %f deg, Roll: %f deg", rot->pitch, rot->roll);
}

static double rot_angle_normalize(double angle_deg)
{
	/* Normalize angle to [-180, 180) */
	angle_deg = fmod(angle_deg + 180.0, 360.0);
	if (angle_deg < 0) {
		angle_deg += 360.0;
	}
	angle_deg -= 180.0;

	return angle_deg;
}

static void rot_normalize(struct accel_to_angle_rot_data *data)
{
	data->pitch = rot_angle_normalize(data->pitch);
	data->roll = rot_angle_normalize(data->roll);
}

/* Unwrap current angle using previous angle (both in degrees). */
static double rot_angle_unwrap(double current, double previous)
{
	double delta = current - previous;

	if (delta > 180.0) {
		current -= 360.0;
	} else if (delta < -180.0) {
		current += 360.0;
	}

	return current;
}

static void rot_unwrap(struct accel_to_angle_rot_data *data, struct accel_to_angle_rot_data *last)
{
	data->pitch = rot_angle_unwrap(data->pitch, last->pitch);
	data->roll = rot_angle_unwrap(data->roll, last->roll);
}

static void accel_m_s2_to_g_convert(struct accel_to_angle_accel_data *data)
{
	__ASSERT_NO_MSG(G_CONSTANT != 0.0);

	data->x /= G_CONSTANT;
	data->y /= G_CONSTANT;
	data->z /= G_CONSTANT;
}

static void sensor_value_to_accel_data(struct sensor_value vals[ACCEL_TO_ANGLE_AXIS_NUM],
				       struct accel_to_angle_accel_data *data)
{
	data->x = sensor_value_to_double(&vals[0]);
	data->y = sensor_value_to_double(&vals[1]);
	data->z = sensor_value_to_double(&vals[2]);

	LOG_DBG("Accel data (m/s^2): X: %f, Y: %f, Z: %f", data->x, data->y, data->z);
}

static void rot_data_process(struct accel_to_angle_ctx *ctx, struct accel_to_angle_rot_data *data)
{
	rot_normalize(data);
	LOG_DBG("Rot data (normalized): Pitch: %f deg, Roll: %f deg",
		data->pitch, data->roll);

	rot_unwrap(data, &ctx->rot_last);
	LOG_DBG("Rot data (unwrapped): Pitch: %f deg, Roll: %f deg",
		data->pitch, data->roll);
}

static void accel_data_process(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_accel_data *data)
{
	/* Value in g units is needed for the rotation computation. */
	accel_m_s2_to_g_convert(data);
	LOG_DBG("Accel data (g): X: %f, Y: %f, Z: %f", data->x, data->y, data->z);

	if (ctx->filter_cb) {
		ctx->filter_cb(data, ctx->filter_ctx);
		LOG_DBG("Accel data (filtered): X: %f, Y: %f, Z: %f", data->x, data->y, data->z);
	}

	accel_normalize(data);
	LOG_DBG("Accel data (normalized): X: %f, Y: %f, Z: %f", data->x, data->y, data->z);
}

static bool rot_last_state_is_empty(struct accel_to_angle_rot_data *rot_last)
{
	static const struct accel_to_angle_rot_data empty_rot;

	return memcmp(rot_last, &empty_rot, sizeof(empty_rot)) == 0;
}

static void rot_last_state_update(struct accel_to_angle_rot_data *rot_last,
				  struct accel_to_angle_rot_data *data)
{
	*rot_last = *data;
}

static void rot_diff_state_update(struct accel_to_angle_rot_data *rot_diff,
				  struct accel_to_angle_rot_data *diff)
{
	rot_diff->pitch += diff->pitch;
	rot_diff->roll += diff->roll;

	LOG_DBG("Cumulative diff: Pitch: %f deg, Roll: %f deg",
		rot_diff->pitch, rot_diff->roll);
}

static void rot_diff_compute(struct accel_to_angle_rot_data *current,
			     struct accel_to_angle_rot_data *last,
			     struct accel_to_angle_rot_data *diff)
{
	diff->pitch = current->pitch - last->pitch;
	diff->roll = current->roll - last->roll;
}

static inline bool rot_threshold_check(double angle_deg, double rot_threshold_deg)
{
	return (angle_deg >= rot_threshold_deg || angle_deg <= -rot_threshold_deg);
}

static bool motion_detected_check(struct accel_to_angle_rot_data *diff,
				  double rot_threshold_deg,
				  size_t axis_threshold_num)
{
	size_t motion_axis_count = 0;

	if (rot_threshold_check(diff->pitch, rot_threshold_deg)) {
		LOG_DBG("Motion detected for pitch: %f deg", diff->pitch);
		motion_axis_count++;
	}

	if (rot_threshold_check(diff->roll, rot_threshold_deg)) {
		LOG_DBG("Motion detected for roll: %f deg", diff->roll);
		motion_axis_count++;
	}

	return (motion_axis_count >= axis_threshold_num);
}

int accel_to_angle_init(struct accel_to_angle_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(*ctx));

	return 0;
}

void accel_to_angle_custom_filter_set(struct accel_to_angle_ctx *ctx,
				      accel_to_angle_filter filter_cb,
				      accel_to_angle_filter_clean filter_clean_cb,
				      void *filter_ctx)
{
	ctx->filter_ctx = filter_ctx;
	ctx->filter_cb = filter_cb;
	ctx->filter_clean_cb = filter_clean_cb;
}

int accel_to_angle_ema_filter_set(struct accel_to_angle_ctx *ctx,
				 struct accel_to_angle_ema_ctx *filter_ctx,
				 double odr_hz, double tau_s)
{
	if (!filter_ctx) {
		return -EINVAL;
	}

	if (odr_hz == 0.0 || tau_s == 0.0) {
		return -EINVAL;
	}

	memset(filter_ctx->filtered_data, 0, sizeof(filter_ctx->filtered_data));
	filter_ctx->alpha = ema_filter_alpha_compute(odr_hz, tau_s);
	ctx->filter_ctx = filter_ctx;
	ctx->filter_cb = ema_filter_data_filter;
	ctx->filter_clean_cb = ema_filter_data_filter_clean;

	return 0;
}

int accel_to_angle_calc(struct accel_to_angle_ctx *ctx,
		      struct sensor_value vals[ACCEL_TO_ANGLE_AXIS_NUM],
		      struct accel_to_angle_rot_data *rot)
{
	struct accel_to_angle_accel_data data;

	if (!vals || !rot) {
		return -EINVAL;
	}

	sensor_value_to_accel_data(vals, &data);
	accel_data_process(ctx, &data);
	rot_compute(&data, rot);
	rot_data_process(ctx, rot);

	return 0;
}

bool accel_to_angle_diff_check(struct accel_to_angle_ctx *ctx,
			     struct accel_to_angle_rot_data *rot,
			     double rot_threshold_deg,
			     size_t axis_threshold_num)
{
	struct accel_to_angle_rot_data diff;

	/* Set the reference rotation. */
	if (rot_last_state_is_empty(&ctx->rot_last)) {
		LOG_DBG("First rotation data received, skipping diff rotation computation");
		rot_last_state_update(&ctx->rot_last, rot);
		return false;
	}

	rot_diff_compute(rot, &ctx->rot_last, &diff);

	rot_last_state_update(&ctx->rot_last, rot);
	rot_diff_state_update(&ctx->rot_diff, &diff);

	if (motion_detected_check(&ctx->rot_diff, rot_threshold_deg, axis_threshold_num)) {
		LOG_INF("Motion detected!");

		/* Reset cumulative diff rotation. */
		memset(&ctx->rot_diff, 0, sizeof(ctx->rot_diff));
		return true;
	}

	return false;
}

void accel_to_angle_state_clean(struct accel_to_angle_ctx *ctx)
{
	memset(&ctx->rot_last, 0, sizeof(ctx->rot_last));
	memset(&ctx->rot_diff, 0, sizeof(ctx->rot_diff));
	if (ctx->filter_clean_cb) {
		ctx->filter_clean_cb(ctx->filter_ctx);
	}
}
