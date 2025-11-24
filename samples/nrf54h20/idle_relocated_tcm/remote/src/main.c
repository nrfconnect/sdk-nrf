/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(idle);

#define CODE_PARTITION_NODE		DT_CHOSEN(zephyr_code_partition)
#define CODE_PARTITION_START_ADDR	DT_FIXED_PARTITION_ADDR(CODE_PARTITION_NODE)

#define SRAM_NODE			DT_CHOSEN(zephyr_sram)
#define SRAM_START_ADDR			DT_REG_ADDR(SRAM_NODE)
#define SRAM_SIZE			DT_REG_SIZE(SRAM_NODE)

int main(void)
{
	unsigned int cnt = 0;
	uintptr_t pc;

	__asm__ volatile("adr %0, ." : "=r"(pc));
	LOG_INF("Multicore idle test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Original code partition start address: 0x%lx",
		(unsigned long)CODE_PARTITION_START_ADDR);
	LOG_INF("Running from the SRAM, start address: 0x%lx, size: 0x%lx",
		(unsigned long)SRAM_START_ADDR, (unsigned long)SRAM_SIZE);
	LOG_INF("Current PC (program counter) address: 0x%lx", (unsigned long)pc);

	/* Using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	LOG_INF("build time: " __DATE__ " " __TIME__);

	while (1) {
		LOG_INF("Multicore idle test iteration %u", cnt++);
		k_msleep(2000);
	}

	return 0;
}
