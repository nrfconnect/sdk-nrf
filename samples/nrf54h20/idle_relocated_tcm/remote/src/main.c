/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(idle);

int main(void)
{
	unsigned int cnt = 0;
	uintptr_t pc;

	__asm__ volatile("adr %0, ." : "=r"(pc));
	LOG_INF("Multicore idle test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Current PC (program counter) address: 0x%lx\n", (unsigned long)pc);

	/* using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	 LOG_INF("build time: " __DATE__ " " __TIME__);

	while (1) {
		LOG_INF("Multicore idle test iteration %u", cnt++);
		k_msleep(2000);
	}

	return 0;
}
