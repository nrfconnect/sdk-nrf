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

/* Set to more than one to allow multiple contexts in case of parallel execution */
#define SUIT_MAX_RAM_COMPONENTS 1

LOG_MODULE_REGISTER(suit_ram_sink, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t erase(void *ctx);
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
	if ((address == NULL) || !suit_memory_global_address_is_in_ram((uintptr_t)address)) {
		LOG_INF("Failed to find RAM area corresponding to address: %p", address);
		return false;
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

			sink->erase = erase;
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

static suit_plat_err_t erase(void *ctx)
{
	if (ctx != NULL) {
		struct ram_ctx *ram_ctx = (struct ram_ctx *)ctx;
		size_t size = ram_ctx->offset_limit - (size_t)ram_ctx->ptr;

		uint8_t *dst = (uint8_t *)suit_memory_global_address_to_ram_address(
			(uintptr_t)ram_ctx->ptr);

		if (dst == NULL) {
			return SUIT_PLAT_ERR_INVAL;
		}

		memset(dst, 0, size);
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	if ((ctx != NULL) && (buf != NULL) && (size > 0)) {
		struct ram_ctx *ram_ctx = (struct ram_ctx *)ctx;

		if ((ram_ctx->offset_limit - (size_t)ram_ctx->ptr) >= size) {
			uint8_t *dst = (uint8_t *)suit_memory_global_address_to_ram_address(
				(uintptr_t)ram_ctx->ptr);

			if (dst == NULL) {
				return SUIT_PLAT_ERR_INVAL;
			}

			memcpy(dst, buf, size);
			ram_ctx->offset += size;

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
