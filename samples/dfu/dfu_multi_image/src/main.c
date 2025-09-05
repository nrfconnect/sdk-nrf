/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <dfu/dfu_multi_image.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>
#include <zephyr/dfu/mcuboot.h>
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <nrf53_cpunet_mgmt.h>
#endif

#include "dfu_multi_image_sample_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dfu_multi_image_sample);

uint8_t dfu_multi_image_sample_buffer[DFU_MULTI_IMAGE_BUFFER_SIZE];

static void dfu_target_callback(enum dfu_target_evt_id evt_id)
{
	printf("DFU target event received: %d\n", evt_id);
}

static int writer_open(int image_id, size_t image_size)
{
	LOG_INF("Opening DFU target image %d with size %zu", image_id, image_size);
	return dfu_target_init(DFU_TARGET_IMAGE_TYPE_MCUBOOT, image_id, image_size,
			       dfu_target_callback);
}

static int writer_write(const uint8_t *chunk, size_t chunk_size)
{
	LOG_INF("Writing %zu bytes to DFU target image", chunk_size);
	return dfu_target_write(chunk, chunk_size);
}

static int writer_close(bool success)
{
	LOG_INF("Closing DFU target image %s", success ? "successfully" : "with failure");
	return dfu_target_done(success);
}

#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
static int writer_offset(size_t *offset)
{
	return dfu_target_offset_get(offset);
}
#endif

static int writer_reset(void)
{
	LOG_INF("Resetting DFU target image");
	return dfu_target_reset();
}

int dfu_multi_image_sample_lib_prepare(void)
{
	int ret = 0;

	ret = dfu_target_mcuboot_set_buf(dfu_multi_image_sample_buffer,
					 DFU_MULTI_IMAGE_BUFFER_SIZE);

	if (ret < 0) {
		LOG_ERR("Failed to set buffer for MCUboot DFU target: %d", ret);
		return ret;
	}

	ret = dfu_multi_image_init(dfu_multi_image_sample_buffer, DFU_MULTI_IMAGE_BUFFER_SIZE);

	if (ret < 0) {
		LOG_ERR("Failed to initialize dfu_multi_image: %d", ret);
		return ret;
	}

	for (int image_id = 0; image_id < CONFIG_UPDATEABLE_IMAGE_NUMBER; ++image_id) {
		struct dfu_image_writer writer;

		writer.image_id = image_id;
		writer.open = writer_open;
		writer.write = writer_write;
		writer.close = writer_close;
#ifdef CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS
		writer.offset = writer_offset;
#endif /* CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS */
		writer.reset = writer_reset;

		ret = dfu_multi_image_register_writer(&writer);
		if (ret < 0) {
			LOG_ERR("Failed to register writer for image %d: %d", image_id, ret);
			return ret;
		}
	};

	return 0;
}

int main(void)
{
	int ret = 0;

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
	nrf53_cpunet_enable(true);
#endif

	ret = dfu_multi_image_sample_lib_prepare();

	if (ret < 0) {
		printf("Failed to prepare dfu_multi_image sample: %d\n", ret);
		return ret;
	}
	/* using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	printf("Starting dfu_multi_image sample, build time: %s %s\n", __DATE__, __TIME__);

	return 0;
}
