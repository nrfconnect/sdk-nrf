/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cnt, LOG_LEVEL_INF);

#include <zephyr/drivers/counter.h>
#include "common.h"

#define ALARM_CHANNEL_ID	(0)

static const struct device *const counter_dev = DEVICE_DT_GET(DT_ALIAS(test_counter));
static struct counter_alarm_cfg alarm_cfg;
static int counter_expire_count;


static void my_counter_cb(const struct device *counter_dev,
	uint8_t chan_id, uint32_t ticks, void *user_data)
{
	int ret;
	uint32_t now_ticks;
	uint64_t now_usec;
	uint64_t expire_usec;

	counter_expire_count++;

	/* Get current ticks */
	ret = counter_get_value(counter_dev, &now_ticks);
	if (ret) {
		LOG_ERR("counter_get_value() failed (%d)", ret);
		return;
	}

	/* Set a new alarm */
	if (counter_expire_count < COUNTER_THREAD_COUNT_MAX) {
		ret = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, user_data);
		if (ret != 0) {
			LOG_ERR("counter_set_channel_alarm() failed (%d)", ret);
		}
	} else {
		counter_stop(counter_dev);
	}

	/* Convert ticks to us */
	expire_usec = counter_ticks_to_us(counter_dev, ticks);
	now_usec = counter_ticks_to_us(counter_dev, now_ticks);

	LOG_INF("Counter expired %d times (at %lld us), restarted just after %lld us",
		counter_expire_count, expire_usec, now_usec);
}

/* Counter thread */
static void counter_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	atomic_inc(&started_threads);

	if (!device_is_ready(counter_dev)) {
		LOG_ERR("Device %s is not ready.", counter_dev->name);
		atomic_inc(&completed_threads);
		return;
	}

	counter_start(counter_dev);

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = counter_us_to_ticks(counter_dev, COUNTER_THREAD_PERIOD * 1000);
	alarm_cfg.callback = my_counter_cb;
	alarm_cfg.user_data = &alarm_cfg;

	ret = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID, &alarm_cfg);
	if (-EINVAL == ret) {
		LOG_ERR("Alarm settings invalid");
		atomic_inc(&completed_threads);
		return;
	} else if (-ENOTSUP == ret) {
		LOG_ERR("Alarm setting request not supported");
		atomic_inc(&completed_threads);
		return;
	} else if (ret != 0) {
		LOG_ERR("counter_set_channel_alarm() failed (%d)", ret);
		atomic_inc(&completed_threads);
		return;
	}

	while (counter_expire_count < COUNTER_THREAD_COUNT_MAX) {
		/* Wait for condition to be meet */
		k_msleep(100);
	}

	LOG_INF("Counter thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_counter_id, COUNTER_THREAD_STACKSIZE, counter_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(COUNTER_THREAD_PRIORITY), 0, 0);
