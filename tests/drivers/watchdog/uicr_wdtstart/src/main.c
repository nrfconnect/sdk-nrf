/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include "hw_info_helper.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uicr_wdtstart, LOG_LEVEL_INF);

static const struct device *const wdt_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(wdt011));
static int wdt_channel_id = -1;


/* First call of configure_watchdog will:
 * - report that wdt_disable() returned -EFAULT (-14)
 *   This is because driver is not aware that WDT was configured and started.
 * - NOT change WDT configuration
 *   This is because WDT can be configured only when it is stopped.
 */
static void configure_watchdog(void)
{
	int32_t ret;
	struct wdt_timeout_cfg m_cfg_wdt0;
	/* After reconfiguration, watchdog window will be
	 * larger than the one set by the Ironside.
	 */
	uint32_t watchdog_window = 5000U;

	if (!device_is_ready(wdt_dev)) {
		LOG_ERR("WDT device %s is not ready", wdt_dev->name);
		return;
	}

	ret = wdt_disable(wdt_dev);
	LOG_INF("wdt_disable() returned %d", ret);

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = watchdog_window;
	m_cfg_wdt0.window.min = 0U;
	wdt_channel_id = wdt_install_timeout(wdt_dev, &m_cfg_wdt0);
	if (wdt_channel_id < 0) {
		LOG_ERR("wdt_install_timeout() returned %d", wdt_channel_id);
		return;
	}

	ret = wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret < 0) {
		LOG_ERR("wdt_setup() returned %d", ret);
		return;
	}

	LOG_INF("Watchdog configured with window of %u milliseconds", watchdog_window);
	print_bar();
}

int main(void)
{
	int ret;
	uint32_t reset_cause = 0;

	LOG_INF("UICR.WDTSTART test on %s", CONFIG_BOARD_TARGET);
	print_bar();

	/* Test relies on RESET_PIN to correctly start. */
	get_current_reset_cause(&reset_cause);

	/* Report unexpected reset type. */
	if (reset_cause & ~(RESET_PIN | RESET_WATCHDOG)) {
		LOG_INF("Unexpected reset cause was found!");
		print_bar();
	}

	/* Check if reset was due to pin reset. */
	if (reset_cause & RESET_PIN) {
		LOG_INF("RESET_PIN detected");
		print_bar();
		clear_reset_cause();

		LOG_INF("Watchdog shall fire in ~2 seconds");
		k_sleep(K_FOREVER);
	}

	/* Check if reset was due to watchdog reset. */
	if (reset_cause & RESET_WATCHDOG) {
		LOG_INF("RESET_WATCHDOG detected");
		print_bar();
		clear_reset_cause();

		/* configure_watchdog() is intentionally called twice.
		 * - First call will "align" WDT driver state to HW configuration.
		 *   This will enable WDT feeding but will NOT reconfigure WDT.
		 * - Second call will actually apply new WDT configuration.
		 */
		configure_watchdog();
		configure_watchdog();
		LOG_INF("WATCHDOG was reconfigured");

		for (int count = 1; count < 4; count++) {
			/* Sleep longer than watchdog window set by the Ironside.
			 * But less than the window set during watchdog reconfiguration.
			 */
			k_msleep(3000);

			ret = wdt_feed(wdt_dev, wdt_channel_id);
			if (ret != 0) {
				LOG_ERR("wdt_feed() returned %d", ret);
			} else {
				LOG_INF("%u: Watchdog was feed", count);
			}
		}
		print_bar();
	}

	ret = wdt_disable(wdt_dev);
	if (ret == 0) {
		LOG_INF("WATCHDOG was disabled");
	} else {
		LOG_INF("FAIL: wdt_disable() returned %d", ret);
	}

	print_bar();
	LOG_INF("Test is completed");
}
