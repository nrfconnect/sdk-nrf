/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>

const struct device *regulator_parent = DEVICE_DT_GET(DT_ALIAS(regulator_parent));

int main(void)
{
	int ret;

	if (!device_is_ready(regulator_parent)) {
		printk("%s is not ready\n", regulator_parent->name);
		return 0;
	}

	printk("%s entering ship mode\n", regulator_parent->name);
	k_busy_wait(10000);

	ret = regulator_parent_ship_mode(regulator_parent);
	printk("%s failed to enter ship mode (ret = %i)\n", regulator_parent->name, ret);
	return 0;
}
