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
#include <ironside/se/boot_report.h>

#define MRAM_WRITE_BLOCK_SIZE 16
#define ERASE_OFFSET                                                                               \
	(PARTITION_OFFSET(protectedmem_partition) + PARTITION_SIZE(protectedmem_partition) -       \
	 MRAM_WRITE_BLOCK_SIZE)

int main(void)
{
	printk("Secondary: Secondary firmware booted\n");

	const struct ironside_se_boot_report *const report = IRONSIDE_SE_BOOT_REPORT;

	if (report->init_context.boot_reason == IRONSIDE_SE_BOOT_REASON_BOOTERROR) {
		if (report->init_context.trigger_init_status.boot_error ==
		    IRONSIDE_SE_BOOT_ERROR_UICR_PROTECTEDMEM_INTEGRITY_FAILED) {
			printk("Secondary: Booted because of PROTECTEDMEM failure\n");
		} else {
			printk("Secondary: Booted because of unexpected boot error: 0x%x\n",
			       report->init_context.trigger_init_status.boot_error);
		}
	} else {
		printk("Secondary: Booted because of unexpected reason: 0x%x\n",
		       report->init_context.boot_reason);
	}

	printk("Secondary: Reverting test pattern write to protected memory...\n");

	const struct device *flash_dev = PARTITION_DEVICE(protectedmem_partition);
	int rc = flash_erase(flash_dev, ERASE_OFFSET, MRAM_WRITE_BLOCK_SIZE);

	if (rc) {
		printk("Secondary: flash_erase failed: %d\n", rc);
		return rc;
	}

	printk("Secondary: Rebooting after 1s. Primary firmware should boot again since the "
	       "corruption was fixed.\n");

	k_msleep(1000);
	NVIC_SystemReset();

	CODE_UNREACHABLE;

	return 0;
}
