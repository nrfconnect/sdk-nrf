/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(app);

int main(void)
{
#if !defined(CONFIG_MULTITHREADING)
	while (1) {
		while (LOG_PROCESS()) {
		}
		k_cpu_idle();
	}
#else
	k_sleep(K_FOREVER);
#endif

	return 0;
}
