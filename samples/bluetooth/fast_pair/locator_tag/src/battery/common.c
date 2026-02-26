/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <bluetooth/fast_pair/fhn/fhn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fhn, LOG_LEVEL_INF);

enum app_battery_level_fhn {
	APP_BATTERY_LEVEL_FHN_NONE,
	APP_BATTERY_LEVEL_FHN_NORMAL,
	APP_BATTERY_LEVEL_FHN_LOW,
	APP_BATTERY_LEVEL_FHN_CRITICAL,
};

enum app_battery_level_dult {
	APP_BATTERY_LEVEL_DULT_FULL,
	APP_BATTERY_LEVEL_DULT_MEDIUM,
	APP_BATTERY_LEVEL_DULT_LOW,
	APP_BATTERY_LEVEL_DULT_CRITICAL,
};

static enum app_battery_level_fhn battery_level_fhn_get(uint8_t percentage_level)
{
	if (percentage_level == BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE) {
		return APP_BATTERY_LEVEL_FHN_NONE;
	}

	if (percentage_level <= CONFIG_BT_FAST_PAIR_FHN_BATTERY_LEVEL_CRITICAL_THR) {
		return APP_BATTERY_LEVEL_FHN_CRITICAL;
	} else if (percentage_level <= CONFIG_BT_FAST_PAIR_FHN_BATTERY_LEVEL_LOW_THR) {
		return APP_BATTERY_LEVEL_FHN_LOW;
	} else {
		return APP_BATTERY_LEVEL_FHN_NORMAL;
	}
}

static enum app_battery_level_dult battery_level_dult_get(uint8_t percentage_level)
{
	__ASSERT_NO_MSG(percentage_level != BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE);

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
	enum app_battery_level_fhn battery_level_fhn;
	enum app_battery_level_dult battery_level_dult;
	static const char * const battery_level_fhn_desc[] = {
		[APP_BATTERY_LEVEL_FHN_NONE] = "Unsupported",
		[APP_BATTERY_LEVEL_FHN_NORMAL] = "Normal",
		[APP_BATTERY_LEVEL_FHN_LOW] = "Low",
		[APP_BATTERY_LEVEL_FHN_CRITICAL] = "Critically low",
	};
	static const char * const battery_level_dult_desc[] = {
		[APP_BATTERY_LEVEL_DULT_FULL] = "Full",
		[APP_BATTERY_LEVEL_DULT_MEDIUM] = "Medium",
		[APP_BATTERY_LEVEL_DULT_LOW] = "Low",
		[APP_BATTERY_LEVEL_DULT_CRITICAL] = "Critically low",
	};

	battery_level_fhn = battery_level_fhn_get(percentage_level);

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_BATTERY_DULT)) {
		battery_level_dult = battery_level_dult_get(percentage_level);
	}

	if (percentage_level != BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE) {
		LOG_INF("FHN: setting battery level to %d %%", percentage_level);
		LOG_INF("\tFHN level: %s", battery_level_fhn_desc[battery_level_fhn]);
		if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_BATTERY_DULT)) {
			LOG_INF("\tDULT level: %s", battery_level_dult_desc[battery_level_dult]);
		}
	} else {
		LOG_INF("FHN: unknown battery level FHN level: %s",
			battery_level_fhn_desc[battery_level_fhn]);
	}
}
