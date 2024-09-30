/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static K_SEM_DEFINE(my_gpio_sem, 0, 1);

void my_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_sem_give(&my_gpio_sem);
}

static struct gpio_callback gpio_cb;

int main(void)
{
	int ret;

	ret = gpio_is_ready_dt(&sw);
	if (ret < 0) {
		printk("GPIO Device not ready (%d)\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&sw, GPIO_INPUT);
	if (ret < 0) {
		printk("Could not configure sw GPIO (%d)\n", ret);
		return 0;
	}

	ret = gpio_pin_interrupt_configure(sw.port, sw.pin, GPIO_INT_LEVEL_ACTIVE);
	if (ret < 0) {
		printk("Could not configure sw GPIO interrupt (%d)\n", ret);
		return 0;
	}
	gpio_init_callback(&gpio_cb, my_gpio_callback, 0xFFFF);
	gpio_add_callback(sw.port, &gpio_cb);

	while (1) {
		if (k_sem_take(&my_gpio_sem, K_FOREVER) != 0) {
			printk("Failed to take a semaphore\n");
			return 0;
		}
		k_busy_wait(1000000);
		k_sem_reset(&my_gpio_sem);
	}

	return 0;
}
