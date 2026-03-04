/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/fuel_gauge.h>

#include <bluetooth/fast_pair/fhn/fhn.h>

#include "app_battery.h"
#include "app_battery_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fhn, LOG_LEVEL_DBG);

#define FUEL_GAUGE_NODE		DT_ALIAS(battery_fuel_gauge)
static const struct device *fuel_gauge_dev = DEVICE_DT_GET(FUEL_GAUGE_NODE);

static void battery_poll_work_handle(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(battery_poll_work, battery_poll_work_handle);

static int battery_level_set(uint8_t level, bool forced_set)
{
	int err;
	static uint8_t old_level;

	if (!forced_set && (old_level == level)) {
		LOG_DBG("FHN: battery level did not change");
		return 0;
	}

	err = bt_fast_pair_fhn_battery_level_set(level);
	if (err) {
		LOG_ERR("FHN: bt_fast_pair_fhn_battery_level_set failed (err %d)", err);
		return err;
	}

	old_level = level;

	app_battery_level_log(level);

	return 0;
}

static void battery_poll_work_handle(struct k_work *work)
{
	int err;
	static bool first_run_done;
	static const fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_VOLTAGE,
		FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
	};
	union fuel_gauge_prop_val poll_vals[ARRAY_SIZE(props)];
	int voltage;
	uint8_t percentage_level;

	LOG_DBG("Battery poll triggered");

	err = fuel_gauge_get_props(fuel_gauge_dev, props, poll_vals, ARRAY_SIZE(props));
	if (err) {
		LOG_ERR("Failed to get fuel gauge properties: %d", err);
		(void)k_work_reschedule(&battery_poll_work,
					K_SECONDS(CONFIG_APP_BATTERY_POLL_INTERVAL));
		return;
	}

	voltage = poll_vals[0].voltage;
	percentage_level = poll_vals[1].relative_state_of_charge;

	LOG_DBG("Battery state: voltage: %d [uV], state of charge: %" PRIu8 " [%%]",
		voltage, percentage_level);

	__ASSERT_NO_MSG(voltage >= 0);
	__ASSERT_NO_MSG(percentage_level <= 100);

	err = battery_level_set(percentage_level, !first_run_done);
	if (!err) {
		first_run_done = true;
	}

	(void)k_work_reschedule(&battery_poll_work, K_SECONDS(CONFIG_APP_BATTERY_POLL_INTERVAL));
}

int app_battery_init(void)
{
	if (!device_is_ready(fuel_gauge_dev)) {
		LOG_ERR("Fuel gauge device not ready");
		return -ENODEV;
	}

	LOG_INF("Fuel gauge device ready");

	battery_poll_work_handle(NULL);

	return 0;
}
