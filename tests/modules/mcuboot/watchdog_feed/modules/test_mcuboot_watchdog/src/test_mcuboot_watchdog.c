/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>

#include "bootutil/mcuboot_status.h"

int watchdog_init(void)
{
	int wdt_channel_id;
	int err;

	printk("Setting up watchdog\n");

	const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

	if (!device_is_ready(wdt)) {
		printk("%s: device not ready.\n", wdt->name);
		return -ENODEV;
	}

	struct wdt_timeout_cfg wdt_config = {
		.flags = WDT_FLAG_RESET_SOC,

		.window.min = CONFIG_TEST_MCUBOOT_WATCHDOG_MIN_WINDOW_MS,
		.window.max = CONFIG_TEST_MCUBOOT_WATCHDOG_MAX_WINDOW_MS,
	};

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);

	if (wdt_channel_id < 0) {
		printk("Watchdog install error: %d\n", wdt_channel_id);
		return -EINVAL;
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err < 0) {
		printk("Watchdog setup error: %d\n", err);
		return err;
	}

#if defined(CONFIG_TEST_MCUBOOT_WATCHDOG_USE_MCUBOOT_STARTUP_DELAYS)
	k_msleep(CONFIG_TEST_MCUBOOT_WATCHDOG_STARTUP_SINGLE_DELAY_TIME_MS);
#endif /* CONFIG_TEST_MCUBOOT_WATCHDOG_USE_MCUBOOT_STARTUP_DELAYS */

	return 0;
}

#if defined(CONFIG_TEST_MCUBOOT_WATCHDOG_USE_MCUBOOT_STARTUP_DELAYS)

void mcuboot_status_change(mcuboot_status_type_t status)
{
	if (status == MCUBOOT_STATUS_STARTUP) {
		k_msleep(CONFIG_TEST_MCUBOOT_WATCHDOG_STARTUP_SINGLE_DELAY_TIME_MS);
	}
}

#endif /* CONFIG_TEST_MCUBOOT_WATCHDOG_USE_MCUBOOT_STARTUP_DELAYS */

SYS_INIT(watchdog_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
