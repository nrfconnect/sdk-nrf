/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <sys/printk.h>
#include <pm_config.h>
#include <fw_info.h>
#include <fprotect.h>
#include <bl_storage.h>
#include <bl_boot.h>
#include <bl_validation.h>


static void validate_and_boot(const struct fw_info *fw_info)
{
	printk("Attempting to boot from address 0x%x.\n\r",
		fw_info->address);

	if (!bl_validate_firmware_local(fw_info->address,
					fw_info)) {
		printk("Failed to validate, permanently invalidating!\n\r");
		fw_info_invalidate(fw_info);
		return;
	}

	bl_boot(fw_info);
}

void main(void)
{
	int err = fprotect_area(PM_B0_IMAGE_ADDRESS, PM_B0_IMAGE_SIZE);

	if (err) {
		printk("Failed to protect B0 flash, cancel startup.\n\r");
		return;
	}

	u32_t s0_addr = s0_address_read();
	u32_t s1_addr = s1_address_read();
	const struct fw_info *s0_info = fw_info_find(s0_addr);
	const struct fw_info *s1_info = fw_info_find(s1_addr);

	if (!s1_info || (s0_info->version >= s1_info->version)) {
		validate_and_boot(s0_info);
		validate_and_boot(s1_info);
	} else {
		validate_and_boot(s1_info);
		validate_and_boot(s0_info);
	}

	printk("No bootable image found. Aborting boot.\n\r");
	return;
}
