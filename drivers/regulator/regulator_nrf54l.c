/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/regulator.h>
#include <soc.h>

#define DRIVER_NODE DT_NODELABEL(regulators)
#define DRIVER_REG_ADDR DT_REG_ADDR(DRIVER_NODE)
#define DRIVER_HIBERNATE_MIN_UPTIME_MS CONFIG_REGULATOR_NRF54L_HIBERNATE_MIN_UPTIME_MS

static NRF_REGULATORS_Type* driver_regs = (NRF_REGULATORS_Type*)DRIVER_REG_ADDR;

#if DRIVER_HIBERNATE_MIN_UPTIME_MS
#if CONFIG_MULTITHREADING
static void driver_await_hibernate_permitted(void)
{
	k_sleep(K_TIMEOUT_ABS_MS(DRIVER_HIBERNATE_MIN_UPTIME_MS));
}
#else
static void driver_await_hibernate_permitted(void)
{
	while(k_uptime_get() < DRIVER_HIBERNATE_MIN_UPTIME_MS) {
		k_busy_wait(1000);
	}
}
#endif
#else
static void driver_await_hibernate_permitted(void)
{
}
#endif

static int driver_api_ship_mode(const struct device *dev)
{
	driver_await_hibernate_permitted();

	driver_regs->HIBERNATOR.SYSTEMHIBERNATE = 1;

	while(1) {
		/* Spin waiting for system to enter hibernate */
	}

	return 0;
}

static DEVICE_API(regulator_parent, driver_api) = {
	.ship_mode = driver_api_ship_mode,
};

static int driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * Unconditionally release GPIO retention on wakeup.
	 *
	 * According to PS, all GPIOs should be configured before
	 * releasing GPIO retention. This however is not supported
	 * by zephyr, where drivers expect GPIOs to be in an unknown
	 * and mutable state upon init. So we release the pins as
	 * early as possible.
	 */
	driver_regs->HIBERNATOR.GPIORETENTIONRELEASE = 1;
	return 0;
}

DEVICE_DT_DEFINE(
	DT_NODELABEL(regulators),
	driver_init,
	NULL,
	NULL,
	NULL,
	PRE_KERNEL_1,
	0,
	&driver_api
);
