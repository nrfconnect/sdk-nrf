/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#if CONFIG_INSTALLER_REBOOT
#include <zephyr/sys/reboot.h>
#endif
#if CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_MODE
#include <zephyr/retention/bootmode.h>
#endif

#if CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_REQ
#include <bootutil/boot_request.h>
#endif
#include <bootutil/bootutil_public.h>

LOG_MODULE_REGISTER(installer, CONFIG_INSTALLER_LOG_LEVEL);

extern uintptr_t _flash_used;

#define INSTALLER_SLOT DT_NODELABEL(slot0_partition)
#define INSTALLER_SLOT_ADDRESS FIXED_PARTITION_NODE_ADDRESS(INSTALLER_SLOT)
#define INSTALLER_SLOT_SIZE FIXED_PARTITION_NODE_SIZE(INSTALLER_SLOT)
#define INSTALLER_SLOT_ID DT_FIXED_PARTITION_ID(INSTALLER_SLOT)

#define INSTALLER_CODE_PARTITION DT_CHOSEN(zephyr_code_partition)
#define INSTALLER_CODE_PARTITION_ADDRESS FIXED_PARTITION_NODE_ADDRESS(INSTALLER_CODE_PARTITION)

/**
 * Offset from the start of the slot the installer resides to the actual image flash start.
 * In other words: offset from which _flash_used symbol starts measuring the used flash.
 * Note: this will be 0 if slot0_partition matches cpuapp_slot0_partition/zephyr,code-partition.
 * In some cases the slot0_partition points to the start of the MCUBoot header,
 * while the zephyr,code-partition points to the start of image itself.
 * In these cases this offset will be non-zero.
 */
#define INSTALLER_FLASH_USED_OFFSET (INSTALLER_CODE_PARTITION_ADDRESS - INSTALLER_SLOT_ADDRESS)

#define FW_LOADER_SLOT DT_NODELABEL(slot1_partition)
#define FW_LOADER_SLOT_ID DT_FIXED_PARTITION_ID(FW_LOADER_SLOT)
#define FW_LOADER_SLOT_SIZE FIXED_PARTITION_NODE_SIZE(FW_LOADER_SLOT)

#define BUFFER_SIZE 4096
#define ERASE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)
#define WRITE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), write_block_size)

#define ERASE_VALUE 0xff

uint8_t buffer_source[BUFFER_SIZE];
uint8_t buffer_target[BUFFER_SIZE];


#if CONFIG_INSTALLER_FW_LOADER_ENTRANCE_SELF_INVALIDATE_BY_ERASE
static int self_invalidate_by_erase(const struct flash_area *source_fa)
{
	int rc = flash_area_erase(source_fa, 0, ERASE_BLOCK_SIZE);

	if (rc < 0) {
		LOG_ERR("Failed to invalidate installer");
	}

	return rc;
}
#endif

static int request_fw_loader_entrance(const struct flash_area *source_fa)
{
	int rc = 0;

#if CONFIG_INSTALLER_FW_LOADER_ENTRANCE_SELF_INVALIDATE_BY_ERASE
	rc = self_invalidate_by_erase(source_fa);
#endif
#if CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_MODE
	(void) source_fa;
	rc = bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
#elif CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_REQ
	(void) source_fa;
	rc = boot_request_enter_firmware_loader();
#endif
	if (rc != 0) {
		LOG_ERR("Failed to request firmware loader entrance");
	}

#if CONFIG_INSTALLER_REBOOT
	if (rc == 0) {
		/* Do not reboot if requesting firmware loader entrance fails,
		 * otherwise the device may be stuck in a boot loop.
		 */
		sys_reboot(SYS_REBOOT_WARM);
	}
#endif

	return rc;
}

static int new_fw_loader_data_info_get(const struct flash_area *source_fa,
				       uint32_t *fw_loader_data_start_offset,
				       size_t *fw_loader_total_size)
{

	int rc;
	struct image_header combined_image_header = {0};

	rc = flash_area_read(source_fa, 0, &combined_image_header, sizeof(struct image_header));

	if (rc < 0) {
		LOG_ERR("Failed to read flash area");
		LOG_DBG("Failed to read installer image header: %d", rc);
		return rc;
	}

	if (combined_image_header.ih_magic != IMAGE_MAGIC) {
		LOG_ERR("The installer image is not valid");
		return -EINVAL;
	}

	/* The updated firmware loader image is appended directly after the installer image.
	 *
	 * Note, that the installer image is linked starting at the address pointed
	 * by the zephyr,code-partition chosen node, while the source_fa flash area
	 * represents the slot0_partition node, which starts with the MCUBoot header.
	 * These two areas do not necessarily match, hence the offset.
	 */
	*fw_loader_data_start_offset = (uint32_t)&_flash_used + INSTALLER_FLASH_USED_OFFSET;

	uint32_t off = *fw_loader_data_start_offset;
	struct image_header fw_loader_header = {0};

	rc = flash_area_read(source_fa, off, &fw_loader_header, sizeof(struct image_header));
	if (rc < 0) {
		LOG_ERR("Failed to read flash area");
		LOG_DBG("Failed to read firmware loader image header: %d", rc);
		return rc;
	}

	if (fw_loader_header.ih_magic != IMAGE_MAGIC) {
		LOG_ERR("The firmware loader image is not valid");
		return -EINVAL;
	}

	LOG_DBG("Found new firmware loader image at offset: 0x%x", off);

	/**
	 * The MCUBoot image structure (excluding the trailer, which irrelavant for this use case)
	 * is as follows:
	 *
	 * |--------------------------------|
	 * | Header                         |
	 * |--------------------------------|
	 * | Image                          |
	 * |--------------------------------|
	 * | Protected TLVs                 |
	 * |--------------------------------|
	 * | Non-protected TLVs             |
	 * |--------------------------------|
	 *
	 * The sizes of the header, image, protected TLV areas are stored in the header.
	 * The non-protected TLV area is not stored immediately after the protected TLV area,
	 * and starts with a TLV info structure containing its size.
	 */
	off += fw_loader_header.ih_hdr_size + fw_loader_header.ih_img_size
	       + fw_loader_header.ih_protect_tlv_size;

	struct image_tlv_info fw_loader_tlv_info = {0};

	rc = flash_area_read(source_fa, off, &fw_loader_tlv_info, sizeof(fw_loader_tlv_info));
	if (rc < 0) {
		LOG_ERR("Failed to read flash area");
		LOG_DBG("Failed to read firmware loader image TLV info: %d", rc);
		return rc;
	}

	if (fw_loader_tlv_info.it_magic != IMAGE_TLV_INFO_MAGIC) {
		LOG_ERR("The firmware loader image TLV info is not valid");
		return -EINVAL;
	}

	off += fw_loader_tlv_info.it_tlv_tot;
	*fw_loader_total_size = fw_loader_header.ih_hdr_size
				+ fw_loader_header.ih_img_size
				+ fw_loader_header.ih_protect_tlv_size
				+ fw_loader_tlv_info.it_tlv_tot;

	LOG_DBG("Firmware loader total size: 0x%x", *fw_loader_total_size);

	if (off > INSTALLER_SLOT_SIZE) {
		LOG_ERR("New firmware loader data too large");
		LOG_DBG("The new firmware loader image data exceeds the installer code partition");
		return -EPERM;
	}

	const size_t installer_package_no_tlv_real_size = *fw_loader_total_size
							+ (size_t)&_flash_used
							+ INSTALLER_FLASH_USED_OFFSET;
	const size_t installer_package_no_tlv_size_from_header = combined_image_header.ih_hdr_size
							       + combined_image_header.ih_img_size;
	if (installer_package_no_tlv_real_size >
	    installer_package_no_tlv_size_from_header) {
		LOG_ERR("New firmware loader data too large");
		LOG_DBG("The combined installer and firmware loader image data exceeds "
			"the combined image size calculated from the header");
		return -EPERM;
	}

	if (ROUND_UP(*fw_loader_total_size, WRITE_BLOCK_SIZE) > FW_LOADER_SLOT_SIZE) {
		LOG_ERR("New firmware loader data too large");
		LOG_DBG("The firmware loader image total size exceeds "
			"the firmware loader code partition size");
		return -EPERM;
	}

	return 0;
}

static int copy_fw_loader(const struct flash_area *source_fa,
			  const struct flash_area *target_fa,
			  uint32_t fw_loader_data_start_offset,
			  size_t fw_loader_total_size)
{
	int rc;

	rc = flash_area_erase(target_fa, 0, flash_area_get_size(target_fa));

	/* No point of requesting firmware loader entrance on error after this point.
	 * If an error occurs, both the installer and the firmware loader are corrupted.
	 * Attempting to request firmware loader entrance could result in a boot loop.
	 */
	if (rc < 0) {
		LOG_ERR("Failed to clear flash area");
		return rc;
	}

	/* Some of the devices might need writes to be aligned to WRITE_BLOCK_SIZE.
	 * We cannot simply align the copy size up,
	 * as the data beyond fw_loader_total_size is not verified.
	 * Instead, we copy the data up to the aligned length
	 * and then write a buffer with the remaining data and pad it
	 * with the erase value up to WRITE_BLOCK_SIZE.
	 */
	const size_t aligned_len = ROUND_DOWN(fw_loader_total_size, WRITE_BLOCK_SIZE);
	const size_t tail_len = fw_loader_total_size - aligned_len;

	rc = flash_area_copy(source_fa, fw_loader_data_start_offset, target_fa, 0,
				 aligned_len, buffer_source, BUFFER_SIZE);
	if (rc < 0) {
		LOG_ERR("Failed to copy firmware loader data");
		return rc;
	}

	if (tail_len > 0) {
		const uint8_t erase_val = flash_area_erased_val(target_fa);

		memset(buffer_source, erase_val, WRITE_BLOCK_SIZE);
		rc = flash_area_read(source_fa, fw_loader_data_start_offset + aligned_len,
				     buffer_source, tail_len);
		if (rc < 0) {
			LOG_ERR("Failed to copy firmware loader data");
			return rc;
		}

		rc = flash_area_write(target_fa, aligned_len, buffer_source, WRITE_BLOCK_SIZE);
		if (rc < 0) {
			LOG_ERR("Failed to copy firmware loader data");
			return rc;
		}
	}

	return 0;
}

static int compare_fw_loader_slots(const struct flash_area *source_fa,
				   const struct flash_area *target_fa,
				   uint32_t fw_loader_data_start_offset,
				   size_t fw_loader_total_size)
{
	int rc;
	size_t bytes_left = fw_loader_total_size;
	uint32_t source_offset = fw_loader_data_start_offset;
	uint32_t target_offset = 0;

	while (bytes_left > BUFFER_SIZE) {
		rc = flash_area_read(source_fa, source_offset, buffer_source, BUFFER_SIZE);
		if (rc < 0) {
			LOG_ERR("Failed to read flash area");
			return rc;
		}
		rc = flash_area_read(target_fa, target_offset, buffer_target, BUFFER_SIZE);
		if (rc < 0) {
			LOG_ERR("Failed to read flash area");
			return rc;
		}

		if (memcmp(buffer_source, buffer_target, BUFFER_SIZE) != 0) {
			return 1;
		}
		source_offset += BUFFER_SIZE;
		target_offset += BUFFER_SIZE;
		bytes_left -= BUFFER_SIZE;
	}

	rc = flash_area_read(source_fa, source_offset, buffer_source, bytes_left);
	if (rc < 0) {
		LOG_ERR("Failed to read flash area");
		return rc;
	}
	rc = flash_area_read(target_fa, target_offset, buffer_target, bytes_left);
	if (rc < 0) {
		LOG_ERR("Failed to read flash area");
		return rc;
	}
	if (memcmp(buffer_source, buffer_target, bytes_left) != 0) {
		return 1;
	}

	return 0;
}

int main(void)
{
	int rc;
	const struct flash_area *source_fa;
	const struct flash_area *target_fa;
	uint32_t fw_loader_data_start_offset;
	size_t fw_loader_total_size;

	LOG_INF("Installer started");

	rc = flash_area_open(INSTALLER_SLOT_ID, &source_fa);

	if (rc < 0) {
		LOG_ERR("Failed to open flash area");
		request_fw_loader_entrance(source_fa);
		return rc;
	}

	rc = new_fw_loader_data_info_get(source_fa, &fw_loader_data_start_offset,
					 &fw_loader_total_size);
	if (rc < 0) {
		LOG_ERR("Failed to get new firmware loader data info");
		request_fw_loader_entrance(source_fa);
		return rc;
	}

	rc = flash_area_open(FW_LOADER_SLOT_ID, &target_fa);
	if (rc < 0) {
		LOG_ERR("Failed to open flash area");
		LOG_DBG("Failed to open firmware loader code partition area: %d", rc);
		request_fw_loader_entrance(source_fa);
		return rc;
	}

	rc = compare_fw_loader_slots(source_fa, target_fa, fw_loader_data_start_offset,
				     fw_loader_total_size);
	if (rc == 0) {
		LOG_INF("Firmware loader images are the same, nothing to do");
		request_fw_loader_entrance(source_fa);
		return 0;
	}

	if (rc < 0) {
		LOG_ERR("Failed to compare firmware loader slots");
		request_fw_loader_entrance(source_fa);
		return rc;
	}

	rc = copy_fw_loader(source_fa, target_fa, fw_loader_data_start_offset,
			    fw_loader_total_size);

	/* No point of requesting firmware loader entrance on error after this point.
	 * If an error occurs, both the installer and the firmware loader are corrupted.
	 * Attempting to request firmware loader entrance could result in a boot loop.
	 */
	if (rc < 0) {
		LOG_ERR("Failed to copy firmware loader data");
		return rc;
	}

	rc = compare_fw_loader_slots(source_fa, target_fa, fw_loader_data_start_offset,
				     fw_loader_total_size);
	if (rc < 0) {
		LOG_ERR("Failed to verify copied firmware loader data");
		return rc;
	} else if (rc > 0) {
		LOG_ERR("Firmware loader data mismatch after installation");
		return rc;
	}

	LOG_INF("Firmware loader data copied and verified");

	request_fw_loader_entrance(source_fa);

	return 0;
}
