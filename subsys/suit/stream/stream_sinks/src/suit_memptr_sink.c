/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_memptr_sink.h>

LOG_MODULE_REGISTER(suit_memptr_sink, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size);
static suit_plat_err_t used_storage(void *ctx, size_t *size);

suit_plat_err_t suit_memptr_sink_get(struct stream_sink *sink, memptr_storage_handle_t handle)
{
	if ((sink != NULL) && (handle != NULL)) {
		sink->erase = NULL;
		sink->write = write;
		sink->seek = NULL;
		sink->flush = NULL;
		sink->used_storage = used_storage;
		sink->release = NULL;
		sink->ctx = handle;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
};

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	suit_memptr_storage_err_t err;

	if ((ctx != NULL) && (buf != NULL) && (size != 0)) {
		err = suit_memptr_storage_ptr_store(ctx, buf, size);
		if (err != SUIT_PLAT_SUCCESS) {
			/* In the current conditions suit_memptr_storage_ptr_store will only
			 * fail with SUIT_MEMPTR_STORAGE_ERR_UNALLOCATED_RECORD
			 */
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		const uint8_t *payload_ptr;
		size_t payload_size;

		if (suit_memptr_storage_ptr_get(ctx, &payload_ptr, &payload_size) ==
		    SUIT_PLAT_SUCCESS) {
			if (payload_ptr != NULL) {
				*size = payload_size;
			} else {
				*size = 0;
			}

			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Storage get failed");
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}
