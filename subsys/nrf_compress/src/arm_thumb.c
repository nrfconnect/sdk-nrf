/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <armthumb.h>
#include <nrf_compress/implementation.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_compress_arm_thumb, CONFIG_NRF_COMPRESS_LOG_LEVEL);

BUILD_ASSERT((CONFIG_NRF_COMPRESS_CHUNK_SIZE % 4) == 0,
	     "CONFIG_NRF_COMPRESS_CHUNK_SIZE must be multiple of 4");

static uint8_t output_buffer[CONFIG_NRF_COMPRESS_CHUNK_SIZE];
static uint32_t data_position = 0;

static int arm_thumb_init(void *inst)
{
	data_position = 0;

	return 0;
}

static int arm_thumb_deinit(void *inst)
{
#ifdef CONFIG_NRF_COMPRESS_CLEANUP
	memset(output_buffer, 0x00, sizeof(output_buffer));
#endif

	return 0;
}

static int arm_thumb_reset(void *inst)
{
	data_position = 0;
	memset(output_buffer, 0x00, sizeof(output_buffer));

	return 0;
}

static size_t arm_thumb_bytes_needed(void *inst)
{
	return CONFIG_NRF_COMPRESS_CHUNK_SIZE;
}

static int arm_thumb_decompress(void *inst, const uint8_t *input, size_t input_size,
				bool last_part, uint32_t *offset, uint8_t **output,
				size_t *output_size)
{
	if (input_size > CONFIG_NRF_COMPRESS_CHUNK_SIZE) {
		return -EINVAL;
	}

	memcpy(output_buffer, input, input_size);
	arm_thumb_filter(output_buffer, input_size, data_position, false);
	data_position += input_size;
	*offset = input_size;
	*output = output_buffer;
	*output_size = input_size;

	return 0;
}

NRF_COMPRESS_IMPLEMENTATION_DEFINE(arm_thumb, NRF_COMPRESS_TYPE_ARM_THUMB, arm_thumb_init,
				   arm_thumb_deinit, arm_thumb_reset, NULL,
				   arm_thumb_bytes_needed, arm_thumb_decompress);
