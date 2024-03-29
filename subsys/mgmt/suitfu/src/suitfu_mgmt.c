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
#include <suit_plat_mem_util.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>

#include <dfu/suit_dfu.h>

LOG_MODULE_REGISTER(suitfu_mgmt, CONFIG_MGMT_SUITFU_LOG_LEVEL);

#if (!(DT_NODE_EXISTS(DT_NODELABEL(dfu_partition))))
#error DFU Partition not defined in devicetree
#endif

#define FIXED_PARTITION_ERASE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), erase_block_size)
#define FIXED_PARTITION_WRITE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), write_block_size)

#define DFU_PARTITION_LABEL	 dfu_partition
#define DFU_PARTITION_ADDRESS	 suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_OFFSET	 FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_SIZE	 FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_EB_SIZE	 FIXED_PARTITION_ERASE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_WRITE_SIZE FIXED_PARTITION_WRITE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE	 FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)

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

int suitfu_mgmt_candidate_envelope_stored(size_t image_size)
{
	int rc = MGMT_ERR_EOK;
	int ret = 0;

	ret = suit_dfu_candidate_envelope_stored();
	if (ret < 0) {
		LOG_ERR("Envelope decoding error");
		update_failure();
		return MGMT_ERR_EBUSY;
	}

	static struct system_update_work suw;

	LOG_INF("Schedule system reboot");
	k_work_init_delayable(&suw.work, schedule_system_update);
	ret = k_work_schedule_for_queue(&system_update_work_queue, &suw.work,
					K_MSEC(CONFIG_MGMT_SUITFU_TRIGGER_UPDATE_RESET_DELAY_MS));
	if (ret < 0) {
		LOG_ERR("Unable to process the envelope");
		update_failure();
		rc = MGMT_ERR_EBUSY;
	}

	return rc;
}

int suitfu_mgmt_is_dfu_partition_ready(void)
{
	const struct device *fdev = DFU_PARTITION_DEVICE;

	if (!device_is_ready(fdev)) {
		return MGMT_ERR_EBADSTATE;
	}

	return MGMT_ERR_EOK;
}

size_t suitfu_mgmt_get_dfu_partition_size(void)
{
	return DFU_PARTITION_SIZE;
}

int suitfu_mgmt_erase_dfu_partition(size_t num_bytes)
{
	const struct device *fdev = DFU_PARTITION_DEVICE;
	size_t erase_size = DIV_ROUND_UP(num_bytes, DFU_PARTITION_EB_SIZE) * DFU_PARTITION_EB_SIZE;

	if (erase_size > DFU_PARTITION_SIZE) {
		return MGMT_ERR_ENOMEM;
	}

	LOG_INF("Erasing %p - %p (%d bytes)", (void *)DFU_PARTITION_ADDRESS,
		(void *)((size_t)DFU_PARTITION_ADDRESS + erase_size), erase_size);

	int rc = flash_erase(fdev, DFU_PARTITION_OFFSET, erase_size);

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return MGMT_ERR_EOK;
}

int suitfu_mgmt_write_dfu_image_data(unsigned int req_offset, const void *addr, unsigned int size,
				     bool flush)
{
	const struct device *fdev = DFU_PARTITION_DEVICE;
	static uint8_t write_buf[DFU_PARTITION_WRITE_SIZE];
	static uint8_t buf_fill_level;
	static size_t offset;
	int err = 0;

	/* Allow to reset the write procedure if the offset is equal to zero. */
	if (req_offset == 0) {
		buf_fill_level = 0;
		offset = 0;
	}

	LOG_DBG("Writing %d bytes (cache fill: %d)", size, buf_fill_level);

	/* Check if cache is continuous with the input buffer. */
	if ((offset + buf_fill_level) != req_offset) {
		LOG_ERR("Write request corrupted. Last offset: %p requested "
			"offset: %p",
			(void *)(DFU_PARTITION_OFFSET + offset + buf_fill_level),
			(void *)(DFU_PARTITION_OFFSET + req_offset));
		return MGMT_ERR_EUNKNOWN;
	}

	/* Fill the write buffer to flush non-aligned bytes from the previous
	 * call.
	 */
	if (buf_fill_level) {
		size_t len = sizeof(write_buf) - buf_fill_level;

		len = MIN(len, size);
		memcpy(&write_buf[buf_fill_level], addr, len);

		buf_fill_level += len;
		addr = ((uint8_t *)addr) + len;
		size -= len;

		/* If write buffer is full - write it into the memory. */
		if (buf_fill_level == sizeof(write_buf)) {
			LOG_DBG("Write continuous %d cache bytes (address: %p)", sizeof(write_buf),
				(void *)(DFU_PARTITION_OFFSET + offset));
			err = flash_write(fdev, DFU_PARTITION_OFFSET + offset, write_buf,
					  sizeof(write_buf));

			buf_fill_level = 0;
			offset += sizeof(write_buf);
		}
	}

	/* Write aligned data directly from input buffer. */
	if ((err == 0) && (size >= sizeof(write_buf))) {
		size_t write_size = ((size / sizeof(write_buf)) * sizeof(write_buf));

		LOG_DBG("Write continuous %d image bytes (address: %p)", write_size,
			(void *)(DFU_PARTITION_OFFSET + offset));
		err = flash_write(fdev, DFU_PARTITION_OFFSET + offset, addr, write_size);

		size -= write_size;
		offset += write_size;
		addr = ((uint8_t *)addr + write_size);
	}

	LOG_DBG("Cache %d bytes (address: %p)", size, (void *)(DFU_PARTITION_OFFSET + offset));

	/* Store remaining bytes into the write buffer. */
	if ((err == 0) && (size > 0)) {
		memcpy(&write_buf[0], addr, size);
		buf_fill_level += size;
	}

	/* Flush buffer if requested. */
	if ((err == 0) && (flush) && (buf_fill_level > 0)) {
		/* Do not leak information about the previous requests. */
		memset(&write_buf[buf_fill_level], 0xFF, sizeof(write_buf) - buf_fill_level);

		LOG_DBG("Flush %d bytes (address: %p)", sizeof(write_buf),
			(void *)(DFU_PARTITION_OFFSET + offset));
		err = flash_write(fdev, DFU_PARTITION_OFFSET + offset, write_buf,
				  sizeof(write_buf));

		buf_fill_level = 0;
		offset += sizeof(write_buf);
	}

	if (flush) {
		const uint8_t *addr = DFU_PARTITION_ADDRESS;

		LOG_INF("Last Chunk Written");
		LOG_INF("Partition address: %p (size: 0x%x), data: %02X %02X "
			"%02X "
			"%02X ...",
			(void *)addr, offset, addr[0], addr[1], addr[2], addr[3]);
	}

	return (err == 0 ? MGMT_ERR_EOK : MGMT_ERR_EUNKNOWN);
}

int suitfu_mgmt_init(void)
{
	k_work_queue_init(&system_update_work_queue);
	k_work_queue_start(&system_update_work_queue, system_update_stack_area,
			   K_THREAD_STACK_SIZEOF(system_update_stack_area), K_HIGHEST_THREAD_PRIO,
			   NULL);
	return suit_dfu_initialize();
}

SYS_INIT(suitfu_mgmt_init, APPLICATION, 0);
