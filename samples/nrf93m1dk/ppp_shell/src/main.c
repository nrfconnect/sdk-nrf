/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

int main(void)
{
	gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_ACTIVE);
	return 0;
}
