/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED0_NODE DT_ALIAS(led0)

#define LED_THREAD_STACK_SIZE 512
#define LED_THREAD_BLINK_MS   500
#define LED_THREAD_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static void led_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (!gpio_is_ready_dt(&led)) {
		return;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) != 0) {
		return;
	}

	while (true) {
		gpio_pin_toggle_dt(&led);
		k_sleep(K_MSEC(LED_THREAD_BLINK_MS));
	}
}

K_THREAD_DEFINE(usb_mcumgr_led_thread, LED_THREAD_STACK_SIZE,
		led_thread_fn, NULL, NULL, NULL,
		LED_THREAD_PRIORITY, 0, 0);
