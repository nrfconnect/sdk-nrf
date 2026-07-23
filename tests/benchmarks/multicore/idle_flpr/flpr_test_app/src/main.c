/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define ACTIVE_TIME_MS 1000
#define SLEEP_TIME_MS 1000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct k_timer my_timer;
static volatile bool timer_expired;

static void my_timer_handler(struct k_timer *dummy)
{
	(void)dummy;
	timer_expired = true;
}

int main(void)
{
	int counter = 0;
	int ret;

	printk("Multicore idle_flpr test on %s\n", CONFIG_BOARD_TARGET);
	printk("Main sleeps for %d ms\n", SLEEP_TIME_MS);

	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret, "LED is not ready\n");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(ret == 0, "Unable to configure GPIO as output\n");

	k_timer_init(&my_timer, my_timer_handler, NULL);

	/* Run test forever */
	while (1) {
		timer_expired = false;

		ret = gpio_pin_set_dt(&led, 1);

		/* start a one-shot timer that expires after 1 second */
		k_timer_start(&my_timer, K_MSEC(ACTIVE_TIME_MS), K_NO_WAIT);
		__ASSERT(ret == 0, "Unable to turn on LED\n");

		/* Keep CPU active for ~ 1 second */
		while (!timer_expired) {
			k_busy_wait(10000);
			k_yield();
		}

		printk("Run %d\n", counter);
		counter++;

		ret = gpio_pin_set_dt(&led, 0);
		__ASSERT(ret == 0, "Unable to turn off LED\n");

		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
