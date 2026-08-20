/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gpio_test.h"

LOG_MODULE_REGISTER(gpio_test, LOG_LEVEL_INF);

#define GPIO_NODE DT_ALIAS(test_gpio)

static const struct gpio_dt_spec gpio_spec = GPIO_DT_SPEC_GET(GPIO_NODE, gpios);

extern atomic_t started_threads;

static int setup(void)
{
	int ret;

	ret = gpio_is_ready_dt(&gpio_spec);
	if (ret != 1) {
		LOG_ERR("GPIO device is not ready: %d", ret);
		return -1;
	}

	ret = gpio_pin_configure_dt(&gpio_spec, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("GPIO configuration failed %d", ret);
		return -2;
	}

	return 0;
}

static void gpio_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("GPIO setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("GPIO load thread started");
	while (1) {
		ret = gpio_pin_toggle_dt(&gpio_spec);
		if (ret != 0) {
			LOG_ERR("Failed to toggle GPIO: %d", ret);
		}
		k_msleep(GPIO_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(gpio_load_thread, GPIO_THREAD_STACKSIZE, gpio_load_thread_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(GPIO_THREAD_PRIORITY), 0, 0);
