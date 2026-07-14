/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/pm/pm.h>
#include "hw_info_helper.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_sleep, LOG_LEVEL_INF);

static const struct device *const wdt_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(wdt));
static int wdt_channel_id = -1;
static bool wdt_support_sleep = true;

static void configure_watchdog(void)
{
	int32_t ret;
	struct wdt_timeout_cfg wdt_cfg;

	if (!device_is_ready(wdt_dev)) {
		LOG_ERR("WDT device %s is not ready", wdt_dev->name);
		return;
	}

	wdt_cfg.callback = NULL;
	wdt_cfg.flags = WDT_FLAG_RESET_SOC;
	wdt_cfg.window.max = 2000U;
	wdt_cfg.window.min = 0U;
	wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_cfg);
	if (wdt_channel_id < 0) {
		LOG_ERR("wdt_install_timeout() returned %d", wdt_channel_id);
		return;
	}

	ret = wdt_setup(wdt_dev, WDT_OPT_PAUSE_IN_SLEEP);
	if (ret == -ENOTSUP) {
		wdt_support_sleep = false;
		ret = wdt_setup(wdt_dev, 0);
	}
	if (ret != 0) {
		LOG_ERR("wdt_setup() returned %d", ret);
		return;
	}
}

static void disable_watchdog(void)
{
	int ret;

	if (!device_is_ready(wdt_dev)) {
		LOG_ERR("WDT device %s is not ready", wdt_dev->name);
		return;
	}

	ret = wdt_disable(wdt_dev);
	if (ret != 0) {
		LOG_ERR("wdt_disable() returned %d", ret);
	}
}

int main(void)
{
	int ret;
	uint32_t reset_cause = 0;

	LOG_INF("WDT Sleep test on %s", CONFIG_BOARD_TARGET);
	print_bar();

	get_current_reset_cause(&reset_cause);

	/* Check if reset was due to pin reset. */
	if (reset_cause & RESET_PIN) {
		LOG_INF("RESET_PIN detected");
		print_bar();
		clear_reset_cause();

		configure_watchdog();
		if (!wdt_support_sleep && IS_ENABLED(CONFIG_PM)) {
			static struct pm_notifier wdt_pm_notifier = {
				.state_entry = (void (*)(enum pm_state))disable_watchdog,
				.state_exit  = (void (*)(enum pm_state))configure_watchdog,
			};
			pm_notifier_register(&wdt_pm_notifier);
			LOG_INF("PM callbacks registered to disable/enable WDT during sleep");
		}

		k_msleep(2100);
		LOG_INF("Watchdog did not reset the system while sleeping.");
		ret = wdt_feed(wdt_dev, wdt_channel_id);
		if (ret != 0) {
			LOG_ERR("wdt_feed() returned %d", ret);
		}
		LOG_INF("Now we'll stay busy without feeding.");
		print_bar();
		k_busy_wait(2100 * 1000);
		LOG_ERR("Watchdog failed to reset system during busy time.");
	}

	/* Check if reset was due to watchdog reset. */
	if (reset_cause & RESET_WATCHDOG) {
		LOG_INF("RESET_WATCHDOG detected");
		print_bar();
		clear_reset_cause();

		configure_watchdog();
		k_busy_wait(1900 * 1000);
		disable_watchdog();
		k_busy_wait(200 * 1000);
		LOG_INF("Test is completed");
	}
}
