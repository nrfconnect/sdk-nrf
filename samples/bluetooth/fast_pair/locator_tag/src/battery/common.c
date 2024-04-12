/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <bluetooth/services/fast_pair/fmdn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

enum app_battery_level_fmdn {
	APP_BATTERY_LEVEL_FMDN_NONE,
	APP_BATTERY_LEVEL_FMDN_NORMAL,
	APP_BATTERY_LEVEL_FMDN_LOW,
	APP_BATTERY_LEVEL_FMDN_CRITICAL,
};

enum app_battery_level_dult {
	APP_BATTERY_LEVEL_DULT_FULL,
	APP_BATTERY_LEVEL_DULT_MEDIUM,
	APP_BATTERY_LEVEL_DULT_LOW,
	APP_BATTERY_LEVEL_DULT_CRITICAL,
};

static enum app_battery_level_fmdn battery_level_fmdn_get(uint8_t percentage_level)
{
	if (percentage_level == BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE) {
		return APP_BATTERY_LEVEL_FMDN_NONE;
	}

	if (percentage_level <= CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_CRITICAL_THR) {
		return APP_BATTERY_LEVEL_FMDN_CRITICAL;
	} else if (percentage_level <= CONFIG_BT_FAST_PAIR_FMDN_BATTERY_LEVEL_LOW_THR) {
		return APP_BATTERY_LEVEL_FMDN_LOW;
	} else {
		return APP_BATTERY_LEVEL_FMDN_NORMAL;
	}
}

static enum app_battery_level_dult battery_level_dult_get(uint8_t percentage_level)
{
	__ASSERT_NO_MSG(percentage_level != BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE);

	if (percentage_level <= CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR) {
		return APP_BATTERY_LEVEL_DULT_CRITICAL;
	} else if (percentage_level <= CONFIG_DULT_BATTERY_LEVEL_LOW_THR) {
		return APP_BATTERY_LEVEL_DULT_LOW;
	} else if (percentage_level <= CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR) {
		return APP_BATTERY_LEVEL_DULT_MEDIUM;
	} else {
		return APP_BATTERY_LEVEL_DULT_FULL;
	}
}

void app_battery_level_log(uint8_t percentage_level)
{
	enum app_battery_level_fmdn battery_level_fmdn;
	enum app_battery_level_dult battery_level_dult;
	static const char * const battery_level_fmdn_desc[] = {
		[APP_BATTERY_LEVEL_FMDN_NONE] = "Unsupported",
		[APP_BATTERY_LEVEL_FMDN_NORMAL] = "Normal",
		[APP_BATTERY_LEVEL_FMDN_LOW] = "Low",
		[APP_BATTERY_LEVEL_FMDN_CRITICAL] = "Critically low",
	};
	static const char * const battery_level_dult_desc[] = {
		[APP_BATTERY_LEVEL_DULT_FULL] = "Full",
		[APP_BATTERY_LEVEL_DULT_MEDIUM] = "Medium",
		[APP_BATTERY_LEVEL_DULT_LOW] = "Low",
		[APP_BATTERY_LEVEL_DULT_CRITICAL] = "Critically low",
	};

	battery_level_fmdn = battery_level_fmdn_get(percentage_level);

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT)) {
		battery_level_dult = battery_level_dult_get(percentage_level);
	}

	if (percentage_level != BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE) {
		LOG_INF("FMDN: setting battery level to %d %%", percentage_level);
		LOG_INF("\tFMDN level: %s", battery_level_fmdn_desc[battery_level_fmdn]);
		if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT)) {
			LOG_INF("\tDULT level: %s", battery_level_dult_desc[battery_level_dult]);
		}
	} else {
		LOG_INF("FMDN: unknown battery level FMDN level: %s",
			battery_level_fmdn_desc[battery_level_fmdn]);
	}
}
