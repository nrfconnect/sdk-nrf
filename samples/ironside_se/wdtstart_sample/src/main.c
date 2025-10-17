/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>

static int wdt_channel_id = -1;
static const struct device *wdt_dev;

static void print_reset_cause(uint32_t reset_cause)
{
	printk("Reset cause: 0x%08x\n", reset_cause);

	if (reset_cause & RESET_WATCHDOG) {
		printk("*** WATCHDOG RESET DETECTED ***\n");
	} else if (reset_cause & RESET_PIN) {
		printk("*** PIN RESET DETECTED ***\n");
	} else if (reset_cause != 0) {
		printk("Other reset cause detected\n");
	} else {
		printk("No reset detected (normal boot)\n");
	}
}

static int init_watchdog(void)
{
	wdt_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(wdt011));
	if (wdt_dev == NULL) {
		printk("Watchdog device not found\n");
		return -ENODEV;
	}

	if (!device_is_ready(wdt_dev)) {
		printk("Watchdog device not ready\n");
		return -ENODEV;
	}

	/* Configure timeout to match UICR.WDTSTART configuration */
	struct wdt_timeout_cfg wdt_config = {.window.min = 0,
					     .window.max = 2000, /* 2 seconds in milliseconds */
					     .callback = NULL,
					     .flags = WDT_FLAG_RESET_SOC};

	wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_config);
	if (wdt_channel_id < 0) {
		printk("Failed to install watchdog timeout: %d\n", wdt_channel_id);
		return wdt_channel_id;
	}

	/* Setup watchdog with minimal options */
	int ret = wdt_setup(wdt_dev, 0);

	if (ret != 0) {
		printk("Failed to setup watchdog: %d\n", ret);
		return ret;
	}

	printk("Watchdog driver initialized successfully\n");
	return 0;
}

static void feed_watchdog(void)
{
	int ret = wdt_feed(wdt_dev, wdt_channel_id);

	if (ret != 0) {
		printk("Failed to feed watchdog: %d\n", ret);
	}
}

int main(void)
{
	int err;
	uint32_t reset_cause = 0;

	printk("=== IronSide SE WDTSTART Sample ===\n\n");

	/* Check reset cause using hwinfo API */
	err = hwinfo_get_reset_cause(&reset_cause);

	if (err == 0) {
		print_reset_cause(reset_cause);
	} else {
		printk("Failed to get reset cause: %d\n", err);
	}

	if (err == 0 && (reset_cause & RESET_WATCHDOG)) {
		/* Watchdog reset detected - continuously feed the watchdog */
		printk("\nWatchdog reset detected - continuously feeding watchdog\n");

		/* Initialize watchdog driver.
		 *
		 * Note that this is technically a reinitialization as the
		 * watchdog HW has already been configured and started by IronSide
		 * SE.
		 *
		 * The Zephyr driver gracefully handles initialization of an
		 * already-initialized watchdog.
		 */
		err = init_watchdog();
		if (err != 0) {
			printk("Failed to initialize watchdog driver: %d\n", err);
			return err;
		}

		/* Continuously feed the watchdog */
		int feed_count = 0;

		while (1) {
			feed_watchdog();
			feed_count++;

			if (feed_count % 500 == 1) {
				printk("Watchdog fed %d times - system still running!\n",
				       feed_count);
			}

			k_sleep(K_MSEC(100));
		}
	} else {
		/* No watchdog reset - wait for watchdog to trigger */
		printk("\nNo watchdog reset detected - waiting for watchdog to trigger...\n");
		printk("The watchdog should timeout and reset the system shortly.\n");
		printk("After the reset, this sample will detect the watchdog reset\n");
		printk("and continuously feed the watchdog to prevent further resets.\n\n");

		/* Wait for watchdog to trigger - don't feed it */
		printk("Waiting for watchdog timeout (not feeding watchdog)...\n");
		while (1) {
			k_sleep(K_MSEC(10000));
			printk("Still waiting for watchdog to trigger...\n");
		}
	}

	CODE_UNREACHABLE;
}
