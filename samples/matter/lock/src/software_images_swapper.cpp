/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "software_images_swapper.h"

#include <cstdlib>
#include <dfu/dfu_multi_image.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

int SoftwareImagesSwapper::Swap(const ImageLocation &source, SoftwareImagesSwapDoneCallback swapDoneCallback)
{
	int result;

	if (mSwapInProgress) {
		return -EALREADY;
	}

	if (!swapDoneCallback) {
		return -EINVAL;
	}

	uint8_t *dfuImageBuffer = (uint8_t *)malloc(kBufferSize);
	if (!dfuImageBuffer) {
		LOG_ERR("Cannot allocate application DFU image buffer");
		return -ENOMEM;
	}

	result = dfu_target_mcuboot_set_buf(dfuImageBuffer, kBufferSize);
	if (result != 0) {
		goto exit;
	}

	mSwapInProgress = true;

	result = SwapImage(source.app_address, source.app_size, 0);
	if (result != 0) {
		mSwapInProgress = false;
		goto exit;
	}

	result = SwapImage(source.net_address, source.net_size, 1);
	if (result != 0) {
		mSwapInProgress = false;
		goto exit;
	}

	result = dfu_target_schedule_update(-1);
	if (result != 0) {
		mSwapInProgress = false;
		goto exit;
	}

	swapDoneCallback();

exit:
	free(dfuImageBuffer);

	return result;
}

int SoftwareImagesSwapper::SwapImage(uint32_t address, uint32_t size, uint8_t id)
{
	int result;
	LOG_INF("Requested to swap image %d from address %x with size %x", id, address, size);

	if (!device_is_ready(mFlashDevice)) {
		LOG_ERR("Flash device is not ready");
		return -ENODEV;
	}

	uint8_t *flashImageBuffer = (uint8_t *)malloc(kBufferSize);
	if (!flashImageBuffer) {
		LOG_ERR("Cannot allocate application flash image buffer");
		return -ENOMEM;
	}

	result = dfu_target_init(DFU_TARGET_IMAGE_TYPE_MCUBOOT, id, size, nullptr);
	if (result != 0) {
		goto exit;
	}

	for (uint32_t offset = 0; offset < size; offset += kBufferSize) {
		uint16_t chunkSize = MIN(size - offset, kBufferSize);
		result = flash_read(mFlashDevice, address + offset, flashImageBuffer, chunkSize);
		if (result != 0) {
			LOG_ERR("Failed to read data from flash");
			dfu_target_reset();
			goto exit;
		}

		result = dfu_target_write(flashImageBuffer, chunkSize);
		if (result != 0) {
			LOG_ERR("Failed to write data chunk");
			dfu_target_reset();
			goto exit;
		}

		/* Print progress log every 10th chunk to reduce number of messages */
		if (offset % (kBufferSize * 10) == 0) {
			LOG_INF("Image %d swap progress %u B/ %u B", id, offset, size);
		}
	}

	result = dfu_target_done(true);

exit:
	free(flashImageBuffer);

	return result;
}
