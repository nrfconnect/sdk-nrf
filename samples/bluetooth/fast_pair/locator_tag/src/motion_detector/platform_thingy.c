/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <bluetooth/services/fast_pair/fmdn.h>

#include <accel_to_angle/accel_to_angle.h>
#include <accel_to_angle/filter/ema.h>

#include "app_motion_detector.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

/* Minimal motion detection thresholds for angle and axes to trigger motion detection. */
#define ACCEL_TO_ANGLE_ANGLE_THR_DEG	10
#define ACCEL_TO_ANGLE_AXIS_THR_NUM	2

/* Configuration parameters for the EMA filter instance. */
#define EMA_FILTER_ADXL362_ODR_HZ	25.0f
#define EMA_FILTER_TAU_S		0.5f

#define ACCEL_NODE    DT_ALIAS(motion_detector)

ACCEL_TO_ANGLE_FILTER_EMA_DEFINE(ema_filter, EMA_FILTER_ADXL362_ODR_HZ, EMA_FILTER_TAU_S);
static ACCEL_TO_ANGLE_CTX_DEFINE(accel_to_angle_ctx, &ema_filter);

/** Motion detector state. */
static struct motion_detector {
	const struct device *dev;
	struct accel_to_angle_ctx *accel_to_angle_ctx;
	bool active;
	bool continuous_detection;
	bool motion_detected;
} motion_detector = {
	.dev = DEVICE_DT_GET(ACCEL_NODE),
	.accel_to_angle_ctx = &accel_to_angle_ctx,
};

BUILD_ASSERT(IS_ENABLED(CONFIG_ADXL362_ACCEL_ODR_25),
	     "CONFIG_ADXL362_ACCEL_ODR_25 must be enabled");
BUILD_ASSERT(EMA_FILTER_ADXL362_ODR_HZ == 25.0f,
	     "EMA_FILTER_ADXL362_ODR_HZ must be set to 25.0f to match the actual "
	     "ODR of the accelerometer");
BUILD_ASSERT((CONFIG_ADXL362_INTERRUPT_MODE == 0),
	     "CONFIG_ADXL362_INTERRUPT_MODE must be set to 0 (default mode)");
BUILD_ASSERT((CONFIG_ADXL362_ABS_REF_MODE == 1),
	     "CONFIG_ADXL362_ABS_REF_MODE must be set to 1 (referenced mode)");
BUILD_ASSERT(ACCEL_TO_ANGLE_AXIS_THR_NUM <= 2,
	     "ACCEL_TO_ANGLE_AXIS_THR_NUM must be less than or equal to 2 "
	     "(only pitch and roll are supported)");

static int inactivity_trigger_configure(bool enable);
static int activity_trigger_configure(bool enable);
static int data_ready_trigger_configure(bool enable);

static void data_ready_trigger_handle(const struct device *dev, const struct sensor_trigger *trig)
{
	int err;
	struct accel_to_angle_pr_data pr;
	struct sensor_value val[ACCEL_TO_ANGLE_AXIS_NUM];
	static struct accel_to_angle_pr_data thr_pr = {
		ACCEL_TO_ANGLE_ANGLE_THR_DEG,
		ACCEL_TO_ANGLE_ANGLE_THR_DEG
	};

	__ASSERT_NO_MSG(!k_is_in_isr());

	err = sensor_sample_fetch(motion_detector.dev);
	if (err) {
		LOG_ERR("Failed to fetch sensor sample: %d", err);
		return;
	}

	err = sensor_channel_get(motion_detector.dev, SENSOR_CHAN_ACCEL_XYZ, val);
	if (err) {
		LOG_ERR("Failed to get sensor channel: %d", err);
		return;
	}

	/* Lock the scheduler to prevent context switches while processing sensor data.
	 * The FMDN API callbacks have higher priority than the sensor trigger handler,
	 * which could lead to corruption of the motion detector state if a context switch
	 * occurs in the middle of processing.
	 */
	k_sched_lock();

	err = accel_to_angle_calc(motion_detector.accel_to_angle_ctx, val, &pr);
	if (err) {
		LOG_ERR("Failed to calculate rotation from accelerometer data: %d", err);

		k_sched_unlock();
		return;
	}

	LOG_DBG("Motion values: Pitch: %f deg, Roll: %f deg", (double)pr.pitch, (double)pr.roll);

	if (accel_to_angle_diff_check(motion_detector.accel_to_angle_ctx,
				      &pr,
				      &thr_pr,
				      ACCEL_TO_ANGLE_AXIS_THR_NUM)) {
		LOG_INF("Motion detected");
		motion_detector.motion_detected = true;
	}

	k_sched_unlock();
}

static int data_ready_trigger_configure(bool enable)
{
	int err;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	accel_to_angle_state_clean(motion_detector.accel_to_angle_ctx);

	motion_detector.continuous_detection = enable;

	err = sensor_trigger_set(motion_detector.dev,
				 &trig,
				 enable ? data_ready_trigger_handle : NULL);
	if (err) {
		LOG_ERR("Failed to set data ready trigger: %d", err);
		return err;
	}

	return 0;
}

static void activity_trigger_handle(const struct device *dev, const struct sensor_trigger *trig)
{
	__ASSERT_NO_MSG(!motion_detector.continuous_detection);

	LOG_INF("Activity interrupt detected, switching to listen for inactivity "
		"and data ready interrupts for proper motion detection");

	data_ready_trigger_configure(true);
	inactivity_trigger_configure(true);

	activity_trigger_configure(false);
}

static int activity_trigger_configure(bool enable)
{
	int err;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_MOTION,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	err = sensor_trigger_set(motion_detector.dev,
				 &trig,
				 enable ? activity_trigger_handle : NULL);
	if (err) {
		LOG_ERR("Failed to set activity trigger: %d", err);
		return err;
	}

	return 0;
}

static void inactivity_trigger_handle(const struct device *dev, const struct sensor_trigger *trig)
{
	__ASSERT_NO_MSG(motion_detector.continuous_detection);

	LOG_INF("Inactivity interrupt detected, switching to listen for activity interrupt");

	data_ready_trigger_configure(false);
	inactivity_trigger_configure(false);

	activity_trigger_configure(true);
}

static int inactivity_trigger_configure(bool enable)
{
	int err;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_STATIONARY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	err = sensor_trigger_set(motion_detector.dev,
				 &trig,
				 enable ? inactivity_trigger_handle : NULL);
	if (err) {
		LOG_ERR("Failed to set inactivity trigger: %d", err);
		return err;
	}

	return 0;
}

static void state_set(bool enable)
{
	if (motion_detector.active == enable) {
		LOG_DBG("Motion detection already %sabled", enable ? "en" : "dis");
		return;
	}

	motion_detector.motion_detected = false;
	motion_detector.active = enable;

	accel_to_angle_state_clean(motion_detector.accel_to_angle_ctx);

	if (motion_detector.active) {
		/* Start only on activity interrupt.
		 * Inactivity and data ready interrupts will be enabled
		 * once activity has been detected.
		 */
		activity_trigger_configure(true);
	} else {
		/* Disable all triggers. */
		activity_trigger_configure(false);
		inactivity_trigger_configure(false);
		data_ready_trigger_configure(false);
	}

	LOG_INF("Motion detection: %sabled", motion_detector.active ? "en" : "dis");
}

static void fmdn_motion_detector_start(void)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	__ASSERT_NO_MSG(!motion_detector.active);
	LOG_INF("FMDN: motion detector started");

	state_set(true);
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector.active);
}

static bool fmdn_motion_detector_period_expired(void)
{
	bool ret = motion_detector.motion_detected;

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	__ASSERT_NO_MSG(motion_detector.active);
	LOG_INF("FMDN: motion detector period expired. Reporting that the motion was %sdetected",
		ret ? "" : "not ");

	motion_detector.motion_detected = false;
	accel_to_angle_state_clean(motion_detector.accel_to_angle_ctx);

	return ret;
}

static void fmdn_motion_detector_stop(void)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	__ASSERT_NO_MSG(motion_detector.active);
	LOG_INF("FMDN: motion detector stopped");

	state_set(false);
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector.active);
}

static const struct bt_fast_pair_fmdn_motion_detector_cb fmdn_motion_detector_cb = {
	.start = fmdn_motion_detector_start,
	.period_expired = fmdn_motion_detector_period_expired,
	.stop = fmdn_motion_detector_stop,
};

int app_motion_detector_init(void)
{
	int err;

	if (!device_is_ready(motion_detector.dev)) {
		LOG_ERR("Device %s is not ready.", motion_detector.dev->name);
		return -EIO;
	}

	err = bt_fast_pair_fmdn_motion_detector_cb_register(&fmdn_motion_detector_cb);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_motion_detector_cb_register failed (err %d)", err);
		return err;
	}

	return 0;
}
