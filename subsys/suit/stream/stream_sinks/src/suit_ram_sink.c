/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <suit_ram_sink.h>
#include <suit_memory_layout.h>
#include <zephyr/cache.h>

/* Set to more than one to allow multiple contexts in case of parallel execution */
#define SUIT_MAX_RAM_COMPONENTS 1

LOG_MODULE_REGISTER(suit_ram_sink, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size);
static suit_plat_err_t seek(void *ctx, size_t offset);
static suit_plat_err_t used_storage(void *ctx, size_t *size);
static suit_plat_err_t release(void *ctx);

struct ram_ctx {
	size_t size_used;
	size_t offset;
	size_t offset_limit;
	uint8_t *ptr;
	bool in_use;
};

static struct ram_ctx ctx[SUIT_MAX_RAM_COMPONENTS];

/**
 * @brief Get the new, free ctx object
 *
 * @return struct ram_ctx* or NULL if no free ctx was found
 */
static struct ram_ctx *get_new_ctx(void)
{
	for (size_t i = 0; i < SUIT_MAX_RAM_COMPONENTS; i++) {
		if (!ctx[i].in_use) {
			return &ctx[i];
		}
	}

	return NULL; /* No free ctx */
}

bool suit_ram_sink_is_address_supported(uint8_t *address)
{
	const size_t cache_line_size = sys_cache_data_line_size_get();

	if ((address == NULL) || !suit_memory_global_address_is_in_ram((uintptr_t)address)) {
		LOG_INF("Failed to find RAM area corresponding to address: %p", address);
		return false;
	}

	if (cache_line_size > 0) {
		if ((((uintptr_t)address) % cache_line_size) != 0) {
			LOG_ERR("Requested memory area (%p) must be aligned with cache lines (%p)",
				(void *)address, (void *)cache_line_size);
			return SUIT_PLAT_ERR_INVAL;
		}
	}

	return true;
}

suit_plat_err_t suit_ram_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size)
{
	if ((dst != NULL) && (size > 0)) {
		struct ram_ctx *ctx = get_new_ctx();

		/* Check if requested area fits in found RAM */
		if (!suit_memory_global_address_range_is_in_ram((uintptr_t)dst, size)) {
			LOG_ERR("Requested memory area (%p) is not within RAM", (void *)dst);
			return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
		}

		if (ctx != NULL) {
			ctx->offset = 0;
			ctx->offset_limit = (size_t)dst + size;
			ctx->size_used = 0;
			ctx->ptr = dst;
			ctx->in_use = true;

			sink->erase = NULL;
			sink->write = write;
			sink->seek = seek;
			sink->flush = NULL;
			sink->used_storage = used_storage;
			sink->release = release;
			sink->ctx = ctx;

			return SUIT_PLAT_SUCCESS; /* SUCCESS */
		}

		LOG_ERR("ERROR - SUIT_MAX_RAM_COMPONENTS reached.");
		return SUIT_PLAT_ERR_NO_RESOURCES;
	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	if ((ctx != NULL) && (buf != NULL) && (size > 0)) {
		struct ram_ctx *ram_ctx = (struct ram_ctx *)ctx;

		if ((ram_ctx->offset_limit - (size_t)ram_ctx->ptr + ram_ctx->offset) >= size) {
			uint8_t *dst = (uint8_t *)suit_memory_global_address_to_ram_address(
				(uintptr_t)(ram_ctx->ptr + ram_ctx->offset));

			if (dst == NULL) {
				return SUIT_PLAT_ERR_INVAL;
			}

			memcpy(dst, buf, size);
			ram_ctx->offset += size;

			/* It is guaranteed by the suit_ram_sink_is_address_supported() that
			 * the ram_ctx->ptr is aligned with the cache lines,
			 * so a simple address alignment implemented inside the cache driver
			 * causes cache flush within the sink memory.
			 * In case of RAM sink usage for copying data using streamers,
			 * the destination address points to the local stack,
			 * so flushing should not have negative effects on the system.
			 */
			int ret = sys_cache_data_flush_range(dst, size);

			if (ret != 0 && ret != -EAGAIN && ret != -ENOTSUP) {
				LOG_ERR("Failed to flush cache buffer range (%p, 0x%x): %d",
					(void *)dst, size, ret);
				return SUIT_PLAT_ERR_INVAL;
			}

			if (ram_ctx->offset > ram_ctx->size_used) {
				ram_ctx->size_used = ram_ctx->offset;
			}

			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Write out of bounds.");
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t seek(void *ctx, size_t offset)
{
	if (ctx != NULL) {
		struct ram_ctx *ram_ctx = (struct ram_ctx *)ctx;

		if (offset < (ram_ctx->offset_limit - (size_t)ram_ctx->ptr)) {
			ram_ctx->offset = offset;
			return SUIT_PLAT_SUCCESS;
		}
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		struct ram_ctx *ram_ctx = (struct ram_ctx *)ctx;

		*size = ram_ctx->offset;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t release(void *ctx)
{
	if (ctx != NULL) {
		struct ram_ctx *ram_ctx = (struct ram_ctx *)ctx;

		ram_ctx->offset = 0;
		ram_ctx->offset_limit = 0;
		ram_ctx->size_used = 0;
		ram_ctx->ptr = NULL;
		ram_ctx->in_use = false;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_ram_sink_readback(void *sink_ctx, size_t offset, uint8_t *buf, size_t size)
{
	if ((sink_ctx != NULL) && (buf != NULL)) {
		struct ram_ctx *ram_ctx = (struct ram_ctx *)sink_ctx;

		bool ctx_found = false;

		for (int i = 0; i < SUIT_MAX_RAM_COMPONENTS; i++) {
			if (ram_ctx == &ctx[i]) {
				ctx_found = true;
				break;
			}
		}

		if (ctx_found == false) {
			LOG_ERR("Readback with invalid sink_ctx called.");
			return SUIT_PLAT_ERR_INVAL;
		}

		if ((ram_ctx->offset_limit - (size_t)ram_ctx->ptr + offset) >= size) {
			uint8_t *dst = (uint8_t *)suit_memory_global_address_to_ram_address(
				(uintptr_t)(ram_ctx->ptr + offset));

			if (dst == NULL) {
				return SUIT_PLAT_ERR_INVAL;
			}

			memcpy(buf, dst, size);

			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Read out of bounds.");
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}
