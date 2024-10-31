/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <zephyr/pm/device.h>


int main(void)
{
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	printf("Hello world from cpuapp!\n");
	k_sleep(K_SECONDS(1));
	z_nrf_grtc_wakeup_prepare(1 * USEC_PER_SEC);

	printf("Entering system off, waking up in 1s.\n");
	pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	sys_poweroff();

	return 0;
}
