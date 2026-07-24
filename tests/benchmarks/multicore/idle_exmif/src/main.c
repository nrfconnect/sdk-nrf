/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS 1000
#define ACTIVE_TIME_MS 1000

static const struct device *const flash_dev = DEVICE_DT_GET(DT_ALIAS(external_memory));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

static uint8_t flash_buffer[512];

K_SEM_DEFINE(timer_expired_sem, 0, 1);

static void timer_handler(struct k_timer *dummy)
{
	(void)dummy;

	k_sem_give(&timer_expired_sem);
}

K_TIMER_DEFINE(timer, timer_handler, NULL);

int main(void)
{
	int ret;

	printk("Multicore idle exmif test on %s\n", CONFIG_BOARD_TARGET);

	ret = device_is_ready(flash_dev);
	__ASSERT(ret == 1, "Flash device not ready\n");

	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret == 1, "GPIO device not ready\n");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	__ASSERT(ret == 0, "gpio_pin_configure_dt return code: %d\n", ret);

	printk("Test sequence start\n");
	while (1) {

		k_timer_start(&timer, K_MSEC(ACTIVE_TIME_MS), K_NO_WAIT);
		gpio_pin_set_dt(&led, 1);
		while (k_sem_take(&timer_expired_sem, K_NO_WAIT) != 0) {
			ret = flash_read(flash_dev, 0, flash_buffer, sizeof(flash_buffer));
			__ASSERT(ret == 0, "Flash read failed: %d\n", ret);
		}
		gpio_pin_set_dt(&led, 0);
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
