/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio, LOG_LEVEL_INF);

#include <zephyr/drivers/gpio.h>
#include "common.h"

#define GPIO_NODE		DT_ALIAS(led2)
#if !DT_NODE_HAS_STATUS(GPIO_NODE, okay)
#error "Unsupported board: led2 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec gpio_spec = GPIO_DT_SPEC_GET(GPIO_NODE, gpios);

/* GPIO thread */
static void gpio_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	atomic_inc(&started_threads);

	if (!gpio_is_ready_dt(&gpio_spec)) {
		LOG_ERR("Device %s is not ready.", gpio_spec.port->name);
		atomic_inc(&completed_threads);
		return;
	}

	ret = gpio_pin_configure_dt(&gpio_spec, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("gpio_pin_configure_dt(pin %d) returned %d", gpio_spec.pin, ret);
		atomic_inc(&completed_threads);
		return;
	}

	for (int i = 0; i < GPIO_THREAD_COUNT_MAX; i++) {
		ret = gpio_pin_toggle_dt(&gpio_spec);
		if (ret < 0) {
			LOG_ERR("gpio_pin_toggle_dt(pin %d) returned %d", gpio_spec.pin, ret);
			atomic_inc(&completed_threads);
			return;
		}

		LOG_INF("LED was toggled");
		k_msleep(GPIO_THREAD_SLEEP);
	}
	LOG_INF("GPIO thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_gpio_id, GPIO_THREAD_STACKSIZE, gpio_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(GPIO_THREAD_PRIORITY), 0, 0);
