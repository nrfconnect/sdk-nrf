/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

/* The loader uses devicetree chosen nodes to specify source
 * and destination memory regions:
 * - zephyr,loaded-fw-src: Source partition in NVM
 * - zephyr,loaded-fw-dst: Destination region in RAM
 */

#define LOADED_FW_NVM_NODE				DT_CHOSEN(zephyr_loaded_fw_src)
#define LOADED_FW_NVM_PARTITION_NODE			DT_PARENT(DT_PARENT(LOADED_FW_NVM_NODE))
#define LOADED_FW_NVM_ADDR				(DT_REG_ADDR(LOADED_FW_NVM_NODE) + \
							 DT_REG_ADDR(LOADED_FW_NVM_PARTITION_NODE))
#define LOADED_FW_NVM_SIZE				DT_REG_SIZE(LOADED_FW_NVM_NODE)

#define LOADED_FW_RAM_NODE				DT_CHOSEN(zephyr_loaded_fw_dst)
#define LOADED_FW_RAM_ADDR				DT_REG_ADDR(LOADED_FW_RAM_NODE)
#define LOADED_FW_RAM_SIZE				DT_REG_SIZE(LOADED_FW_RAM_NODE)

/* Verify devicetree configuration at build time */
BUILD_ASSERT(DT_NODE_EXISTS(DT_CHOSEN(zephyr_loaded_fw_src)),
	     "Missing chosen node: zephyr,loaded-fw-src");
BUILD_ASSERT(DT_NODE_EXISTS(DT_CHOSEN(zephyr_loaded_fw_dst)),
	     "Missing chosen node: zephyr,loaded-fw-dst");
BUILD_ASSERT(LOADED_FW_NVM_SIZE <= LOADED_FW_RAM_SIZE,
	     "Firmware size exceeds available TCM RAM");

/**
 * @brief Copy firmware from MRAM to TCM and jump to it
 *
 * This function runs as SYS_INIT(EARLY, 0) before main() and the scheduler.
 * It copies the firmware from MRAM to TCM for optimal performance, then
 * transfers execution to the loaded firmware's reset handler.
 *
 * This function never returns - execution transfers to the loaded firmware.
 *
 * @return 0 on success (never reached), -1 on failure (never reached)
 */
static int load_and_jump_to_firmware(void)
{
	/* Copy firmware from MRAM to TCM */
	memcpy((void *)LOADED_FW_RAM_ADDR, (void *)(LOADED_FW_NVM_ADDR), (LOADED_FW_NVM_SIZE ));

	/* Extract reset handler from ARM Cortex-M vector table (entry 1) */
	uint32_t *vector_table = (uint32_t *)(LOADED_FW_RAM_ADDR + CONFIG_ROM_START_OFFSET);
	typedef void reset_handler_t(void);
	reset_handler_t *reset_handler = (reset_handler_t *)(vector_table[1]);

	/* Jump to loaded firmware - this never returns */
	reset_handler();

	/* Should never reach here */
	return -1;
}

SYS_INIT(load_and_jump_to_firmware, EARLY, 0);

/**
 * @brief Main function - should never be reached
 *
 * If we reach main(), the firmware load and jump failed.
 * This indicates a critical error in the loader.
 */
int main(void)
{
#ifdef CONFIG_PRINTK
	printk("ERROR: Firmware jump failed!\n");
#endif
	while (1) {
		/* Hang here if jump fails */
	}
	return -1;
}
