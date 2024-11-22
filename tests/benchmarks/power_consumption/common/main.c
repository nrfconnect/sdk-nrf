/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#if defined CONFIG_GPIO
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec gpio_p1_8_output = GPIO_DT_SPEC_GET(DT_ALIAS(out0), gpios);


int main(void)
{
	int rc;

	rc = gpio_pin_configure_dt(&gpio_p1_8_output, GPIO_OUTPUT_ACTIVE);
	__ASSERT_NO_MSG(rc == 0);

	while (1) {
		gpio_pin_set_dt(&gpio_p1_8_output, 1);
		k_msleep(1000);
		gpio_pin_set_dt(&gpio_p1_8_output, 0);
		k_msleep(1000);
	}
}

#else

int main(void)
{
	while (1) {
		k_msleep(1000);
	}
}

#endif