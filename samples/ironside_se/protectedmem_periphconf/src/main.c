/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/arch/cpu.h>

#define MRAM_WRITE_BLOCK_SIZE 16
#define TEST_WRITE_OFFSET                                                                          \
	(PARTITION_OFFSET(protectedmem_partition) + PARTITION_SIZE(protectedmem_partition) -       \
	 MRAM_WRITE_BLOCK_SIZE)

static const uint8_t test_pattern[MRAM_WRITE_BLOCK_SIZE] = {
	0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
	0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
};

int main(void)
{
	printk("Writing test pattern to protected memory...\n");

	const struct device *flash_dev = PARTITION_DEVICE(protectedmem_partition);
	int rc = flash_write(flash_dev, TEST_WRITE_OFFSET, test_pattern, sizeof(test_pattern));

	if (rc) {
		printk("flash_write failed: %d\n", rc);
		return rc;
	}

	printk("Rebooting after 1s. Secondary firmware should boot if protection is working.\n");

	k_msleep(1000);
	NVIC_SystemReset();

	CODE_UNREACHABLE;

	return 0;
}
