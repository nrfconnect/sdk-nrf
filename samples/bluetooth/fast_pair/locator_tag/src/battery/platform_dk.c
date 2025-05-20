/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <bluetooth/services/fast_pair/fmdn.h>

#include "app_battery.h"
#include "app_battery_priv.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

/* Battery level decrement [%] that is used when handling the battery level change request. */
#define BATTERY_LEVEL_DECREMENT (CONFIG_APP_BATTERY_LEVEL_DECREMENT)

/* Maximum battery level [%]. */
#define BATTERY_LEVEL_MAX (100)

static uint8_t battery_level = BATTERY_LEVEL_MAX;

static int battery_level_propagate(void)
{
	int err;

	err = bt_fast_pair_fmdn_battery_level_set(battery_level);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_battery_level_set failed (err %d)",
			err);
		return err;
	}

	app_battery_level_log(battery_level);

	return 0;
}

static void battery_request_handle(enum app_ui_request request)
{
	if (request == APP_UI_REQUEST_SIMULATED_BATTERY_CHANGE) {
		if (battery_level == BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE) {
			battery_level = BATTERY_LEVEL_MAX;
		} else if (battery_level >= BATTERY_LEVEL_DECREMENT) {
			battery_level -= BATTERY_LEVEL_DECREMENT;
		} else {
			battery_level = IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT) ?
				BATTERY_LEVEL_MAX : BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE;
		}

		(void) battery_level_propagate();
	}
}

int app_battery_init(void)
{
	int err;

	err = battery_level_propagate();
	if (err) {
		LOG_ERR("FMDN: battery_level_propagate failed (err %d)", err);
		return err;
	}

	return 0;
}

APP_UI_REQUEST_LISTENER_REGISTER(battery_request_handler, battery_request_handle);
