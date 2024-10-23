/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>

#include "suitfu_mgmt_priv.h"

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>

#include <dfu/suit_dfu.h>
#include <suit_plat_err.h>

LOG_MODULE_REGISTER(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

#define SYSTEM_UPDATE_WORKER_STACK_SIZE 2048

K_THREAD_STACK_DEFINE(system_update_stack_area, SYSTEM_UPDATE_WORKER_STACK_SIZE);

struct system_update_work {
	struct k_work_delayable work;
	/* Other data to pass to the workqueue might go here */
};

struct k_work_q system_update_work_queue;

static void update_failure(void)
{
	if (suit_dfu_cleanup() < 0) {
		LOG_ERR("Error while cleaning up the SUIT DFU module!");
	}
}

static void schedule_system_update(struct k_work *item)
{
	int ret = suit_dfu_candidate_preprocess();

	if (ret < 0) {
		LOG_ERR("Envelope processing error");
		update_failure();
		return;
	}
	k_msleep(CONFIG_MGMT_SUITFU_TRIGGER_UPDATE_RESET_DELAY_MS);

	ret = suit_dfu_update_start();
	if (ret < 0) {
		LOG_ERR("Failed to start firmware upgrade!");
		update_failure();
		return;
	}
}

int suitfu_mgmt_cleanup(void)
{
	if (suit_dfu_cleanup() < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return MGMT_ERR_EOK;
}

int suitfu_mgmt_candidate_envelope_stored(void)
{
	int ret = suit_dfu_candidate_envelope_stored();

	if (ret < 0) {
		LOG_ERR("Envelope decoding error");
		update_failure();
		return MGMT_ERR_EBUSY;
	}

	return MGMT_ERR_EOK;
}

int suitfu_mgmt_candidate_envelope_process(void)
{
	static struct system_update_work suw;

	LOG_INF("Schedule system reboot");
	k_work_init_delayable(&suw.work, schedule_system_update);

	int ret = k_work_schedule_for_queue(&system_update_work_queue, &suw.work, K_NO_WAIT);

	if (ret < 0) {
		LOG_ERR("Unable to process the envelope");
		update_failure();
		return MGMT_ERR_EBUSY;
	}

	return MGMT_ERR_EOK;
}

int suitfu_mgmt_erase(struct suit_nvm_device_info *device_info, size_t num_bytes)
{
	if (device_info == NULL || device_info->fdev == NULL) {
		return MGMT_ERR_EINVAL;
	}

	size_t erase_size = DIV_ROUND_UP(num_bytes, device_info->erase_block_size) *
			    device_info->erase_block_size;

	if (erase_size > device_info->partition_size) {
		return MGMT_ERR_ENOMEM;
	}

	LOG_INF("Erasing, addr: %p, size %d bytes", (void *)device_info->mapped_address,
		erase_size);

	int rc = flash_erase(device_info->fdev, device_info->partition_offset, erase_size);

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return MGMT_ERR_EOK;
}

int suitfu_mgmt_write(struct suit_nvm_device_info *device_info, size_t req_img_offset,
		      const uint8_t *chunk, size_t chunk_size, bool flush)
{
	static const struct device *fdev;
	static size_t write_block_size;
	static uint8_t write_buf[16];
	static uint8_t buf_fill_level;
	static size_t offset;

	int rc;

	if (device_info == NULL || device_info->fdev == NULL || chunk == NULL) {
		LOG_ERR("Wrong parameters");
		return MGMT_ERR_EINVAL;
	}

	/* Allow to reset the write procedure if the offset is equal to zero. */
	if (req_img_offset == 0) {

		if (device_info->write_block_size > sizeof(write_buf)) {
			LOG_ERR("Device not supported, write_block_size: %d bytes",
				device_info->write_block_size);
			return MGMT_ERR_EUNKNOWN;
		}

		fdev = device_info->fdev;
		write_block_size = device_info->write_block_size;
		buf_fill_level = 0;
		offset = 0;
	}

	if (device_info->fdev != fdev) {
		LOG_ERR("Write request corrupted, writing to other device in progress");
		return MGMT_ERR_EUNKNOWN;
	}

	/* Check if cache is continuous with the input buffer. */
	if ((offset + buf_fill_level) != req_img_offset) {
		LOG_ERR("Write request corrupted, last offset: %d, requested "
			"offset: %d",
			offset + buf_fill_level, req_img_offset);
		return MGMT_ERR_EUNKNOWN;
	}

	LOG_DBG("Writing %d bytes (buffer fill level: %d)", chunk_size, buf_fill_level);

	/* Fill the write buffer to flush non-aligned bytes from the previous
	 * call.
	 */
	if (buf_fill_level) {
		size_t len = write_block_size - buf_fill_level;

		len = MIN(len, chunk_size);
		memcpy(&write_buf[buf_fill_level], chunk, len);

		buf_fill_level += len;

		/* If write buffer is full - write it into the memory. */
		if (buf_fill_level == write_block_size) {
			LOG_DBG("Writing %d buffered bytes to addr: %p", write_block_size,
				(void *)(device_info->mapped_address + offset));

			rc = flash_write(fdev, device_info->partition_offset + offset, write_buf,
					 write_block_size);

			if (rc < 0) {
				LOG_ERR("Writing to addr: %p failed",
					(void *)(device_info->mapped_address + offset));
				return MGMT_ERR_EUNKNOWN;
			}

			buf_fill_level = 0;
			offset += write_block_size;
		}

		chunk += len;
		chunk_size -= len;
	}

	/* Write aligned data directly from input buffer. */
	if (chunk_size >= write_block_size) {
		size_t len = ((chunk_size / write_block_size) * write_block_size);

		LOG_DBG("Writing continuous %d image bytes to addr: %p", len,
			(void *)(device_info->mapped_address + offset));

		rc = flash_write(fdev, device_info->partition_offset + offset, chunk, len);
		if (rc < 0) {
			LOG_ERR("Writing to addr: %p failed",
				(void *)(device_info->mapped_address + offset));
			return MGMT_ERR_EUNKNOWN;
		}

		chunk += len;
		chunk_size -= len;
		offset += len;
	}

	/* Store remaining bytes into the write buffer. */
	if (chunk_size > 0) {
		LOG_DBG("Caching remaining %d image bytes", chunk_size);
		memcpy(&write_buf[0], chunk, chunk_size);
		buf_fill_level += chunk_size;
	}

	/* Flush buffer if requested. */
	if (flush && buf_fill_level > 0) {
		/* Do not leak information about the previous requests. */
		memset(&write_buf[buf_fill_level], 0xFF, write_block_size - buf_fill_level);

		LOG_DBG("Flushing %d buffered bytes to addr: %p", write_block_size,
			(void *)(device_info->mapped_address + offset));

		rc = flash_write(fdev, device_info->partition_offset + offset, write_buf,
				 write_block_size);
		if (rc < 0) {
			LOG_ERR("Writing to addr: %p failed",
				(void *)(device_info->mapped_address + offset));
			return MGMT_ERR_EUNKNOWN;
		}

		buf_fill_level = 0;
		offset += write_block_size;
	}

	return MGMT_ERR_EOK;
}

int suitfu_mgmt_init(void)
{
	k_work_queue_init(&system_update_work_queue);
	k_work_queue_start(&system_update_work_queue, system_update_stack_area,
			   K_THREAD_STACK_SIZEOF(system_update_stack_area), K_HIGHEST_THREAD_PRIO,
			   NULL);
#ifdef CONFIG_MGMT_SUITFU_GRP_OS_BOOTLOADER_INFO_HOOK
	suitfu_mgmt_register_bootloader_info_hook();
#endif /* CONFIG_MGMT_SUITFU_GRP_OS_BOOTLOADER_INFO_HOOK */
#ifdef CONFIG_MGMT_SUITFU_INITIALIZE_SUIT
	return suit_dfu_initialize();
#else
	return 0;
#endif /* CONFIG_MGMT_SUITFU_INITIALIZE_SUIT */
}

SYS_INIT(suitfu_mgmt_init, APPLICATION, 0);
