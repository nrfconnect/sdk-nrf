/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/cache.h>
#include "hw_info_helper.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_soft_rst_cnf, LOG_LEVEL_INF);

static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static const uint32_t watchdog_window = 2000U;

#define NOINIT_SECTION ".noinit.test_wdt"
static uint32_t supported __attribute__((section(NOINIT_SECTION)));
static int my_wdt_channel_1 __attribute__((section(NOINIT_SECTION)));
static int my_wdt_channel_2 __attribute__((section(NOINIT_SECTION)));

#if defined(CONFIG_TEST_CPU_LOCKUP_RESET)
#include <errno.h>
#include <zephyr/logging/log_ctrl.h>

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	LOG_INF("%s(%d)", __func__, reason);
	LOG_PANIC();

	/* Assert inside Assert handler - shall result in reset due to cpu lockup. */
	__ASSERT(0, "Intentionally failed assert inside kernel panic");
}
#endif

void configure_watchdog(uint32_t window, bool second_chan)
{
	int32_t ret;

	if (!device_is_ready(my_wdt_device)) {
		LOG_ERR("WDT device %s is not ready", my_wdt_device->name);
		return;
	}

	m_cfg_wdt0.callback = NULL;
	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = window;
	m_cfg_wdt0.window.min = 0U;

	my_wdt_channel_1 = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
	if (my_wdt_channel_1 < 0) {
		LOG_ERR("wdt_install_timeout(No. 1) returned %d", my_wdt_channel_1);
		return;
	}

	if (second_chan) {
		my_wdt_channel_2 = wdt_install_timeout(my_wdt_device, &m_cfg_wdt0);
		if (my_wdt_channel_2 < 0) {
			LOG_ERR("wdt_install_timeout(No. 2) returned %d", my_wdt_channel_2);
			return;
		}
	}

	ret = wdt_setup(my_wdt_device, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret < 0) {
		LOG_ERR("wdt_setup() returned %d", ret);
		return;
	}

	sys_cache_data_flush_range((void *) &my_wdt_channel_1, sizeof(my_wdt_channel_1));
	sys_cache_data_flush_range((void *) &my_wdt_channel_2, sizeof(my_wdt_channel_2));

	LOG_INF("Watchdog configured with window of %u milliseconds", window);
	if (second_chan) {
		LOG_INF("Two watchdog channels");
	} else {
		LOG_INF("One watchdog channel");
	}
	print_bar();
}

int main(void)
{
	uint32_t cause;
	int32_t ret;

#if defined(CONFIG_TEST_SOFTWARE_RESET)
	LOG_INF("WDT reconfigure after software reset executed on %s", CONFIG_BOARD_TARGET);
#endif
#if defined(CONFIG_TEST_CPU_LOCKUP_RESET)
	LOG_INF("WDT reconfigure after cpu lockup reset executed on %s", CONFIG_BOARD_TARGET);
#endif
	print_bar();

	/* Test depends on RESET_PIN to correctly start. */
	get_current_reset_cause(&cause);

	/* Report unexpected reset type. */
	if (cause & ~(RESET_PIN | RESET_SOFTWARE | RESET_CPU_LOCKUP | RESET_WATCHDOG)) {
		LOG_INF("Unexpected reset cause was found!");
	}

	/* Check if reset was due to pin reset. */
	if (cause & RESET_PIN) {
		LOG_INF("RESET_PIN detected");
		print_bar();
		get_supported_reset_cause(&supported);
		sys_cache_data_flush_range((void *) &supported, sizeof(supported));

		clear_reset_cause();

		/* Start Watchdog with !TWO! timeout channels. */
		configure_watchdog(watchdog_window, true);

#if defined(CONFIG_TEST_SOFTWARE_RESET)
		LOG_INF("Trigger Software reset");
		k_msleep(300);
		sys_reboot(SYS_REBOOT_COLD);
#endif
#if defined(CONFIG_TEST_CPU_LOCKUP_RESET)
		LOG_INF("Trigger CPU Lockup reset");
		k_msleep(300);
		__ASSERT(0, "Intentionally failed assertion");
#endif
	}

	/* Check if reset was due to expected software reset (or cpu lockup reset).
	 * Then reconfigure and start WDT with single timeout.
	 * Check that feeding one timeout is enough to prevent WDT from resetting the device.
	 * Finally, disable the WDT.
	 */
	if ((cause & RESET_SOFTWARE) || (cause & RESET_CPU_LOCKUP)) {
		if (cause & RESET_SOFTWARE) {
			LOG_INF("RESET_SOFTWARE detected");
		}
		if (cause & RESET_CPU_LOCKUP) {
			LOG_INF("RESET_CPU_LOCKUP detected");
		}
		print_bar();
		clear_reset_cause();

		/* Watchdog behavior after sotware reset (or cpu lockup reset)
		 * is configured with CONFIG_NRFX_WDT_CONFIG_STOP_AT_INIT.
		 */
#if defined(CONFIG_NRFX_WDT_CONFIG_STOP_AT_INIT) || \
    defined(CONFIG_SOC_SERIES_NRF54H) || \
    defined(CONFIG_SOC_SERIES_NRF92)
		/* If CONFIG_NRFX_WDT_CONFIG_STOP_AT_INIT=y (default) then
		 * WDT is stopped during WDT driver initialization.
		 * This feature shall restore WDT state to "the default", like after HW reset".
		 */

		LOG_INF("Wait to see if WDT is disabled.");
		k_msleep(watchdog_window + 1000);
		print_bar();
#else
		/* If CONFIG_NRFX_WDT_CONFIG_STOP_AT_INIT=n then
		 * WDT preserves its configuration (WDT registers are NOT restored to their
		 * default values by the hardware, as in the case with pinreset).
		 * In such case, WDT driver is not aware that WDT is configured and running.
		 */
		ret = wdt_feed(my_wdt_device, my_wdt_channel_1);
		LOG_INF("Feeding WDT returns %d, expected -EINVAL (-22)", ret);

		/* When WDT runs, its configuration registers are locked.
		 * To unlock WDT registers do dummy configuration then call wdt_disable().
		 * Subsequent WDT configuration will take effect.
		 *
		 * !!! Dummy configuration must "configure" same amount of WDT timeout channels.
		 * !!! Otherwise, wdt_disable() will NOT delte all of them.
		 */
		configure_watchdog(watchdog_window, true);
		wdt_disable(my_wdt_device);
		print_bar();
#endif

		/* Start Watchdog with 2x larger window and ONE channel. */
		configure_watchdog(2 * watchdog_window, false);

		/* Feed ONE timeout channel. */
		for (int i = 0; i < 3; i++) {
			ret = wdt_feed(my_wdt_device, my_wdt_channel_1);
			LOG_INF("%u: feeding WDT (%d)", i, ret);
			k_msleep(watchdog_window + 1000);
		}

		wdt_disable(my_wdt_device);
		print_bar();

		LOG_INF("Test completed");
		return 0;
	}

	/* Check if reset was due to watchdog reset. */
	if (cause & RESET_WATCHDOG) {
		/* Should never get here! */
		LOG_ERR("Unexpected Watchdog reset has occurred");
		return -1;
	}
}
