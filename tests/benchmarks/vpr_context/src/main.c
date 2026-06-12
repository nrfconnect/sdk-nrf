/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * This simple test shows that allocating at the end of the VPR RAM
 * where VPR context is configured to be stored
 * may break (dut dependent) user data if VPR goes to low power mode
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#define VPR_CONTEXT_SIZE 0x200

#define CPUFLPR_SRAM_NODE DT_NODELABEL(cpuflpr_sram)
#define CPUFLPR_SRAM_BASE DT_REG_ADDR(CPUFLPR_SRAM_NODE)
#define CPUFLPR_SRAM_SIZE DT_REG_SIZE(CPUFLPR_SRAM_NODE)

static uint8_t user_data_pattern[VPR_CONTEXT_SIZE];

int main(void)
{
	uint8_t *const test_ptr =
		(uint8_t *)(CPUFLPR_SRAM_BASE + CPUFLPR_SRAM_SIZE - VPR_CONTEXT_SIZE);

	printk("Hello %s\n", CONFIG_BOARD_TARGET);
	printk("VPR context area pointer = %p\n", test_ptr);

	if (memset(user_data_pattern, 0xAB, VPR_CONTEXT_SIZE) == NULL) {
		printk("User data pattern set failed\n");
		return -1;
	}
	if (memset(test_ptr, 0xAB, VPR_CONTEXT_SIZE) == NULL) {
		printk("User data set failed\n");
		return -1;
	}

	k_msleep(1000);

	if (memcmp(user_data_pattern, test_ptr, VPR_CONTEXT_SIZE)) {
		printk("User data after sleep is different than before sleep\n");
		printk("Test FAIL\n");
	} else {
		printk("Test PASS\n");
	}

	return 0;
}
