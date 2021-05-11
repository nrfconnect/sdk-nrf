/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <sys/printk.h>
#include <pm_config.h>
#include <fw_info.h>
#include <fprotect.h>
#include <bl_storage.h>
#include <bl_boot.h>
#include <bl_validation.h>

#if defined(CONFIG_HW_UNIQUE_KEY)
#include <init.h>
#include <hw_unique_key.h>

int load_huk(const struct device *unused)
{
	(void)unused;

	if (!hw_unique_key_is_written(HUK_KEYSLOT_KDR)) {
		printk("Error: Hardware Unique Key not present.\n");
		k_panic();
		return -1;
	}

	hw_unique_key_load_kdr();

	return 0;
}

SYS_INIT(load_huk, PRE_KERNEL_2, 0);
#endif


static void validate_and_boot(const struct fw_info *fw_info, uint16_t slot)
{
	printk("Attempting to boot slot %d.\r\n", slot);

	if (fw_info == NULL) {
		printk("No fw_info struct found.\r\n");
		return;
	}

	printk("Attempting to boot from address 0x%x.\n\r",
		fw_info->address);

	if (!bl_validate_firmware_local(fw_info->address,
					fw_info)) {
		printk("Failed to validate, permanently invalidating!\n\r");
		fw_info_invalidate(fw_info);
		return;
	}

	printk("Firmware version %d\r\n", fw_info->version);

	if (fw_info->version > get_monotonic_version(NULL)) {
		set_monotonic_version(fw_info->version, slot);
	}

	bl_boot(fw_info);
}

#define BOOT_SLOT_0 0
#define BOOT_SLOT_1 1

void main(void)
{
	int err = fprotect_area(PM_B0_ADDRESS, PM_B0_SIZE);

	if (err) {
		printk("Failed to protect B0 flash, cancel startup.\n\r");
		return;
	}

	uint32_t s0_addr = s0_address_read();
	uint32_t s1_addr = s1_address_read();
	const struct fw_info *s0_info = fw_info_find(s0_addr);
	const struct fw_info *s1_info = fw_info_find(s1_addr);

	if (!s1_info || (s0_info->version >= s1_info->version)) {
		validate_and_boot(s0_info, BOOT_SLOT_0);
		validate_and_boot(s1_info, BOOT_SLOT_1);
	} else {
		validate_and_boot(s1_info, BOOT_SLOT_1);
		validate_and_boot(s0_info, BOOT_SLOT_0);
	}

	printk("No bootable image found. Aborting boot.\n\r");
	return;
}
