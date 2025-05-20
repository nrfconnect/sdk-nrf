/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/pm/device.h>

int main(void)
{
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	printf("Hello world from cpuflpr.");
	pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);

	return 0;
}
