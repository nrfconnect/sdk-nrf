/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/cache.h>
#include "hw_info_helper.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_soft_rst, LOG_LEVEL_INF);

static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static int my_wdt_channel;
static uint32_t watchdog_window = 5000U;

#define NOINIT_SECTION ".noinit.test_wdt"
static uint32_t supported __attribute__((section(NOINIT_SECTION)));

void configure_watchdog(void)
{
	int32_t ret;

	if (!device_is_ready(my_wdt_device)) {
		LOG_ERR("WDT device %s is not ready", my_wdt_device->name);
		return;
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = watchdog_window;
	m_cfg_wdt0.window.min = 0U;
	my_wdt_channel = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
	if (my_wdt_channel < 0) {
		LOG_ERR("wdt_install_timeout() returned %d", my_wdt_channel);
		return;
	}

	ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret < 0) {
		LOG_ERR("wdt_setup() returned %d", ret);
		return;
	}

	LOG_INF("Watchdog configured with window of %u milliseconds", watchdog_window);
	print_bar();
}

int main(void)
{
	uint32_t cause;

	LOG_INF("WDT and software reset test on %s", CONFIG_BOARD_TARGET);
	print_bar();

	/* Test relies on RESET_PIN to correctly start. */
	get_current_reset_cause(&cause);

	/* Report unexpected reset type. */
	if (cause & ~(RESET_PIN | RESET_SOFTWARE | RESET_WATCHDOG)) {
		LOG_INF("Unexpected reset cause was found!");
	}

	/* Check if reset was due to pin reset. */
	if (cause & RESET_PIN) {
		LOG_INF("RESET_PIN detected");
		print_bar();
		get_supported_reset_cause(&supported);
		sys_cache_data_flush_range((void *) &supported, sizeof(supported));

		clear_reset_cause();

		configure_watchdog();

		LOG_INF("Trigger Software reset");
		k_msleep(2000);
		sys_reboot(SYS_REBOOT_COLD);
	}

	/* Check if reset was due to software reset. */
	if (cause & RESET_SOFTWARE) {
		LOG_INF("RESET_SOFTWARE detected");
		print_bar();
		clear_reset_cause();

		LOG_INF("Wait to see if WDT will fire in less than %u ms", watchdog_window);
		for (int i = 0; i < (2 * watchdog_window); i += 1000) {
			LOG_INF("%u: wait for WDT", i);
			k_msleep(1000);
		}
	}

	/* Check if reset was due to watchdog reset. */
	if (cause & RESET_WATCHDOG) {
		LOG_INF("RESET_WATCHDOG detected");
		print_bar();
		clear_reset_cause();

		LOG_INF("RESET_WATCHDOG has disabled WDT");
	}

	LOG_INF("Test completed");
	return 0;
}
