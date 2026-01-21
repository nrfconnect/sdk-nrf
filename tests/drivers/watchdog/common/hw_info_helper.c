/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include "hw_info_helper.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hw_info_helper, LOG_LEVEL_INF);

/**
 * @brief Print LOG delimiter.
 */
void print_bar(void)
{
	LOG_INF("===================================================================");
}

/**
 * @brief Get and print reset causes supported by the HW.
 *
 * @param supported Pointer to a variable that will be set with supported reset causes.
 */
void get_supported_reset_cause(uint32_t *supported)
{
	int32_t ret;

	ret = hwinfo_get_supported_reset_cause(supported);

	if (ret == 0) {
		LOG_INF("Supported reset causes are:");
		if (*supported & RESET_PIN) {
			LOG_INF(" 0: RESET_PIN is supported");
		} else {
			LOG_INF(" 0: no support for RESET_PIN");
		}
		if (*supported & RESET_SOFTWARE) {
			LOG_INF(" 1: RESET_SOFTWARE is supported");
		} else {
			LOG_INF(" 1: no support for RESET_SOFTWARE");
		}
		if (*supported & RESET_BROWNOUT) {
			LOG_INF(" 2: RESET_BROWNOUT is supported");
		} else {
			LOG_INF(" 2: no support for RESET_BROWNOUT");
		}
		if (*supported & RESET_POR) {
			LOG_INF(" 3: RESET_POR is supported");
		} else {
			LOG_INF(" 3: no support for RESET_POR");
		}
		if (*supported & RESET_WATCHDOG) {
			LOG_INF(" 4: RESET_WATCHDOG is supported");
		} else {
			LOG_INF(" 4: no support for RESET_WATCHDOG");
		}
		if (*supported & RESET_DEBUG) {
			LOG_INF(" 5: RESET_DEBUG is supported");
		} else {
			LOG_INF(" 5: no support for RESET_DEBUG");
		}
		if (*supported & RESET_SECURITY) {
			LOG_INF(" 6: RESET_SECURITY is supported");
		} else {
			LOG_INF(" 6: no support for RESET_SECURITY");
		}
		if (*supported & RESET_LOW_POWER_WAKE) {
			LOG_INF(" 7: RESET_LOW_POWER_WAKE is supported");
		} else {
			LOG_INF(" 7: no support for RESET_LOW_POWER_WAKE");
		}
		if (*supported & RESET_CPU_LOCKUP) {
			LOG_INF(" 8: RESET_CPU_LOCKUP is supported");
		} else {
			LOG_INF(" 8: no support for RESET_CPU_LOCKUP");
		}
		if (*supported & RESET_PARITY) {
			LOG_INF(" 9: RESET_PARITY is supported");
		} else {
			LOG_INF(" 9: no support for RESET_PARITY");
		}
		if (*supported & RESET_PLL) {
			LOG_INF("10: RESET_PLL is supported");
		} else {
			LOG_INF("10: no support for RESET_PLL");
		}
		if (*supported & RESET_CLOCK) {
			LOG_INF("11: RESET_CLOCK is supported");
		} else {
			LOG_INF("11: no support for RESET_CLOCK");
		}
		if (*supported & RESET_HARDWARE) {
			LOG_INF("12: RESET_HARDWARE is supported");
		} else {
			LOG_INF("12: no support for RESET_HARDWARE");
		}
		if (*supported & RESET_USER) {
			LOG_INF("13: RESET_USER is supported");
		} else {
			LOG_INF("13: no support for RESET_USER");
		}
		if (*supported & RESET_TEMPERATURE) {
			LOG_INF("14: RESET_TEMPERATURE is supported");
		} else {
			LOG_INF("14: no support for RESET_TEMPERATURE");
		}
		if (*supported & RESET_BOOTLOADER) {
			LOG_INF("15: RESET_BOOTLOADER is supported");
		} else {
			LOG_INF("15: no support for RESET_BOOTLOADER");
		}
		if (*supported & RESET_FLASH) {
			LOG_INF("16: RESET_FLASH is supported");
		} else {
			LOG_INF("16: no support for RESET_FLASH");
		}
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_supported_reset_cause() is NOT supported");
		*supported = 0;
	} else {
		LOG_ERR("hwinfo_get_supported_reset_cause() failed (ret = %d)", ret);
	}
	print_bar();
}

/**
 * @brief Get and print current value of reset cause.
 *
 * @param cause Pointer to a variable that will be set with current reset causes.
 */
void get_current_reset_cause(uint32_t *cause)
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

/**
 * @brief Clear reset cause.
 */
void clear_reset_cause(void)
{
	int32_t ret;

	ret = hwinfo_clear_reset_cause();
	if (ret == 0) {
		LOG_INF("hwinfo_clear_reset_cause() was executed");
	} else if (ret == -ENOSYS) {
		LOG_INF("hwinfo_get_reset_cause() is NOT supported");
	} else {
		LOG_ERR("hwinfo_get_reset_cause() failed (ret = %d)", ret);
	}

	/* Confirm all are cleared. */
	hwinfo_get_reset_cause(&ret);
	if (ret == 0) {
		LOG_INF("PASS: reset causes were cleared");
	} else {
		LOG_ERR("FAIL: reset case = %u while expected is 0", ret);
	}
	print_bar();
}
