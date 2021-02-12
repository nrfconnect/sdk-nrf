/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __SENSOR_SENSOR_SIM_H__
#define __SENSOR_SENSOR_SIM_H__

#include <zephyr/types.h>
#include <device.h>

struct sensor_sim_data {
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	const struct device *gpio;
	const char *gpio_port;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	struct k_sem gpio_sem;

	sensor_trigger_handler_t drdy_handler;
	struct sensor_trigger drdy_trigger;

	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_SENSOR_SIM_THREAD_STACK_SIZE);
	struct k_thread thread;
#endif  /* CONFIG_SENSOR_SIM_TRIGGER */
};

#endif /* __SENSOR_SENSOR_SIM_H__ */
