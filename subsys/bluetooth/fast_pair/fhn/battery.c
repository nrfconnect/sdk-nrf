/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_battery, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <dult.h>
#include <bluetooth/fast_pair/fhn/fhn.h>

#include "fp_activation.h"
#include "fp_fhn_battery.h"
#include "fp_fhn_dult_integration.h"

/* Maximum value of battery level in percentages. */
#define BATTERY_LEVEL_MAX (100)

/* Validate the battery configuration. */
BUILD_ASSERT(CONFIG_BT_FAST_PAIR_FHN_BATTERY_LEVEL_LOW_THR >=
	     CONFIG_BT_FAST_PAIR_FHN_BATTERY_LEVEL_CRITICAL_THR);

/* Validate the DULT battery configuration. */
BUILD_ASSERT(IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_BATTERY_DULT) ||
	     !IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_DULT) ||
	     !IS_ENABLED(CONFIG_DULT_BATTERY),
	     "FHN DULT integration must include battery integration with CONFIG_DULT_BATTERY=y");

static bool is_enabled;
static uint8_t battery_level = BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE;
static const struct fp_fhn_battery_cb *battery_cb;

static enum fp_fhn_battery_level fhn_battery_level_convert(uint8_t percentage_level)
{
	if (percentage_level == BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE) {
		return FP_FHN_BATTERY_LEVEL_NONE;
	}

	__ASSERT(percentage_level <= BATTERY_LEVEL_MAX,
		 "FHN: Battery: incorrect input level");

	if (percentage_level <= CONFIG_BT_FAST_PAIR_FHN_BATTERY_LEVEL_CRITICAL_THR) {
		return FP_FHN_BATTERY_LEVEL_CRITICAL;
	} else if (percentage_level <= CONFIG_BT_FAST_PAIR_FHN_BATTERY_LEVEL_LOW_THR) {
		return FP_FHN_BATTERY_LEVEL_LOW;
	} else {
		return FP_FHN_BATTERY_LEVEL_NORMAL;
	}
}

enum fp_fhn_battery_level fp_fhn_battery_level_get(void)
{
	return fhn_battery_level_convert(battery_level);
}

int bt_fast_pair_fhn_battery_level_set(uint8_t percentage_level)
{
	int err;
	uint8_t old_battery_level = battery_level;
	enum fp_fhn_battery_level old_fhn_batter_level;
	enum fp_fhn_battery_level fhn_battery_level;

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_BATTERY_DULT) &&
	    (percentage_level == BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE)) {
		LOG_ERR("FHN: Battery: unknown level not supported in DULT battery config");
		return -EINVAL;
	}

	if ((percentage_level > BATTERY_LEVEL_MAX) &&
	    (percentage_level != BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE)) {
		LOG_ERR("FHN: Battery: incorrect battery level in %%");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_BATTERY_DULT) && is_enabled) {
		err = dult_battery_level_set(fp_fhn_dult_integration_user_get(),
					     percentage_level);
		if (err) {
			LOG_ERR("FHN: Battery: dult_battery_level_set returned error: %d", err);
			return err;
		}
	}

	battery_level = percentage_level;

	if (battery_cb) {
		old_fhn_batter_level = fhn_battery_level_convert(old_battery_level);
		fhn_battery_level = fhn_battery_level_convert(battery_level);
		if (fhn_battery_level != old_fhn_batter_level) {
			battery_cb->battery_level_changed();
		}
	}

	return 0;
}

int fp_fhn_battery_cb_register(const struct fp_fhn_battery_cb *cb)
{
	__ASSERT(cb && cb->battery_level_changed,
		 "FHN: Battery: incorrect callback parameter");
	__ASSERT(!battery_cb, "FHN: Battery: only one callback supported");

	battery_cb = cb;

	return 0;
}

static int init(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_BATTERY_DULT)) {
		if (battery_level == BT_FAST_PAIR_FHN_BATTERY_LEVEL_NONE) {
			LOG_ERR("FHN: Battery: battery level unset before the enable operation");
			return -EINVAL;
		}

		err = dult_battery_level_set(fp_fhn_dult_integration_user_get(), battery_level);
		if (err) {
			LOG_ERR("FHN: Battery: dult_battery_level_set returned error: %d", err);
			return err;
		}
	}

	is_enabled = true;

	return 0;
}

static int uninit(void)
{
	is_enabled = false;
	battery_cb = NULL;

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fhn_battery,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      init,
			      uninit);
