/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <pm_config.h>
#include <fw_info.h>
#include <fprotect.h>
#include <bl_storage.h>
#include <bl_boot.h>
#include <bl_validation.h>
#include <dfu/pcd.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#ifdef CONFIG_PCD_LOCK_NETCORE_APPROTECT
#include <nrfx_nvmc.h>
#endif

int main(void)
{
	int err;
	const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (!device_is_ready(fdev)) {
		printk("Flash device not ready\n");
		return 0;
	}

	/* The flash is locked at flash page granularity */
	BUILD_ASSERT(
		(PM_B0N_CONTAINER_SIZE % CONFIG_FPROTECT_BLOCK_SIZE) == 0,
		"PM_B0N_CONTAINER_SIZE % CONFIG_FPROTECT_BLOCK_SIZE was not 0. Check the B0_SIZE Kconfig.");

	err = fprotect_area(PM_B0N_CONTAINER_ADDRESS, PM_B0N_CONTAINER_SIZE);
	if (err) {
		printk("Failed to protect b0n flash, cancel startup\n\r");
		goto failure;
	}

	uint32_t s0_addr = s0_address_read();
	bool valid = false;

	switch (pcd_fw_copy_status_get()) {
#ifdef CONFIG_PCD_LOCK_NETCORE_DEBUG
	case PCD_STATUS_LOCK_DEBUG:
		nrfx_nvmc_word_write((uint32_t)&NRF_UICR_NS->APPROTECT,
				     UICR_APPROTECT_PALL_Protected);
		while (!nrfx_nvmc_write_done_check())
			;

		pcd_done();

		/* Success, waiting to be rebooted */
		while (1)
			;
		CODE_UNREACHABLE;
	break;
#endif

#ifdef CONFIG_PCD_READ_NETCORE_APP_VERSION
	case PCD_STATUS_READ_VERSION:
		err = pcd_find_fw_version();
		if (err < 0) {
			printk("Unable to find valid firmware version %d\n\r", err);
			pcd_fw_copy_invalidate();
		}
		pcd_done();

		/* Success, waiting to be rebooted */
		while (1)
			;
		CODE_UNREACHABLE;
	break;
#endif

	case PCD_STATUS_COPY:
		/* First we validate the data where the PCD CMD tells
		 * us that we can find it.
		 */
		uint32_t update_addr = (uint32_t)pcd_cmd_data_ptr_get();

		valid = bl_validate_firmware(s0_addr, update_addr);
		if (!valid) {
			printk("Unable to find valid firmware inside %p\n\r",
				(void *)update_addr);
			goto failure;
		}

		err = pcd_fw_copy(fdev);
		if (err != 0) {
			printk("Failed to transfer image: %d\n\r", err);
			goto failure;
		}

		/* Note that only the SHA is validated, no signature
		 * check is performed. This because the signature validation
		 * is performed by the application core. This check is only
		 * done to verify that the flash copy operation was successful.
		 */
		valid = bl_validate_firmware(s0_addr, s0_addr);
		if (valid) {
			pcd_done();
		} else {
			printk("Unable to find valid firmware inside %p\n\r",
				(void *)s0_addr);
			goto failure;
		}

		/* Success, waiting to be rebooted */
		while (1)
			;
		CODE_UNREACHABLE;
	break;

	default:
		break;
	}

	err = fprotect_area(PM_APP_ADDRESS, PM_APP_SIZE);
	if (err) {
		printk("Failed to protect app flash: %d\n\r", err);
		goto failure;
	}

	bl_boot(fw_info_find(s0_addr));
	return 0;

failure:
	pcd_fw_copy_invalidate();
	return 0;
}
