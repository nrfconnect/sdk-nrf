/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "counter_test.h"

LOG_MODULE_REGISTER(counter_test, LOG_LEVEL_INF);

#define COUNTER_DEV_ENTRY(node) DEVICE_DT_GET(node),

static const struct device *const counter_devs[] = {
	DT_FOREACH_STATUS_OKAY(nordic_nrf_timer, COUNTER_DEV_ENTRY)};

#define COUNTER_DEVICES_COUNT ARRAY_SIZE(counter_devs)

static struct counter_alarm_cfg alarm_cfg[COUNTER_DEVICES_COUNT];
extern atomic_t started_threads;

static struct k_sem counter_expired_sems[COUNTER_DEVICES_COUNT];

static void counter_callback(const struct device *counter_dev, uint8_t chan_id, uint32_t ticks,
			     void *user_data)
{
	ARG_UNUSED(counter_dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);

	struct k_sem *per_timer_expiry_sem = (struct k_sem *)user_data;

	k_sem_give(per_timer_expiry_sem);
}

static int setup(void)
{
	int ret;

	for (uint8_t counter_id = 0; counter_id < COUNTER_DEVICES_COUNT; counter_id++) {

		k_sem_init(&counter_expired_sems[counter_id], 0, 1);

		alarm_cfg[counter_id].flags = 0;
		alarm_cfg[counter_id].ticks =
			counter_us_to_ticks(counter_devs[counter_id], COUNTER_THREAD_SLEEP * 1000);
		alarm_cfg[counter_id].callback = counter_callback;
		alarm_cfg[counter_id].user_data = (void *)&counter_expired_sems[counter_id];

		ret = device_is_ready(counter_devs[counter_id]);
		if (ret != 1) {
			LOG_ERR("COUNTER/TIMER device %u not ready: %d", counter_id, ret);
			return -1;
		}

		ret = counter_start(counter_devs[counter_id]);
		if (ret != 0) {
			LOG_ERR("COUNTER/TIMER device %u failed to start: %d", counter_id, ret);
			return -2;
		}

		ret = counter_set_channel_alarm(counter_devs[counter_id], ALARM_CHANNEL_ID,
						&alarm_cfg[counter_id]);
		if (ret != 0) {
			LOG_ERR("COUNTER/TIMER device %u channel alarm set failed: %d", counter_id,
				ret);
			return -3;
		}
	}

	return 0;
}

static void counter_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	uint32_t now_ticks;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("COUNTER/TIMER setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("COUNTER/TIMER load thread started");
	while (1) {
		for (uint8_t counter_id = 0; counter_id < COUNTER_DEVICES_COUNT; counter_id++) {
			if (k_sem_take(&counter_expired_sems[counter_id], K_NO_WAIT) == 0) {
				/* Get current ticks */
				ret = counter_get_value(counter_devs[counter_id], &now_ticks);
				if (ret) {
					LOG_ERR("counter_get_value() dev %u failed: %d", counter_id,
						ret);
					return;
				}

				/* Set a new alarm */
				ret = counter_set_channel_alarm(counter_devs[counter_id],
								ALARM_CHANNEL_ID,
								&alarm_cfg[counter_id]);
				if (ret != 0) {
					LOG_ERR("counter_set_channel_alarm() dev %u failed: %d",
						counter_id, ret);
				}
			}
		}
		k_msleep(COUNTER_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(counter_load_thread, COUNTER_THREAD_STACKSIZE, counter_load_thread_worker, NULL,
		NULL, NULL, K_PRIO_PREEMPT(COUNTER_THREAD_PRIORITY), 0, 0);
