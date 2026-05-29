/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#if IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP)
#include <zephyr/sys/reboot.h>
#endif

LOG_MODULE_REGISTER(test, 4);

int main(void)
{
	printk("Hello world from %s\n", CONFIG_BOARD_TARGET);
	k_msleep(1000);

	for (uint32_t i = 0; i < 0xFFFF; i++) {
		LOG_INF("Stm stress test: %d\n", i);
	}

#if IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP)
	k_msleep(10000);
	sys_reboot(0);
#endif

	return 0;
}
