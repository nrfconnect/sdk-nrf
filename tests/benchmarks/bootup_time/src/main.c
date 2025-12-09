/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
/* Unable to use GPIO Zephyr driver on FLPR core due to lack of GPIOTE interrupt. */
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);
#endif

int main(void)
{
#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
	int rc;

	rc = gpio_is_ready_dt(&led);
	__ASSERT(rc, "Error: GPIO Device not ready");

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	__ASSERT(rc == 0, "Could not configure led GPIO");

	k_msleep(10);
	gpio_pin_set_dt(&led, 1);
#endif
	return 0;
}
