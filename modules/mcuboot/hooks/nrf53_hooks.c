/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash/flash_simulator.h>
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/boot_hooks.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"

#define NET_CORE_SECONDARY_SLOT 1
#define NET_CORE_VIRTUAL_PRIMARY_SLOT 3

#include <dfu/pcd.h>

int boot_read_image_header_hook(int img_index, int slot,
		struct image_header *img_head)
{
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
	static const struct device *mock_flash_dev;
	void *mock_flash;
	size_t mock_size;

	mock_flash_dev = DEVICE_DT_GET(DT_NODELABEL(PM_MCUBOOT_PRIMARY_1_DEV));
	if (!device_is_ready(mock_flash_dev)) {
		return -ENODEV;
	}

	mock_flash = flash_simulator_get_memory(NULL, &mock_size);
	hdr = (struct image_header *) mock_flash;
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
	if (img_index == NET_CORE_SECONDARY_SLOT) {
		return network_core_update(true);
	}

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

int boot_reset_request_hook(bool force)
{
	ARG_UNUSED(force);

	if (pcd_fw_copy_status_get() == PCD_STATUS_COPY) {
		return BOOT_RESET_REQUEST_HOOK_BUSY;
	}
	return 0;
}
