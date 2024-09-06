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
#include <suit_envelope_info.h>
#ifdef CONFIG_DFU_TARGET_SUIT_CACHE_PROCESSING
#include <suit_dfu_cache_rw.h>
#endif
#include <dfu/suit_dfu.h>

LOG_MODULE_REGISTER(dfu_target_suit, CONFIG_DFU_TARGET_LOG_LEVEL);

#define IS_ALIGNED_32(POINTER) (((uintptr_t)(const void *)(POINTER)) % 4 == 0)

#define ENVELOPE_IMAGE_NUMBER 0

static uint8_t *stream_buf;
static size_t stream_buf_len;
static size_t stream_buf_bytes;
static size_t image_size;
static size_t image_num;
static bool stream_flash_in_use;

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
	static struct suit_nvm_device_info device_info;
	int err = 0;

	stream_buf_bytes = 0;
	image_size = 0;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	if (ENVELOPE_IMAGE_NUMBER == img_num) {
		/* Get info about dfu_partition to store the envelope */
		err = suit_dfu_partition_device_info_get(&device_info);
	} else {
#ifdef CONFIG_DFU_TARGET_SUIT_CACHE_PROCESSING
		/* Cache partitions ids starts from 0, whereas image number 0 is reserved for
		 * dfu_partition. Decrease img_num value by 1 to reach the correct DFU cache
		 * partition id.
		 */
		err = suit_dfu_cache_rw_device_info_get(img_num - 1, &device_info);
#else
		return -ENOTSUP;
#endif
	}

	if (err != SUIT_PLAT_SUCCESS) {
		return -ENXIO;
	}

	if (file_size > device_info.partition_size) {
		LOG_ERR("Requested file for image %d too big to fit in flash %zu > %zu", img_num,
			file_size, device_info.partition_size);
		return -EFBIG;
	}

	flash_dev = device_info.fdev;
	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Failed to get device for suit storage");
		return -EFAULT;
	}

	if (stream_flash_in_use) {
		LOG_ERR("Stream flash is already in use");
		return -EBUSY;
	}

	err = dfu_target_stream_init(
		&(struct dfu_target_stream_init){.id = "suit_dfu",
						 .fdev = flash_dev,
						 .buf = stream_buf,
						 .len = stream_buf_len,
						 .offset = (uintptr_t)device_info.partition_offset,
						 .size = device_info.partition_size,
						 .cb = NULL});
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		stream_flash_in_use = false;
		return err;
	}

	stream_flash_in_use = true;
	image_num = img_num;

	return 0;
}

int dfu_target_suit_offset_get(size_t *out)
{
	if (0 != dfu_target_stream_offset_get(out)) {
		return -EFAULT;
	}

	*out += stream_buf_bytes;

	return 0;
}

int dfu_target_suit_write(const void *const buf, size_t len)
{
	if (!stream_flash_in_use) {
		LOG_ERR("Stream flash is not in use");
		return -EFAULT;
	}

	stream_buf_bytes = (stream_buf_bytes + len) % stream_buf_len;

	return dfu_target_stream_write(buf, len);
}

int dfu_target_suit_done(bool successful)
{
	stream_flash_in_use = false;

	int err = dfu_target_stream_done(successful);
	if (err != 0) {
		LOG_ERR("dfu_target_stream_done error %d", err);
		return err;
	}

	if (ENVELOPE_IMAGE_NUMBER == image_num) {

		/* Inform SUIT that envelope has been received */
		err = suit_dfu_candidate_envelope_stored();
		if (err != 0) {
			LOG_ERR("suit_dfu_candidate_envelope_stored error %d", err);
			return err;
		}
	}

	if (successful) {
		stream_buf_bytes = 0;
	} else {
		LOG_INF("SUIT envelope upgrade aborted.");
	}

	return 0;
}

int dfu_target_suit_schedule_update(int img_num)
{
	/* img_num is not needed for SUIT */
	(void)img_num;

	LOG_INF("Schedule update");

	/* Process the saved envelope */
	int err = suit_dfu_candidate_preprocess();

	if (err != 0) {
		LOG_ERR("suit_dfu_candidate_preprocess error %d", err);
		return err;
	}

	return 0;
}

int dfu_target_suit_reset(void)
{
	int rc = dfu_target_stream_reset();

	if (rc != 0) {
		LOG_ERR("dfu_target_stream_reset error %d", rc);
		return rc;
	}

	suit_ssf_err_t err = suit_dfu_cleanup();

	stream_flash_in_use = false;
	stream_buf_bytes = 0;
	image_num = 0;

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to cleanup SUIT DFU");
		rc = -EIO;
	}

	return 0;
}

int dfu_target_suit_reboot(void)
{
	k_msleep(CONFIG_DFU_TARGET_REBOOT_RESET_DELAY_MS);

	suit_ssf_err_t err = suit_dfu_update_start();

	if (err < 0) {
		LOG_ERR("Failed to start firmware upgrade!");
		return err;
	}

	return 0;
}

#ifdef CONFIG_DFU_TARGET_SUIT_INITIALIZE_SUIT
int dfu_target_suit_init_suit(void)
{
	return suit_dfu_initialize();
}

SYS_INIT(dfu_target_suit_init_suit, APPLICATION, 0);
#endif /* CONFIG_DFU_TARGET_SUIT_INITIALIZE_SUIT */
