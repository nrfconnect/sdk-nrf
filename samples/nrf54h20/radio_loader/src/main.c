/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define FIXED_PARTITION_ADDRESS(label)                                                             \
	(DT_REG_ADDR(DT_NODELABEL(label)) +                                                        \
	 DT_REG_ADDR(COND_CODE_1(DT_FIXED_SUBPARTITION_EXISTS(DT_NODELABEL(label)),                \
			(DT_GPARENT(DT_PARENT(DT_NODELABEL(label)))),                              \
			(DT_GPARENT(DT_NODELABEL(label))))))

#define FIXED_PARTITION_SIZE(label) DT_REG_SIZE(DT_NODELABEL(label))

#ifdef CONFIG_USE_DT_CODE_PARTITION
#define FLASH_LOAD_OFFSET DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition))
#elif defined(CONFIG_FLASH_LOAD_OFFSET)
#define FLASH_LOAD_OFFSET CONFIG_FLASH_LOAD_OFFSET
#endif

#define PARTITION_IS_RUNNING_APP_PARTITION(label)                                                  \
	(DT_REG_ADDR(DT_NODELABEL(label)) <= FLASH_LOAD_OFFSET &&                                  \
	 DT_REG_ADDR(DT_NODELABEL(label)) + DT_REG_SIZE(DT_NODELABEL(label)) > FLASH_LOAD_OFFSET)

/* Used to determine the running slot of the radio loader. */
#define RADIO_LOADER_PRIMARY_SLOT	cpurad_slot0_partition
#define RADIO_LOADER_SECONDARY_SLOT	cpurad_slot1_partition

/* Used to determine the loaded firmware source address from NVM and size. */
#define LOADED_FW_PRIMARY_SLOT		cpurad_slot2_partition
#define LOADED_FW_SECONDARY_SLOT	cpurad_slot3_partition

/* Used to determine the running slot of the radio core. */
#define RUNNING_FW_SLOT			cpurad_ram0
#define RUNNING_FW_SLOT_NODE		DT_NODELABEL(RUNNING_FW_SLOT)
#define RUNNING_FW_SLOT_ADDR		DT_REG_ADDR(RUNNING_FW_SLOT_NODE)
#define RUNNING_FW_SLOT_SIZE		DT_REG_SIZE(RUNNING_FW_SLOT_NODE)

BUILD_ASSERT(DT_NODE_EXISTS(DT_NODELABEL(RADIO_LOADER_PRIMARY_SLOT)),
			    "Missing nodelabel: cpurad_slot0_partition");
BUILD_ASSERT(DT_NODE_EXISTS(DT_NODELABEL(RADIO_LOADER_SECONDARY_SLOT)),
			    "Missing nodelabel: cpurad_slot1_partition");

BUILD_ASSERT(DT_NODE_EXISTS(DT_NODELABEL(LOADED_FW_PRIMARY_SLOT)),
			    "Missing nodelabel: cpurad_slot2_partition");
BUILD_ASSERT(DT_NODE_EXISTS(DT_NODELABEL(LOADED_FW_SECONDARY_SLOT)),
			    "Missing nodelabel: cpurad_slot3_partition");
BUILD_ASSERT((FIXED_PARTITION_SIZE(LOADED_FW_PRIMARY_SLOT) ==
	     FIXED_PARTITION_SIZE(LOADED_FW_SECONDARY_SLOT)),
	     "LOADED_FW_PRIMARY_SLOT and LOADED_FW_SECONDARY_SLOT sizes are not equal");

BUILD_ASSERT(DT_NODE_EXISTS(DT_NODELABEL(RUNNING_FW_SLOT)),
			    "Missing nodelabel: cpurad_ram0");
BUILD_ASSERT((FIXED_PARTITION_SIZE(LOADED_FW_PRIMARY_SLOT) <= RUNNING_FW_SLOT_SIZE),
	     "LOADED_FW_PRIMARY_SLOT size exceeds RUNNING_FW_SLOT size");
BUILD_ASSERT((FIXED_PARTITION_SIZE(LOADED_FW_SECONDARY_SLOT) <= RUNNING_FW_SLOT_SIZE),
	     "LOADED_FW_SECONDARY_SLOT size exceeds RUNNING_FW_SLOT size");

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
	/* Get the loaded firmware source address from NVM and size */
	void *loaded_fw_nvm_addr = NULL;
	size_t loaded_fw_nvm_size = 0;

	/* Get the loaded firmware destination address from RAM and size */
	void *loaded_fw_ram_addr = (void *)(RUNNING_FW_SLOT_ADDR);

	if (PARTITION_IS_RUNNING_APP_PARTITION(RADIO_LOADER_PRIMARY_SLOT)) {
		loaded_fw_nvm_addr = (void *)(FIXED_PARTITION_ADDRESS(LOADED_FW_PRIMARY_SLOT));
		loaded_fw_nvm_size = (size_t)(FIXED_PARTITION_SIZE(LOADED_FW_PRIMARY_SLOT));
	} else {
		loaded_fw_nvm_addr = (void *)(FIXED_PARTITION_ADDRESS(LOADED_FW_SECONDARY_SLOT));
		loaded_fw_nvm_size = (size_t)(FIXED_PARTITION_SIZE(LOADED_FW_SECONDARY_SLOT));
	}

	/* Copy firmware from MRAM to TCM */
	memcpy(loaded_fw_ram_addr, loaded_fw_nvm_addr, loaded_fw_nvm_size);

	/* Extract reset handler from ARM Cortex-M vector table (entry 1) */
	uint32_t *vector_table =
		(uint32_t *)((uint8_t *)loaded_fw_ram_addr + CONFIG_ROM_START_OFFSET);
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
