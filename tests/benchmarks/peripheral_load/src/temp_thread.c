/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(temp, LOG_LEVEL_INF);

#include <zephyr/drivers/sensor.h>
#include "common.h"

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp_sensor));
static enum sensor_channel chan_to_use = SENSOR_CHAN_DIE_TEMP;

/* TEMP thread */
static void temp_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	struct sensor_value val;
	int32_t temp_val;

	atomic_inc(&started_threads);

	if (!device_is_ready(temp_dev)) {
		LOG_ERR("Device %s is not ready.", temp_dev->name);
		atomic_inc(&completed_threads);
		return;
	}

	for (int i = 0; i < TEMP_THREAD_COUNT_MAX; i++) {
		/* Capture TEMP sample */
		ret = sensor_sample_fetch_chan(temp_dev, chan_to_use);
		if (ret < 0) {
			LOG_ERR("sensor_sample_fetch_chan() returned: %d", ret);
			atomic_inc(&completed_threads);
			return;
		}

		/* Retrieve TEMP sample */
		ret = sensor_channel_get(temp_dev, chan_to_use, &val);
		if (ret < 0) {
			LOG_ERR("sensor_channel_get() returned: %d", ret);
			atomic_inc(&completed_threads);
			return;
		}
		temp_val = (val.val1 * 100) + (val.val2 / 10000);

		LOG_INF("DIE_TEMP is %d.%02u", temp_val / 100, abs(temp_val) % 100);

		k_msleep(TEMP_THREAD_PERIOD);
	}

	LOG_INF("TEMP thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_temp_id, TEMP_THREAD_STACKSIZE, temp_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(TEMP_THREAD_PRIORITY), 0, 0);
