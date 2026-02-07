/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <string.h>
#include <accel_to_angle/accel_to_angle.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(accel_to_angle, CONFIG_ACCEL_TO_ANGLE_LOG_LEVEL);

/* Constants which might not be present in all math.h configurations */
#define PI_CONSTANT   3.1415927f
#define G_CONSTANT    9.80665f

static void accel_normalize(struct accel_to_angle_accel_data *data)
{
	float mag = sqrtf((data->x * data->x) + (data->y * data->y) + (data->z * data->z));

	if (mag == 0.0f) {
		LOG_WRN("Zero magnitude accelerometer data, cannot normalize");
		data->x = 0.0f;
		data->y = 0.0f;
		data->z = 0.0f;
		return;
	}

	data->x /= mag;
	data->y /= mag;
	data->z /= mag;
}

static float rad_to_deg_convert(float rad)
{
	return rad * 180.0f / PI_CONSTANT;
}

static float rot_pitch_compute(float ax, float ay, float az)
{
	return atan2f(-ax, sqrtf((ay * ay) + (az * az)));
}

static float rot_roll_compute(float ax, float ay, float az)
{
	return atan2f(ay, az);
}

static void rot_compute(struct accel_to_angle_accel_data *accel,
			struct accel_to_angle_pr_data *rot)
{
	if ((accel->x == 0.0f) && (accel->y == 0.0f) && (accel->z == 0.0f)) {
		LOG_WRN("Zero accelerometer data, cannot compute rotation");
		rot->pitch = 0.0f;
		rot->roll = 0.0f;
		return;
	}

	rot->pitch = rad_to_deg_convert(rot_pitch_compute(accel->x, accel->y, accel->z));
	rot->roll = rad_to_deg_convert(rot_roll_compute(accel->x, accel->y, accel->z));

	LOG_DBG("Rot data (computed): Pitch: %f deg, Roll: %f deg",
							(double)rot->pitch, (double)rot->roll);
}

static float rot_angle_normalize(float angle_deg)
{
	/* Normalize angle to [-180, 180) */
	angle_deg = fmodf(angle_deg + 180.0f, 360.0f);
	if (angle_deg < 0.0f) {
		angle_deg += 360.0f;
	}
	angle_deg -= 180.0f;

	return angle_deg;
}

static void rot_normalize(struct accel_to_angle_pr_data *data)
{
	data->pitch = rot_angle_normalize(data->pitch);
	data->roll = rot_angle_normalize(data->roll);
}

/* Unwrap current angle using previous angle (both in degrees). */
static float rot_angle_unwrap(float current, float previous)
{
	float delta = current - previous;

	if (delta > 180.0f) {
		current -= 360.0f;
	} else if (delta < -180.0f) {
		current += 360.0f;
	}

	return current;
}

static void rot_unwrap(struct accel_to_angle_pr_data *data, struct accel_to_angle_pr_data *last)
{
	data->pitch = rot_angle_unwrap(data->pitch, last->pitch);
	data->roll = rot_angle_unwrap(data->roll, last->roll);
}

static void accel_m_s2_to_g_convert(struct accel_to_angle_accel_data *data)
{
	__ASSERT_NO_MSG(G_CONSTANT != 0.0f);

	data->x /= G_CONSTANT;
	data->y /= G_CONSTANT;
	data->z /= G_CONSTANT;
}

static void sensor_value_to_accel_data(struct sensor_value vals[ACCEL_TO_ANGLE_AXIS_NUM],
				       struct accel_to_angle_accel_data *data)
{
	data->x = sensor_value_to_float(&vals[0]);
	data->y = sensor_value_to_float(&vals[1]);
	data->z = sensor_value_to_float(&vals[2]);

	LOG_DBG("Accel data (m/s^2): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);
}

static void rot_data_process(struct accel_to_angle_ctx *ctx, struct accel_to_angle_pr_data *data)
{
	rot_normalize(data);
	LOG_DBG("Rot data (normalized): Pitch: %f deg, Roll: %f deg",
		(double)data->pitch, (double)data->roll);

	rot_unwrap(data, &ctx->pr_last);
	LOG_DBG("Rot data (unwrapped): Pitch: %f deg, Roll: %f deg",
		(double)data->pitch, (double)data->roll);
}

static void accel_data_process(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_accel_data *data)
{
	/* Value in g units is needed for the rotation computation. */
	accel_m_s2_to_g_convert(data);
	LOG_DBG("Accel data (g): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);

	if (ctx->filter && ctx->filter->data_process_request) {
		ctx->filter->data_process_request(data, ctx->filter->ctx);
		LOG_DBG("Accel data (filtered): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);
	}

	accel_normalize(data);
	LOG_DBG("Accel data (normalized): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);
}

static bool pr_last_state_is_empty(struct accel_to_angle_pr_data *pr_last)
{
	static const struct accel_to_angle_pr_data empty_rot;

	return memcmp(pr_last, &empty_rot, sizeof(empty_rot)) == 0;
}

static void pr_last_state_update(struct accel_to_angle_pr_data *pr_last,
				 struct accel_to_angle_pr_data *data)
{
	*pr_last = *data;
}

static void pr_diff_state_update(struct accel_to_angle_pr_data *pr_diff,
				 struct accel_to_angle_pr_data *diff)
{
	pr_diff->pitch += diff->pitch;
	pr_diff->roll += diff->roll;

	LOG_DBG("Cumulative diff: Pitch: %f deg, Roll: %f deg",
						(double)pr_diff->pitch, (double)pr_diff->roll);
}

static void pr_diff_compute(struct accel_to_angle_pr_data *current,
			     struct accel_to_angle_pr_data *last,
			     struct accel_to_angle_pr_data *diff)
{
	diff->pitch = current->pitch - last->pitch;
	diff->roll = current->roll - last->roll;
}

static inline bool rot_threshold_check(float angle_deg, float rot_threshold_deg)
{
	return (angle_deg >= rot_threshold_deg || angle_deg <= -rot_threshold_deg);
}

static bool motion_detected_check(struct accel_to_angle_pr_data *diff,
				  float rot_threshold_deg,
				  size_t axis_threshold_num)
{
	size_t motion_axis_count = 0;

	if (rot_threshold_check(diff->pitch, rot_threshold_deg)) {
		LOG_DBG("Motion detected for pitch: %f deg", (double)diff->pitch);
		motion_axis_count++;
	}

	if (rot_threshold_check(diff->roll, rot_threshold_deg)) {
		LOG_DBG("Motion detected for roll: %f deg", (double)diff->roll);
		motion_axis_count++;
	}

	return (motion_axis_count >= axis_threshold_num);
}

int accel_to_angle_filter_set(struct accel_to_angle_ctx *ctx,
			      struct accel_to_angle_filter *filter)
{
	if (!ctx) {
		return -EINVAL;
	}

	ctx->filter = filter;

	return 0;
}

int accel_to_angle_calc(struct accel_to_angle_ctx *ctx,
		      struct sensor_value vals[ACCEL_TO_ANGLE_AXIS_NUM],
		      struct accel_to_angle_pr_data *pr)
{
	struct accel_to_angle_accel_data data;

	if (!ctx || !vals || !pr) {
		return -EINVAL;
	}

	sensor_value_to_accel_data(vals, &data);
	accel_data_process(ctx, &data);
	rot_compute(&data, pr);
	rot_data_process(ctx, pr);

	return 0;
}

bool accel_to_angle_diff_check(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_pr_data *pr,
			       float pr_threshold_deg,
			       size_t axis_threshold_num)
{
	struct accel_to_angle_pr_data diff;

	if (!ctx || !pr) {
		LOG_ERR("No context or rotation data provided");
		return false;
	}

	/* Set the reference rotation. */
	if (pr_last_state_is_empty(&ctx->pr_last)) {
		LOG_DBG("First rotation data received, skipping diff rotation computation");
		pr_last_state_update(&ctx->pr_last, pr);
		return false;
	}

	pr_diff_compute(pr, &ctx->pr_last, &diff);

	pr_last_state_update(&ctx->pr_last, pr);
	pr_diff_state_update(&ctx->pr_diff, &diff);

	if (motion_detected_check(&ctx->pr_diff, pr_threshold_deg, axis_threshold_num)) {
		LOG_INF("Motion detected!");

		/* Reset cumulative diff rotation. */
		memset(&ctx->pr_diff, 0, sizeof(ctx->pr_diff));
		return true;
	}

	return false;
}

int accel_to_angle_state_clean(struct accel_to_angle_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	memset(&ctx->pr_last, 0, sizeof(ctx->pr_last));
	memset(&ctx->pr_diff, 0, sizeof(ctx->pr_diff));

	if (ctx->filter && ctx->filter->data_clean_request) {
		ctx->filter->data_clean_request(ctx->filter->ctx);
	}

	return 0;
}
