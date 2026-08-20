/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor_test.h"

LOG_MODULE_REGISTER(sensor_test, LOG_LEVEL_INF);

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp_sensor));
static enum sensor_channel chan_to_use = SENSOR_CHAN_DIE_TEMP;

extern atomic_t started_threads;

static int setup(void)
{
	int ret;

	ret = device_is_ready(temp_dev);
	if (ret != 1) {
		LOG_ERR("TEMP sensor device is not ready: %d", ret);
		return -1;
	}

	return 0;
}

static void temp_sensor_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	struct sensor_value val;
	int32_t temp_val;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("TEMP sensor setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("TEMP sensor load thread started");
	while (1) {
		ret = sensor_sample_fetch_chan(temp_dev, chan_to_use);
		if (ret < 0) {
			LOG_ERR("sensor_sample_fetch_chan() returned: %d", ret);
		}

		ret = sensor_channel_get(temp_dev, chan_to_use, &val);
		if (ret < 0) {
			LOG_ERR("sensor_channel_get() returned: %d", ret);
		}
		temp_val = (val.val1 * 100) + (val.val2 / 10000);

		LOG_INF("Chip die temperature: %d.%02u", temp_val / 100, abs(temp_val) % 100);
		k_msleep(TEMP_SENSOR_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(temp_load_thread, TEMP_SENSOR_THREAD_STACKSIZE, temp_sensor_load_thread_worker,
		NULL, NULL, NULL, K_PRIO_PREEMPT(TEMP_SENSOR_THREAD_PRIORITY), 0, 0);
