/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

int main(void)
{
	int rc;

	rc = gpio_is_ready_dt(&led);
	__ASSERT(rc, "Error: GPIO Device not ready");

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(rc == 0, "Could not configure led GPIO");

	k_msleep(10);
	gpio_pin_set_dt(&led, 0);
	k_msleep(10);
	gpio_pin_set_dt(&led, 1);
	return 0;
}
