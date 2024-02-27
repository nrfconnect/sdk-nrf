/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/fast_pair/fast_pair.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_sample, LOG_LEVEL_INF);

#include "battery_module.h"

int battery_module_init(void)
{
	static const struct bt_fast_pair_battery fp_batteries[BT_FAST_PAIR_BATTERY_COMP_COUNT] = {
		[BT_FAST_PAIR_BATTERY_COMP_LEFT_BUD]  = { .charging = true, .level = 67},
		[BT_FAST_PAIR_BATTERY_COMP_RIGHT_BUD] = { .charging = false, .level = 100},
		[BT_FAST_PAIR_BATTERY_COMP_BUD_CASE]  = { .charging = false, .level = 1},
	};
	int err;

	for (size_t i = 0; i < BT_FAST_PAIR_BATTERY_COMP_COUNT; i++) {
		err = bt_fast_pair_battery_set(i, fp_batteries[i]);
		if (err) {
			LOG_ERR("Unable to set battery value (err %d, idx %zu)", err, i);
			return err;
		}
	}

	return 0;
}
