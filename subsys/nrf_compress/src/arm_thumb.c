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

/* Requires 2 extra bytes to allow checking cross-chunk 16-bit aligned ARM thumb instructions */
#define EXTRA_BUFFER_SIZE 2

static uint8_t output_buffer[CONFIG_NRF_COMPRESS_CHUNK_SIZE + EXTRA_BUFFER_SIZE];
static uint8_t temp_extra_buffer[EXTRA_BUFFER_SIZE];
static uint32_t data_position = 0;
static bool has_extra_buffer_data;

static int arm_thumb_init(void *inst)
{
	data_position = 0;
	has_extra_buffer_data = false;

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
	has_extra_buffer_data = false;
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
	bool end_part_match = false;
	bool extra_buffer_used = false;

	if (input_size > CONFIG_NRF_COMPRESS_CHUNK_SIZE) {
		return -EINVAL;
	}

	if (has_extra_buffer_data == true) {
		/* Copy bytes from temporary holding buffer */
		memcpy(output_buffer, temp_extra_buffer, sizeof(temp_extra_buffer));
		memcpy(&output_buffer[sizeof(temp_extra_buffer)], input, input_size);
		end_part_match = true;
		extra_buffer_used = true;
		has_extra_buffer_data = false;
		input_size += sizeof(temp_extra_buffer);
	} else {
		memcpy(output_buffer, input, input_size);
	}

	arm_thumb_filter(output_buffer, input_size, data_position, false, &end_part_match);
	data_position += input_size;
	*offset = input_size;

	if (extra_buffer_used) {
		*offset -= sizeof(temp_extra_buffer);
	}

	*output = output_buffer;
	*output_size = input_size;

	if (end_part_match == true && !last_part) {
		/* Partial match at end of input, need to cut the final 2 bytes off and stash
		 * them
		 */
		memcpy(temp_extra_buffer, &output_buffer[(input_size - sizeof(temp_extra_buffer))],
		       sizeof(temp_extra_buffer));
		has_extra_buffer_data = true;
		*output_size -= sizeof(temp_extra_buffer);
		data_position -= sizeof(temp_extra_buffer);
	}

	return 0;
}

NRF_COMPRESS_IMPLEMENTATION_DEFINE(arm_thumb, NRF_COMPRESS_TYPE_ARM_THUMB, arm_thumb_init,
				   arm_thumb_deinit, arm_thumb_reset, NULL,
				   arm_thumb_bytes_needed, arm_thumb_decompress);
