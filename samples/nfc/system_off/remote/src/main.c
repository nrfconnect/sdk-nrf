/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>

int main(void)
{
	if (IS_ENABLED(CONFIG_CONSOLE)) {
		printf("%s system off demo. Ready for system off.\n", CONFIG_BOARD_TARGET);
	}

	sys_poweroff();

	return 0;
}
