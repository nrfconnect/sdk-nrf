/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

#if CONFIG_FLASH
#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_spi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_qspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_qspi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(jedec_mspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_mspi_nor)
#else
#error Unsupported flash driver
#define FLASH_NODE DT_INVALID_NODE
#endif
#endif

#if defined(FLASH_NODE)
static const struct device *const flash_dev = DEVICE_DT_GET(FLASH_NODE);
#endif

int main(void)
{
	int rc;

#if defined(FLASH_NODE)
	rc = device_is_ready(flash_dev);
	__ASSERT(rc, "Error: Flash Device not ready");
#endif

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
