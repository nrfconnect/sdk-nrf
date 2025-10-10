/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

#if DT_NODE_HAS_STATUS(DT_ALIAS(accel0), okay)
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(accel, LOG_LEVEL_INF);

static const struct device *const accelerometer = DEVICE_DT_GET(DT_ALIAS(accel0));

/* Accelerometer thread */
static void accel_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	struct sensor_value samples[3];
	double latest[3];
	double average[3] = {0.0};

	atomic_inc(&started_threads);

	if (!device_is_ready(accelerometer)) {
		LOG_ERR("Device %s is not ready.", accelerometer->name);
		atomic_inc(&completed_threads);
		return;
	}

	for (int i = 0; i < (ACCEL_THREAD_COUNT_MAX * ACCEL_THREAD_OVERSAMPLING); i++) {
		/* Capture acceleration value */
		ret = sensor_sample_fetch(accelerometer);
		if (ret < 0) {
			LOG_ERR("%s: sensor_sample_fetch() returned: %d", accelerometer->name, ret);
			atomic_inc(&completed_threads);
			return;
		}

		/* Retrieve samples */
		ret = sensor_channel_get(accelerometer, SENSOR_CHAN_ACCEL_XYZ, samples);
		if (ret < 0) {
			LOG_ERR("%s: sensor_channel_get() returned: %d", accelerometer->name, ret);
			atomic_inc(&completed_threads);
			return;
		}

		/* Calculate average */
		for (int axis = 0; axis < 3; axis++) {
			latest[axis] = sensor_value_to_double(&samples[axis]);
			average[axis] = (average[axis] + latest[axis]) / 2;
		}

		/* Print result */
		if ((i % ACCEL_THREAD_OVERSAMPLING) == 0) {
			LOG_INF("%10s - X: %6.2f; Y: %6.2f; Z: %6.2f",
				accelerometer->name, average[0], average[1], average[2]);
		}

		k_msleep(ACCEL_THREAD_SLEEP / ACCEL_THREAD_OVERSAMPLING);
	}
	LOG_INF("Accelerometer thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_accel_id, ACCEL_THREAD_STACKSIZE, accel_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(ACCEL_THREAD_PRIORITY), 0, 0);

#else
#pragma message("Accelerometer thread skipped due to missing node in the DTS")
#endif
