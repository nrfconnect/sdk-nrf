/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf71_idle_power.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define IDLE_TIME K_SECONDS(CONFIG_SAMPLE_POWER_CONSUMPTION_IDLE_SECONDS)

int main(void)
{
#if defined(CONFIG_SERIAL)
	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(console)) {
		return 0;
	}

	printk("\nPower consumption demo ready.\n");

	uint32_t wakeups = 0;
#endif

	while (true) {
		nrf71_idle_power_suspend_console();
		k_sleep(IDLE_TIME);
		nrf71_idle_power_resume_console();

#if defined(CONFIG_SERIAL)
		wakeups++;
		printk("Woken up %u time(s)\n", wakeups);
#endif
	}

	return 0;
}
