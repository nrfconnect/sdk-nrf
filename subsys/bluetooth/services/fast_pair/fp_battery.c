/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/fast_pair.h>
#include "fp_battery.h"

static struct bt_fast_pair_battery_data fp_battery_data = {
	.batteries = {
		[BT_FAST_PAIR_BATTERY_COMP_LEFT_BUD]  = {
			.charging = false, .level = BT_FAST_PAIR_BATTERY_LEVEL_UNKNOWN
		},
		[BT_FAST_PAIR_BATTERY_COMP_RIGHT_BUD] = {
			.charging = false, .level = BT_FAST_PAIR_BATTERY_LEVEL_UNKNOWN
		},
		[BT_FAST_PAIR_BATTERY_COMP_BUD_CASE]  = {
			.charging = false, .level = BT_FAST_PAIR_BATTERY_LEVEL_UNKNOWN
		},
	}
};

static bool is_battery_level_valid(uint8_t level)
{
	if ((level > 100) && (level != BT_FAST_PAIR_BATTERY_LEVEL_UNKNOWN)) {
		return false;
	}

	return true;
}

int bt_fast_pair_battery_set(enum bt_fast_pair_battery_comp battery_comp,
			     struct bt_fast_pair_battery battery)
{
	if (!is_battery_level_valid(battery.level)) {
		return -EINVAL;
	}

	if ((battery_comp < 0) || (battery_comp >= BT_FAST_PAIR_BATTERY_COMP_COUNT)) {
		return -EINVAL;
	}

	BUILD_ASSERT(ARRAY_SIZE(fp_battery_data.batteries) == BT_FAST_PAIR_BATTERY_COMP_COUNT);

	fp_battery_data.batteries[battery_comp] = battery;

	return 0;
}

struct bt_fast_pair_battery_data fp_battery_get_battery_data(void)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	return fp_battery_data;
}
