/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>
#include <zephyr/devicetree.h>
#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw/sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */
#include <suit_plat_mem_util.h>

LOG_MODULE_REGISTER(dfu_target_suit, CONFIG_DFU_TARGET_LOG_LEVEL);

#define IS_ALIGNED_32(POINTER) (((uintptr_t)(const void *)(POINTER)) % 4 == 0)

#define DFU_PARTITION_LABEL   dfu_partition
#define DFU_PARTITION_OFFSET  FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE  FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_ADDRESS suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_SIZE    FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)

static uint8_t *stream_buf;
static size_t stream_buf_len;
static size_t stream_buf_bytes;
static size_t image_size;

bool dfu_target_suit_identify(const void *const buf)
{
	/* Not implemented */
	return -ENOSYS;
}

int dfu_target_suit_set_buf(uint8_t *buf, size_t len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	if (!IS_ALIGNED_32(buf)) {
		return -EINVAL;
	}

	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_suit_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	const struct device *flash_dev;
	int err;

	stream_buf_bytes = 0;
	image_size = 0;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	if (file_size > DFU_PARTITION_SIZE) {
		LOG_ERR("Requested file too big to fit in flash %zu > 0x%x", file_size,
			DFU_PARTITION_SIZE);
		return -EFBIG;
	}

	flash_dev = DFU_PARTITION_DEVICE;
	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Failed to get device for suit storage");
		return -EFAULT;
	}

	err = dfu_target_stream_init(
		&(struct dfu_target_stream_init){.id = "suit_dfu",
						 .fdev = flash_dev,
						 .buf = stream_buf,
						 .len = stream_buf_len,
						 .offset = (uintptr_t)DFU_PARTITION_OFFSET,
						 .size = DFU_PARTITION_SIZE,
						 .cb = NULL});
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	return 0;
}

int dfu_target_suit_offset_get(size_t *out)
{
	int err = 0;

	err = dfu_target_stream_offset_get(out);
	if (err == 0) {
		*out += stream_buf_bytes;
	}

	return err;
}

int dfu_target_suit_write(const void *const buf, size_t len)
{
	stream_buf_bytes = (stream_buf_bytes + len) % stream_buf_len;

	return dfu_target_stream_write(buf, len);
}

int dfu_target_suit_done(bool successful)
{
	int err = 0;

	dfu_target_suit_offset_get(&image_size);
	err = dfu_target_stream_done(successful);
	if (err != 0) {
		LOG_ERR("dfu_target_stream_done error %d", err);
		return err;
	}

	if (successful) {
		stream_buf_bytes = 0;
	} else {
		LOG_INF("SUIT envelope upgrade aborted.");
	}

	return err;
}

int dfu_target_suit_schedule_update(int img_num)
{
	LOG_INF("Schedule update");

	suit_plat_mreg_t update_candidate[] = {
		{
			.mem = DFU_PARTITION_ADDRESS,
			.size = image_size,
		},
	};

	suit_ssf_err_t err = suit_trigger_update(update_candidate, ARRAY_SIZE(update_candidate));

	if (err != SUIT_PLAT_SUCCESS) {
		return -EIO;
	}

	return 0;
}

int dfu_target_suit_reset(void)
{
	stream_buf_bytes = 0;
	return dfu_target_stream_reset();
}
