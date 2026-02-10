/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <pm_config.h>
#include <fw_info.h>
#if defined(CONFIG_FPROTECT)
#include <fprotect.h>
#else
#ifndef CONFIG_SOC_SERIES_NRF54L
#warning "FPROTECT not enabled, the bootloader will be unprotected."
#endif
#endif
#include <bl_storage.h>
#include <bl_boot.h>
#include <bl_validation.h>
#if defined(CONFIG_NRFX_NVMC)
#include <nrfx_nvmc.h>
#elif defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#else
#error "No NRFX memory backend selected"
#endif

#if defined(CONFIG_HW_UNIQUE_KEY_LOAD)
#include <zephyr/init.h>
#include <hw_unique_key.h>

#define HUK_FLAG_OFFSET 0xFFC /* When this word is set, expect HUK to be written. */

int load_huk(void)
{
	if (!hw_unique_key_is_written(HUK_KEYSLOT_KDR)) {
		uint32_t huk_flag_addr = PM_HW_UNIQUE_KEY_PARTITION_ADDRESS + HUK_FLAG_OFFSET;

		if (*(uint32_t *)huk_flag_addr == 0xFFFFFFFF) {
			printk("First boot, expecting app to write HUK.\n");
#if defined(CONFIG_NRFX_NVMC)
			nrfx_nvmc_word_write(huk_flag_addr, 0);
#elif defined(CONFIG_NRFX_RRAMC)
			nrfx_rramc_word_write(huk_flag_addr, 0);
#endif
			return 0;
		}
		printk("Error: Hardware Unique Key not present.\n");
		k_panic();
		return -1;

	}

	if (hw_unique_key_load_kdr() != HW_UNIQUE_KEY_SUCCESS) {
		printk("Error: Cannot load the Hardware Unique Key into the KDR.\n");
		k_panic();
		return -1;
	}

	return 0;
}

SYS_INIT(load_huk, PRE_KERNEL_2, 0);
#endif


static void validate_and_boot(const struct fw_info *fw_info, counter_t slot)
{
	printk("Attempting to boot slot %d.\r\n", slot);

	if (fw_info == NULL) {
		printk("No fw_info struct found.\r\n");
		return;
	}

	printk("Attempting to boot from address 0x%x.\r\n",
		fw_info->address);

	if (!bl_validate_firmware_local(fw_info->address,
					fw_info)) {
		printk("Failed to validate, permanently invalidating!\r\n");
		fw_info_invalidate(fw_info);
		return;
	}

	printk("Firmware version %d\r\n", fw_info->version);

#ifdef CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION
	counter_t stored_version;

	int err = get_monotonic_version(&stored_version);

	if (err) {
		printk("Failed to read the monotonic counter!\r\n");
		return;
	}

	if (fw_info->version > stored_version) {
		int err = set_monotonic_version(fw_info->version, slot);

		if (err) {
			return;
		}
	}
#endif

	/*
	 * We can lock the keys and other resources now, as any failures
	 * in the bl_boot function are considered fatal and would prevent
	 * the alternate image from booting as well.
	 * Thus, we meet the criteria for calling bl_validate_housekeeping.
	 */
	bl_validate_housekeeping();

	bl_boot(fw_info);
}

#define BOOT_SLOT_0 0
#define BOOT_SLOT_1 1

int main(void)
{

#if defined(CONFIG_FPROTECT)
	int err = fprotect_area(PM_B0_ADDRESS, PM_B0_SIZE);

	if (err) {
		printk("Failed to protect B0 flash, cancel startup.\r\n");
		return 0;
	}
#else
	printk("Fprotect disabled. No protection applied.\r\n");
#endif

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

	printk("No bootable image found. Aborting boot.\r\n");
	bl_validate_housekeeping();
	return 0;
}
