/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	/* LED4 blue */
	gpio_pin_set_dt(&led_blue, 1);	return 0;
}
