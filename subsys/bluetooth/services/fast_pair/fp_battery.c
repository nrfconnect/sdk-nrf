/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/fast_pair.h>
#include "fp_battery.h"


static struct bt_fast_pair_battery_data fp_battery_data = {
	.left_bud = { .charging = false, .level = BT_FAST_PAIR_UNKNOWN_BATTERY_LEVEL},
	.right_bud = { .charging = false, .level = BT_FAST_PAIR_UNKNOWN_BATTERY_LEVEL},
	.bud_case = { .charging = false, .level = BT_FAST_PAIR_UNKNOWN_BATTERY_LEVEL}
	};


static bool validate_battery_value(struct bt_fast_pair_battery_value battery_value)
{
	if ((battery_value.level > 100) &&
	    (battery_value.level != BT_FAST_PAIR_UNKNOWN_BATTERY_LEVEL)) {
		return false;
	}

	return true;
}

int bt_fast_pair_set_battery_data(struct bt_fast_pair_battery_data battery_data)
{
	if (!validate_battery_value(battery_data.left_bud)) {
		return -EINVAL;
	}

	if (!validate_battery_value(battery_data.right_bud)) {
		return -EINVAL;
	}

	if (!validate_battery_value(battery_data.bud_case)) {
		return -EINVAL;
	}

	fp_battery_data = battery_data;

	return 0;
}

struct bt_fast_pair_battery_data fp_battery_get_battery_data(void)
{
	return fp_battery_data;
}
