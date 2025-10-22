/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(be680, LOG_LEVEL_INF);

#include <zephyr/drivers/sensor.h>
#include "common.h"

static const struct device *const bme680 = DEVICE_DT_GET_ONE(bosch_bme680);

/* BME680 thread */
static void bme680_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	struct sensor_value samples[4]; /* 0-temp, 1-press, 2-humidity, 3-gas_res*/
	int32_t latest_val1[4];
	int32_t latest_val2[4];
	int32_t average_val1[4] = {0};
	int32_t average_val2[4] = {0};

	atomic_inc(&started_threads);

	if (!device_is_ready(bme680)) {
		LOG_ERR("Device %s is not ready.", bme680->name);
		atomic_inc(&completed_threads);
		return;
	}

	for (int i = 0; i < (BME680_THREAD_COUNT_MAX * BME680_THREAD_OVERSAMPLING); i++) {
		/* Capture samples */
		ret = sensor_sample_fetch(bme680);
		if (ret < 0) {
			LOG_ERR("%s: sensor_sample_fetch() returned: %d", bme680->name, ret);
			atomic_inc(&completed_threads);
			return;
		}

		/* Retrieve samples */
		sensor_channel_get(bme680, SENSOR_CHAN_AMBIENT_TEMP, &samples[0]);
		if (ret < 0) {
			LOG_ERR("%s: sensor_channel_get(TEMP) returned: %d", bme680->name, ret);
			atomic_inc(&completed_threads);
			return;
		}
		sensor_channel_get(bme680, SENSOR_CHAN_PRESS, &samples[1]);
		if (ret < 0) {
			LOG_ERR("%s: sensor_channel_get(PRESS) returned: %d", bme680->name, ret);
			atomic_inc(&completed_threads);
			return;
		}
		sensor_channel_get(bme680, SENSOR_CHAN_HUMIDITY, &samples[2]);
		if (ret < 0) {
			LOG_ERR("%s: sensor_channel_get(HUMIDITY) returned: %d", bme680->name, ret);
			atomic_inc(&completed_threads);
			return;
		}
		sensor_channel_get(bme680, SENSOR_CHAN_GAS_RES, &samples[3]);
		if (ret < 0) {
			LOG_ERR("%s: sensor_channel_get(GAS_RES) returned: %d", bme680->name, ret);
			atomic_inc(&completed_threads);
			return;
		}

		/* Calculate average */
		for (int property = 0; property < 4; property++) {
			latest_val1[property] = samples[property].val1;
			latest_val2[property] = samples[property].val2;
			average_val1[property] =
				(average_val1[property] + latest_val1[property]) / 2;
			average_val2[property] =
				(average_val2[property] + latest_val2[property]) / 2;
		}

		if ((i % BME680_THREAD_OVERSAMPLING) == 0) {
			/* Print result */
			LOG_INF("%10s - T: %d.%06d; P: %d.%06d; H: %d.%06d; G: %d.%06d",
				bme680->name,
				average_val1[0], average_val2[0], average_val1[1], average_val2[1],
				average_val1[2], average_val2[2], average_val1[3], average_val2[3]);
		}

		k_msleep(BME680_THREAD_SLEEP / BME680_THREAD_OVERSAMPLING);
	}
	LOG_INF("BME680 thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_bme680_id, BME680_THREAD_STACKSIZE, bme680_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(BME680_THREAD_PRIORITY), 0, 0);
