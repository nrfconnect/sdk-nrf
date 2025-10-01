/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>

#define SLEEP_TIME_MS 1000

int main(void)
{

#if defined(CONFIG_SOC_NRF52840)
	/* [246] System: Intermittent extra current consumption when going to sleep */
	*(volatile uint32_t *)0x4007AC84ul = 0x00000002ul;
#endif

	while (1) {
		printk("33 characters long string !!!!!!\n");
		k_sleep(K_MSEC(SLEEP_TIME_MS));
	}

	return 0;
}
