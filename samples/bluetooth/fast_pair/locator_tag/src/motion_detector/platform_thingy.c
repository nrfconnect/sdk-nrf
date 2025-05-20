/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <bluetooth/services/fast_pair/fmdn.h>

#include "app_motion_detector.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

#define PI_CONSTANT 3.14159265358979323846

/* Minimum required rotation in degrees to consider device as moving */
#define ROT_THR_DEG		10

/* Minimum required rotation in degrees to consider device as moving */
#define ROT_THR_RAD		(ROT_THR_DEG * PI_CONSTANT / 180)

#define AXIS_NUM_MAX		3
#define AXIS_THR_NUM		2

/* Sampling rate (in samples per second) of the motion detection sensor (gyroscope). */
#define GYRO_SAMPLING_FREQ	10

#define GYRO_NODE	DT_ALIAS(gyro)
#define GYRO_PWR_NODE	DT_ALIAS(gyro_pwr)

#define GYRO_POLL_THREAD_STACK_SIZE	1024
#define GYRO_POLL_THREAD_PRIORITY	0

#define SENSOR_VALUE_SET(_var, _int, _frac) \
	_var.val1 = _int; \
	_var.val2 = _frac

#define SENSOR_VALUE_SCALE_1000_DEG_PER_SEC_SET(_var)	SENSOR_VALUE_SET(_var, 1000, 0)

#define SENSOR_VALUE_OVERSAMPLING_DISABLE(_var)		SENSOR_VALUE_SET(_var, 1, 0)

/* The lowest sampling frequency available. */
#define SENSOR_VALUE_SAMPLING_FREQ_25_HZ_SET(_var)	SENSOR_VALUE_SET(_var, 25, 0)

static K_SEM_DEFINE(gyro_poll_sem, 0, 1);

static struct motion_detector {
	const struct device *dev;
	bool active;
	/* Rotation in X, Y and Z axis respectively. */
	double rot[AXIS_NUM_MAX];
	struct k_spinlock lock;
} motion_detector = {
	.dev = DEVICE_DT_GET(GYRO_NODE),
};

static bool motion_detected_check(void)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	size_t motion_axis_count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(motion_detector.rot); i++) {
		if (motion_detector.rot[i] >= ROT_THR_RAD ||
		    motion_detector.rot[i] <= -ROT_THR_RAD) {
			motion_axis_count++;
		}
	}

	return (motion_axis_count >= AXIS_THR_NUM);
}

static void fmdn_motion_detector_start(void)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	__ASSERT_NO_MSG(!motion_detector.active);
	LOG_INF("FMDN: motion detector started");
	motion_detector.active = true;
	k_sem_give(&gyro_poll_sem);
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector.active);
}

static bool fmdn_motion_detector_period_expired(void)
{
	bool is_motion_detected;

	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	is_motion_detected = motion_detected_check();

	__ASSERT_NO_MSG(motion_detector.active);
	LOG_INF("FMDN: motion detector period expired. Reporting that the motion was %sdetected",
		is_motion_detected ? "" : "not ");
	memset(motion_detector.rot, 0, sizeof(motion_detector.rot));
	return is_motion_detected;
}

static void fmdn_motion_detector_stop(void)
{
	/* It is assumed that this function executes in the cooperative thread context. */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	__ASSERT_NO_MSG(motion_detector.active);
	LOG_INF("FMDN: motion detector stopped");
	motion_detector.active = false;
	memset(motion_detector.rot, 0, sizeof(motion_detector.rot));
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector.active);
}

static const struct bt_fast_pair_fmdn_motion_detector_cb fmdn_motion_detector_cb = {
	.start = fmdn_motion_detector_start,
	.period_expired = fmdn_motion_detector_period_expired,
	.stop = fmdn_motion_detector_stop,
};

static int sensor_configure(const struct device *sensor)
{
	struct sensor_value scale;
	struct sensor_value sampling_freq;
	struct sensor_value oversampling;
	int err;

	SENSOR_VALUE_SCALE_1000_DEG_PER_SEC_SET(scale);
	err = sensor_attr_set(sensor, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &scale);
	if (err) {
		return err;
	}

	SENSOR_VALUE_OVERSAMPLING_DISABLE(oversampling);
	err = sensor_attr_set(sensor, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING,
			      &oversampling);
	if (err) {
		return err;
	}

	SENSOR_VALUE_SAMPLING_FREQ_25_HZ_SET(sampling_freq);
	err = sensor_attr_set(sensor, SENSOR_CHAN_GYRO_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY,
			      &sampling_freq);
	if (err) {
		return err;
	}

	return err;
}

static void gyro_poll_thread_fn(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	while (true) {
		k_spinlock_key_t key;
		struct sensor_value val[ARRAY_SIZE(motion_detector.rot)];

		k_sem_take(&gyro_poll_sem, K_FOREVER);

		sensor_sample_fetch(motion_detector.dev);
		sensor_channel_get(motion_detector.dev, SENSOR_CHAN_GYRO_XYZ, val);

		key = k_spin_lock(&motion_detector.lock);
		if (motion_detector.active) {
			for (size_t i = 0; i < ARRAY_SIZE(motion_detector.rot); i++) {
				/* Rotation = Angular velocity * Time = Angular velocity /
				 *					Sampling frequency.
				 */
				motion_detector.rot[i] += sensor_value_to_double(&val[i]) /
							  GYRO_SAMPLING_FREQ;
			}
			k_sem_give(&gyro_poll_sem);
		}
		k_spin_unlock(&motion_detector.lock, key);

		k_msleep(MSEC_PER_SEC / GYRO_SAMPLING_FREQ);
	}
}

K_THREAD_DEFINE(gyro_poll_thread, GYRO_POLL_THREAD_STACK_SIZE, gyro_poll_thread_fn,
		NULL, NULL, NULL, GYRO_POLL_THREAD_PRIORITY, 0, 0);

int app_motion_detector_init(void)
{
	const struct gpio_dt_spec pwr = GPIO_DT_SPEC_GET(GYRO_PWR_NODE, enable_gpios);
	int err;

	if (!device_is_ready(pwr.port)) {
		LOG_ERR("GYRO_PWR_NODE is not ready");
		return -EIO;
	}

	err = gpio_pin_configure_dt(&pwr, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("Error while configuring GYRO_PWR_NODE (err %d)", err);
		return err;
	}

	if (!device_is_ready(motion_detector.dev)) {
		LOG_ERR("Device %s is not ready.", motion_detector.dev->name);
		return -EIO;
	}

	err = sensor_configure(motion_detector.dev);
	if (err) {
		LOG_ERR("Configuring gyroscope sensor failed (err %d)", err);
		return err;
	}

	err = bt_fast_pair_fmdn_motion_detector_cb_register(&fmdn_motion_detector_cb);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_motion_detector_cb_register failed (err %d)", err);
		return err;
	}

	return 0;
}
