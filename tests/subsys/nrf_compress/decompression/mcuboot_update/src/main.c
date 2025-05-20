/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/byteorder.h>

#include <bootutil/image.h>

LOG_MODULE_REGISTER(mcuboot_update, LOG_LEVEL_DBG);

#define UPDATE_SLOT_NUMBER CONFIG_UPDATE_SLOT_NUMBER

#if CONFIG_UPDATE_SLOT_NUMBER == 0
#define UPDATE_SLOT_ID PM_MCUBOOT_SECONDARY_ID
#else
#define UPDATE_SLOT_ID PM_MCUBOOT_SECONDARY_ ## CONFIG_UPDATE_SLOT_NUMBER ## _ID
#endif

int main(void)
{
	int rc;
	struct image_header header;
	const struct flash_area *fa;

	rc = flash_area_open(UPDATE_SLOT_ID, &fa);

	if (rc) {
		LOG_ERR("Flash area open failed");

		return 0;
	}

	rc = flash_area_read(fa, 0, &header, sizeof(header));

	flash_area_close(fa);

	if (rc) {
		LOG_ERR("Flash area read failed");

		return 0;
	}

	header.ih_magic = sys_le32_to_cpu(header.ih_magic);
	header.ih_hdr_size = sys_le16_to_cpu(header.ih_hdr_size);
	header.ih_flags = sys_le32_to_cpu(header.ih_flags);

	if (header.ih_magic != IMAGE_MAGIC) {
		LOG_ERR("Invalid header");

		return 0;
	}

	if ((header.ih_flags & IMAGE_F_COMPRESSED_LZMA2) == 0) {
		LOG_ERR("Invalid image flags");

		return 0;
	}

	rc = boot_request_upgrade_multi(UPDATE_SLOT_NUMBER, true);

	if (rc) {
		LOG_ERR("Update of image %d failed: %d", UPDATE_SLOT_NUMBER, rc);
		k_sleep(K_SECONDS(1));

		return 0;
	}

	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}
