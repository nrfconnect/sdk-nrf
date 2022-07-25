/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash/flash_simulator.h>
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"

#define NET_CORE_SECONDARY_SLOT 1
#define NET_CORE_VIRTUAL_PRIMARY_SLOT 3
#define NRF5340_CPUNET_FLASH_SIZE 0x40000

#include <dfu/pcd.h>

#ifndef CONFIG_FLASH_SIMULATOR
static uint8_t mock_flash[NRF5340_CPUNET_FLASH_SIZE];
#endif
int network_core_update(bool wait);

int boot_read_image_header_hook(int img_index, int slot,
		struct image_header *img_head)
{

#ifdef PM_MCUBOOT_PRIMARY_1_ADDRESS
	if (img_index == 1 && slot == 0) {
		img_head->ih_magic = IMAGE_MAGIC;
		img_head->ih_hdr_size = PM_MCUBOOT_PAD_SIZE;
		img_head->ih_load_addr = PM_MCUBOOT_PRIMARY_1_ADDRESS;
		img_head->ih_img_size = PM_CPUNET_APP_SIZE;
		img_head->ih_flags = 0;
		img_head->ih_ver.iv_major = 0;
		img_head->ih_ver.iv_minor = 0;
		img_head->ih_ver.iv_revision = 0;
		img_head->ih_ver.iv_build_num = 0;
		img_head->_pad1 = 0;
		return 0;
	}
#endif

	return BOOT_HOOK_REGULAR;
}

fih_int boot_image_check_hook(int img_index, int slot)
{
	if (img_index == 1 && slot == 0) {
		FIH_RET(FIH_SUCCESS);
	}

	FIH_RET(fih_int_encode(BOOT_HOOK_REGULAR));
}

int boot_perform_update_hook(int img_index, struct image_header *img_head,
		const struct flash_area *area)
{

#ifndef CONFIG_FLASH_SIMULATOR
	uint32_t reset_addr = 0;
	int32_t rc;

	rc = flash_area_read(area, img_head->ih_hdr_size + 4, &reset_addr, 4);

	if (reset_addr > PM_CPUNET_B0N_ADDRESS) {
		if (img_head->ih_hdr_size + img_head->ih_img_size > NRF5340_CPUNET_FLASH_SIZE) {
			return 1;
		}
		rc = flash_area_read(area, 0, (uint8_t *)mock_flash,
				     img_head->ih_hdr_size + img_head->ih_img_size);
		if (rc == 0) {
			struct flash_sector fs[CONFIG_BOOT_MAX_IMG_SECTORS];
			uint32_t count = CONFIG_BOOT_MAX_IMG_SECTORS;

			rc = network_core_update(true);

			//erase the trailer of secondary slot
			flash_area_get_sectors(area->fa_id, &count, fs);
			flash_area_erase(area, area->fa_size - fs[0].fs_size, fs[0].fs_size);
		}
		return rc;
	}
#endif

	return BOOT_HOOK_REGULAR;
}

int boot_read_swap_state_primary_slot_hook(int image_index,
		struct boot_swap_state *state)
{
	if (image_index == 1) {
		/* Populate with fake data */
		state->magic = BOOT_MAGIC_UNSET;
		state->swap_type = BOOT_SWAP_TYPE_NONE;
		state->image_num = image_index;
		state->copy_done = BOOT_FLAG_UNSET;
		state->image_ok = BOOT_FLAG_UNSET;

		/*
		 * Skip more handling of the primary slot for Image 1 as the slot
		 * exsists in RAM and is empty.
		 */
		return 0;
	}

	return BOOT_HOOK_REGULAR;
}

int network_core_update(bool wait)
{

	struct image_header *hdr;
#ifdef CONFIG_FLASH_SIMULATOR
	static const struct device *mock_flash_dev;
	void *mock_flash;
	size_t mock_size;

	mock_flash_dev = device_get_binding(PM_MCUBOOT_PRIMARY_1_DEV_NAME);
	if (mock_flash_dev == NULL) {
		return -ENODEV;
	}

	mock_flash = flash_simulator_get_memory(NULL, &mock_size);
#endif

	hdr = (struct image_header *)mock_flash;
	if (hdr->ih_magic == IMAGE_MAGIC) {
		uint32_t fw_size = hdr->ih_img_size;
		uint32_t vtable_addr = (uint32_t)hdr + hdr->ih_hdr_size;
		uint32_t *vtable = (uint32_t *)(vtable_addr);
		uint32_t reset_addr = vtable[1];

		if (reset_addr > PM_CPUNET_B0N_ADDRESS) {
			if (wait) {
				return pcd_network_core_update(vtable, fw_size);
			} else {
				return pcd_network_core_update_initiate(vtable, fw_size);
			}
		}
	}

	/* No IMAGE_MAGIC no valid image */
	return -ENODATA;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area,
		size_t size)
{

#ifdef CONFIG_FLASH_SIMULATOR
	if (img_index == NET_CORE_SECONDARY_SLOT) {
		return network_core_update(true);
	}
#endif

	return 0;
}

int boot_serial_uploaded_hook(int img_index, const struct flash_area *area,
		size_t size)
{
	if (img_index == NET_CORE_VIRTUAL_PRIMARY_SLOT) {
		return network_core_update(false);
	}

	return 0;
}
