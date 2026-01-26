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
#define PI_CONSTANT   (3.1415927f)
#define G_CONSTANT    (9.80665f)

static void accel_normalize(struct accel_to_angle_accel_data *data)
{
	__ASSERT_NO_MSG(data);

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

static float pr_pitch_compute(float ax, float ay, float az)
{
	return atan2f(-ax, sqrtf((ay * ay) + (az * az)));
}

static float pr_roll_compute(float ax, float ay, float az)
{
	return atan2f(ay, az);
}

static void pr_compute(struct accel_to_angle_accel_data *accel,
		       struct accel_to_angle_pr_data *pr)
{
	__ASSERT_NO_MSG(accel);
	__ASSERT_NO_MSG(pr);

	if ((accel->x == 0.0f) && (accel->y == 0.0f) && (accel->z == 0.0f)) {
		LOG_WRN("Zero accelerometer data, cannot compute PR");
		pr->pitch = 0.0f;
		pr->roll = 0.0f;
		return;
	}

	pr->pitch = rad_to_deg_convert(pr_pitch_compute(accel->x, accel->y, accel->z));
	pr->roll = rad_to_deg_convert(pr_roll_compute(accel->x, accel->y, accel->z));

	LOG_DBG("PR data (computed): Pitch: %f deg, Roll: %f deg",
							(double)pr->pitch, (double)pr->roll);
}

static float pr_angle_normalize(float angle_deg)
{
	/* Normalize angle to [-180, 180) */
	angle_deg = fmodf(angle_deg + 180.0f, 360.0f);
	if (angle_deg < 0.0f) {
		angle_deg += 360.0f;
	}
	angle_deg -= 180.0f;

	return angle_deg;
}

static void pr_normalize(struct accel_to_angle_pr_data *data)
{
	__ASSERT_NO_MSG(data);

	data->pitch = pr_angle_normalize(data->pitch);
	data->roll = pr_angle_normalize(data->roll);
}

/* Unwrap current angle using previous angle (both in degrees). */
static float pr_angle_unwrap(float current, float previous)
{
	float delta = current - previous;

	if (delta > 180.0f) {
		current -= 360.0f;
	} else if (delta < -180.0f) {
		current += 360.0f;
	}

	return current;
}

static void pr_unwrap(struct accel_to_angle_pr_data *data, struct accel_to_angle_pr_data *last)
{
	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(last);

	data->pitch = pr_angle_unwrap(data->pitch, last->pitch);
	data->roll = pr_angle_unwrap(data->roll, last->roll);
}

static void accel_m_s2_to_g_convert(struct accel_to_angle_accel_data *data)
{
	__ASSERT_NO_MSG(G_CONSTANT != 0.0f);
	__ASSERT_NO_MSG(data);

	data->x /= G_CONSTANT;
	data->y /= G_CONSTANT;
	data->z /= G_CONSTANT;
}

static void sensor_value_to_accel_data(struct sensor_value vals[ACCEL_TO_ANGLE_AXIS_NUM],
				       struct accel_to_angle_accel_data *data)
{
	__ASSERT_NO_MSG(vals);
	__ASSERT_NO_MSG(data);

	data->x = sensor_value_to_float(&vals[0]);
	data->y = sensor_value_to_float(&vals[1]);
	data->z = sensor_value_to_float(&vals[2]);

	LOG_DBG("Accel data (m/s^2): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);
}

static void pr_data_process(struct accel_to_angle_ctx *ctx, struct accel_to_angle_pr_data *data)
{
	pr_normalize(data);
	LOG_DBG("PR data (normalized): Pitch: %f deg, Roll: %f deg",
		(double)data->pitch, (double)data->roll);

	pr_unwrap(data, &ctx->pr_last);
	LOG_DBG("PR data (unwrapped): Pitch: %f deg, Roll: %f deg",
		(double)data->pitch, (double)data->roll);
}

static void accel_data_process(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_accel_data *data)
{
	__ASSERT_NO_MSG(ctx);
	__ASSERT_NO_MSG(data);

	/* Value in g units is needed for the PR computation. */
	accel_m_s2_to_g_convert(data);
	LOG_DBG("Accel data (g): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);

	if (ctx->filter && ctx->filter->data_process_request) {
		ctx->filter->data_process_request(ctx->filter->ctx, data);
		LOG_DBG("Accel data (filtered): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);
	}

	accel_normalize(data);
	LOG_DBG("Accel data (normalized): X: %f, Y: %f, Z: %f",
						(double)data->x, (double)data->y, (double)data->z);
}

static bool pr_last_state_is_empty(struct accel_to_angle_pr_data *pr_last)
{
	static const struct accel_to_angle_pr_data empty_pr;

	__ASSERT_NO_MSG(pr_last);

	return memcmp(pr_last, &empty_pr, sizeof(empty_pr)) == 0;
}

static void pr_last_state_update(struct accel_to_angle_pr_data *pr_last,
				 struct accel_to_angle_pr_data *data)
{
	__ASSERT_NO_MSG(pr_last);
	__ASSERT_NO_MSG(data);

	*pr_last = *data;
}

static void pr_diff_state_update(struct accel_to_angle_pr_data *pr_diff,
				 struct accel_to_angle_pr_data *diff)
{
	__ASSERT_NO_MSG(pr_diff);
	__ASSERT_NO_MSG(diff);

	pr_diff->pitch += diff->pitch;
	pr_diff->roll += diff->roll;

	LOG_DBG("Cumulative diff: Pitch: %f deg, Roll: %f deg",
		(double)pr_diff->pitch, (double)pr_diff->roll);
}

static void pr_diff_compute(struct accel_to_angle_pr_data *current,
			     struct accel_to_angle_pr_data *last,
			     struct accel_to_angle_pr_data *diff)
{
	__ASSERT_NO_MSG(current);
	__ASSERT_NO_MSG(last);
	__ASSERT_NO_MSG(diff);

	diff->pitch = current->pitch - last->pitch;
	diff->roll = current->roll - last->roll;
}

static inline bool pr_threshold_check(float angle_deg, float pr_threshold_deg)
{
	return (angle_deg >= pr_threshold_deg || angle_deg <= -pr_threshold_deg);
}

static bool motion_detected_check(struct accel_to_angle_pr_data *diff,
				  struct accel_to_angle_pr_data *pr_threshold_deg,
				  uint8_t axis_threshold_num)
{
	__ASSERT_NO_MSG(diff);
	__ASSERT_NO_MSG(pr_threshold_deg);

	uint8_t motion_axis_count = 0;

	if (pr_threshold_check(diff->pitch, pr_threshold_deg->pitch)) {
		LOG_DBG("Motion detected for pitch: %f deg", (double)diff->pitch);
		motion_axis_count++;
	}

	if (pr_threshold_check(diff->roll, pr_threshold_deg->roll)) {
		LOG_DBG("Motion detected for roll: %f deg", (double)diff->roll);
		motion_axis_count++;
	}

	return (motion_axis_count >= axis_threshold_num);
}

static void last_diff_update(struct accel_to_angle_ctx *ctx, struct accel_to_angle_pr_data *pr)
{
	struct accel_to_angle_pr_data diff = {0.0f, 0.0f};

	__ASSERT_NO_MSG(ctx);
	__ASSERT_NO_MSG(pr);

	/* Set the reference PR. */
	if (pr_last_state_is_empty(&ctx->pr_last)) {
		LOG_DBG("First PR data received, skipping diff PR computation");
		pr_last_state_update(&ctx->pr_last, pr);
		pr_diff_state_update(&ctx->pr_diff, &diff);
		return;
	}

	pr_diff_compute(pr, &ctx->pr_last, &diff);

	pr_last_state_update(&ctx->pr_last, pr);
	pr_diff_state_update(&ctx->pr_diff, &diff);
}

int accel_to_angle_filter_set(struct accel_to_angle_ctx *ctx,
			      struct accel_to_angle_filter *filter)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (filter && !filter->data_process_request) {
		LOG_ERR("Filter process request callback is not set");
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
	pr_compute(&data, pr);
	pr_data_process(ctx, pr);
	last_diff_update(ctx, pr);

	return 0;
}

bool accel_to_angle_diff_check(struct accel_to_angle_ctx *ctx,
			       struct accel_to_angle_pr_data *pr,
			       struct accel_to_angle_pr_data *pr_threshold_deg,
			       uint8_t axis_threshold_num)
{
	if (!ctx || !pr || !pr_threshold_deg) {
		LOG_ERR("No context or PR data provided");
		return false;
	}

	if (axis_threshold_num == 0) {
		LOG_ERR("Axis threshold number is set to 0, detection will always return true");
	} else if (axis_threshold_num > 2) {
		LOG_ERR("Axis threshold number is greater than the number of available  PR axes, "
			"detection will never return true");
	}

	if (motion_detected_check(&ctx->pr_diff, pr_threshold_deg, axis_threshold_num)) {
		LOG_INF("Motion detected!");

		/* Reset cumulative diff PR. */
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
