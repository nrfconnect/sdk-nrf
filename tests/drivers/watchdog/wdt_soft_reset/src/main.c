/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_soft_rst, LOG_LEVEL_INF);

static const struct device *const my_wdt_device = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static struct wdt_timeout_cfg m_cfg_wdt0;
static int my_wdt_channel;
static uint32_t watchdog_window = 5000U;

#define NOINIT_SECTION ".noinit.test_wdt"
volatile uint32_t supported __attribute__((section(NOINIT_SECTION)));

/* Print LOG delimiter. */
static void print_bar(void)
{
	LOG_INF("===================================================================");
}

static void print_supported_reset_cause(void)
{
	int32_t ret;

	/* Store supported reset causes in global variable 'supported'
	 * placed at NOINIT_SECTION.
	 */
	ret = hwinfo_get_supported_reset_cause((uint32_t *) &supported);
	sys_cache_data_flush_range((void *) &supported, sizeof(supported));

	if (ret == 0) {
		LOG_INF("Supported reset causes are:");
		if (supported & RESET_PIN) {
			LOG_INF(" 0: RESET_PIN is supported");
		} else {
			LOG_INF(" 0: no support for RESET_PIN");
		}
		if (supported & RESET_SOFTWARE) {
			LOG_INF(" 1: RESET_SOFTWARE is supported");
		} else {
			LOG_INF(" 1: no support for RESET_SOFTWARE");
		}
		if (supported & RESET_BROWNOUT) {
			LOG_INF(" 2: RESET_BROWNOUT is supported");
		} else {
			LOG_INF(" 2: no support for RESET_BROWNOUT");
		}
		if (supported & RESET_POR) {
			LOG_INF(" 3: RESET_POR is supported");
		} else {
			LOG_INF(" 3: no support for RESET_POR");
		}
		if (supported & RESET_WATCHDOG) {
			LOG_INF(" 4: RESET_WATCHDOG is supported");
		} else {
			LOG_INF(" 4: no support for RESET_WATCHDOG");
		}
		if (supported & RESET_DEBUG) {
			LOG_INF(" 5: RESET_DEBUG is supported");
		} else {
			LOG_INF(" 5: no support for RESET_DEBUG");
		}
		if (supported & RESET_SECURITY) {
			LOG_INF(" 6: RESET_SECURITY is supported");
		} else {
			LOG_INF(" 6: no support for RESET_SECURITY");
		}
		if (supported & RESET_LOW_POWER_WAKE) {
			LOG_INF(" 7: RESET_LOW_POWER_WAKE is supported");
		} else {
			LOG_INF(" 7: no support for RESET_LOW_POWER_WAKE");
		}
		if (supported & RESET_CPU_LOCKUP) {
			LOG_INF(" 8: RESET_CPU_LOCKUP is supported");
		} else {
			LOG_INF(" 8: no support for RESET_CPU_LOCKUP");
		}
		if (supported & RESET_PARITY) {
			LOG_INF(" 9: RESET_PARITY is supported");
		} else {
			LOG_INF(" 9: no support for RESET_PARITY");
		}
		if (supported & RESET_PLL) {
			LOG_INF("10: RESET_PLL is supported");
		} else {
			LOG_INF("10: no support for RESET_PLL");
		}
		if (supported & RESET_CLOCK) {
			LOG_INF("11: RESET_CLOCK is supported");
		} else {
			LOG_INF("11: no support for RESET_CLOCK");
		}
		if (supported & RESET_HARDWARE) {
			LOG_INF("12: RESET_HARDWARE is supported");
		} else {
			LOG_INF("12: no support for RESET_HARDWARE");
		}
		if (supported & RESET_USER) {
			LOG_INF("13: RESET_USER is supported");
		} else {
			LOG_INF("13: no support for RESET_USER");
		}
		if (supported & RESET_TEMPERATURE) {
			LOG_INF("14: RESET_TEMPERATURE is supported");
		} else {
			LOG_INF("14: no support for RESET_TEMPERATURE");
		}
		if (supported & RESET_BOOTLOADER) {
			LOG_INF("15: RESET_BOOTLOADER is supported");
		} else {
			LOG_INF("15: no support for RESET_BOOTLOADER");
		}
		if (supported & RESET_FLASH) {
			LOG_INF("16: RESET_FLASH is supported");
		} else {
			LOG_INF("16: no support for RESET_FLASH");
		}
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_supported_reset_cause() is NOT supported");
		supported = 0;
	} else {
		LOG_ERR("hwinfo_get_supported_reset_cause() failed (ret = %d)", ret);
	}
	print_bar();
}

/* Print current value of reset cause. */
static void print_current_reset_cause(uint32_t *cause)
{
	int32_t ret;

	ret = hwinfo_get_reset_cause(cause);
	if (ret == 0) {
		LOG_INF("Current reset cause is:");
		if (*cause & RESET_PIN) {
			LOG_INF(" 0: reset due to RESET_PIN");
		}
		if (*cause & RESET_SOFTWARE) {
			LOG_INF(" 1: reset due to RESET_SOFTWARE");
		}
		if (*cause & RESET_BROWNOUT) {
			LOG_INF(" 2: reset due to RESET_BROWNOUT");
		}
		if (*cause & RESET_POR) {
			LOG_INF(" 3: reset due to RESET_POR");
		}
		if (*cause & RESET_WATCHDOG) {
			LOG_INF(" 4: reset due to RESET_WATCHDOG");
		}
		if (*cause & RESET_DEBUG) {
			LOG_INF(" 5: reset due to RESET_DEBUG");
		}
		if (*cause & RESET_SECURITY) {
			LOG_INF(" 6: reset due to RESET_SECURITY");
		}
		if (*cause & RESET_LOW_POWER_WAKE) {
			LOG_INF(" 7: reset due to RESET_LOW_POWER_WAKE");
		}
		if (*cause & RESET_CPU_LOCKUP) {
			LOG_INF(" 8: reset due to RESET_CPU_LOCKUP");
		}
		if (*cause & RESET_PARITY) {
			LOG_INF(" 9: reset due to RESET_PARITY");
		}
		if (*cause & RESET_PLL) {
			LOG_INF("10: reset due to RESET_PLL");
		}
		if (*cause & RESET_CLOCK) {
			LOG_INF("11: reset due to RESET_CLOCK");
		}
		if (*cause & RESET_HARDWARE) {
			LOG_INF("12: reset due to RESET_HARDWARE");
		}
		if (*cause & RESET_USER) {
			LOG_INF("13: reset due to RESET_USER");
		}
		if (*cause & RESET_TEMPERATURE) {
			LOG_INF("14: reset due to RESET_TEMPERATURE");
		}
		if (*cause & RESET_BOOTLOADER) {
			LOG_INF("15: reset due to RESET_BOOTLOADER");
		}
		if (*cause & RESET_FLASH) {
			LOG_INF("16: reset due to RESET_FLASH");
		}
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_reset_cause() is NOT supported");
		*cause = 0;
	} else {
		LOG_ERR("hwinfo_get_reset_cause() failed (ret = %d)", ret);
	}
	print_bar();
}

/* Clear reset cause. */
static void clear_reset_cause(void)
{
	int32_t ret, temp;

	ret = hwinfo_clear_reset_cause();
	if (ret == 0) {
		LOG_INF("hwinfo_clear_reset_cause() was executed");
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_reset_cause() is NOT supported");
	} else {
		LOG_ERR("hwinfo_get_reset_cause() failed (ret = %d)", ret);
	}

	/* Confirm all are cleared. */
	hwinfo_get_reset_cause(&temp);
	if (temp == 0) {
		LOG_INF("PASS: reset causes were cleared");
	} else {
		LOG_ERR("FAIL: reset case = %u while expected is 0", temp);
	}
	print_bar();
}

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
	print_current_reset_cause(&cause);

	/* Report unexpected reset type. */
	if (cause & !(RESET_PIN | RESET_SOFTWARE | RESET_WATCHDOG)) {
		LOG_INF("Unexpected reset cause was found!");
	}

	/* Check if reset was due to pin reset. */
	if (cause & RESET_PIN) {
		LOG_INF("RESET_PIN detected");
		print_bar();
		print_supported_reset_cause();
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
