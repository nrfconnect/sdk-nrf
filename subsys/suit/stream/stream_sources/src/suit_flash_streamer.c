/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_memptr_streamer.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <suit_flash_streamer.h>
#include <suit_memory_layout.h>

LOG_MODULE_REGISTER(suit_flash_streamer, CONFIG_SUIT_LOG_LEVEL);

#define READ_CHUNK_SIZE 256

bool suit_flash_streamer_address_in_range(const uint8_t *address)
{
	return suit_memory_global_address_is_in_external_memory((uintptr_t)address);
}

suit_plat_err_t suit_flash_streamer_stream(const uint8_t *payload, size_t payload_size,
					   struct stream_sink *sink)
{
	const struct device *fdev = suit_memory_external_memory_device_get();
	uintptr_t offset;
	size_t bytes_read = 0;
	uint8_t read_buffer[READ_CHUNK_SIZE];

	if (fdev == NULL) {
		return SUIT_PLAT_ERR_IO;
	}

	if (!device_is_ready(fdev)) {
		return SUIT_PLAT_ERR_HW_NOT_READY;
	}

	if (!suit_memory_global_address_to_external_memory_offset((uintptr_t)payload, &offset)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if ((payload == NULL) || (sink == NULL) || (sink->write == NULL) || (payload_size <= 0)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	do {
		size_t bytes_remaining = payload_size - bytes_read;
		size_t read_size = MIN(bytes_remaining, READ_CHUNK_SIZE);

		int result = flash_read(fdev, offset, read_buffer, read_size);

		if (result != 0) {
			return SUIT_PLAT_ERR_IO;
		}

		suit_plat_err_t ret = sink->write(sink->ctx, read_buffer, read_size);

		if (ret != SUIT_PLAT_SUCCESS) {
			return ret;
		}

		bytes_read += read_size;
	} while (bytes_read < payload_size);

	return SUIT_PLAT_SUCCESS;
}
