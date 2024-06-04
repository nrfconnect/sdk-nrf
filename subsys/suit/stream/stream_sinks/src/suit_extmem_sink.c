/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <suit_extmem_sink.h>
#include <zephyr/sys/bitarray.h>
#include <sdfw/sdfw_services/extmem_service.h>
#include <suit_memory_layout.h>

#define SUIT_MAX_EXTMEM_COMPONENTS 1

LOG_MODULE_REGISTER(suit_extmem_sink, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t erase(void *ctx);
static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size);
static suit_plat_err_t seek(void *ctx, size_t offset);
static suit_plat_err_t used_storage(void *ctx, size_t *size);
static suit_plat_err_t release(void *ctx);

struct extmem_ctx {
	uintptr_t start_offset;
	uintptr_t end_offset;
	size_t bytes_written;
	size_t size;
	size_t max_storage_used;
	uint8_t *ptr;
};

static struct extmem_ctx ctx[SUIT_MAX_EXTMEM_COMPONENTS];

SYS_BITARRAY_DEFINE_STATIC(ctx_bitarray, SUIT_MAX_EXTMEM_COMPONENTS);

/**
 * @brief Get the new ctx object
 *
 * @return struct extmem_ctx* or NULL if no free ctx was found
 */
static struct extmem_ctx *alloc_ctx(void)
{
	size_t offset;

	if (sys_bitarray_alloc(&ctx_bitarray, 1, &offset) < 0) {
		return NULL;
	}

	return &ctx[offset];
}

/**
 * @brief Free ctx object
 */
static void free_ctx(struct extmem_ctx *ctx_to_free)
{
	int err;

	err = sys_bitarray_free(&ctx_bitarray, 1, ctx_to_free - ctx);
	__ASSERT(err == 0, "Wrong context was freed");
}

static bool is_address_supported(uintptr_t address, struct extmem_capabilities *capabilities)
{
	uintptr_t extmem_area_start = capabilities->base_addr;
	size_t extmem_area_size = capabilities->capacity;

	return suit_memory_global_address_range_is_in_external_memory(extmem_area_start,
								      extmem_area_size);
}

bool suit_extmem_sink_is_address_supported(uint8_t *address)
{
	struct extmem_capabilities capabilities;
	int err;

	if (address == NULL) {
		return false;
	}

	err = extmem_capabilities_get(&capabilities);
	if (err) {
		LOG_INF("Extmem remote error %d. External memory not accessible", err);
		return false;
	}

	return is_address_supported((uintptr_t)address, &capabilities);
}

suit_plat_err_t suit_extmem_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size)
{
	struct extmem_capabilities capabilities;
	int err;

	err = extmem_capabilities_get(&capabilities);
	if (err) {
		LOG_ERR("Extmem remote error %d. External memory not accessible", err);
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	if ((dst != NULL) && (size > 0)) {
		struct extmem_ctx *ctx = alloc_ctx();
		uintptr_t start_addr = (uintptr_t)dst;
		uintptr_t end_addr = (uintptr_t)dst + size;

		/* Check if requested area fits in external memory area */
		if (!is_address_supported(start_addr, &capabilities) ||
		    !is_address_supported(end_addr, &capabilities)) {
			LOG_ERR("Requested memory area (%p) out of bounds", (dst + size));
			return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
		}

		if (ctx != NULL) {
			ctx->start_offset = start_addr - capabilities.base_addr;
			ctx->end_offset = end_addr - capabilities.base_addr;
			ctx->ptr = dst;
			ctx->bytes_written = 0;
			ctx->size = size;
			ctx->max_storage_used = 0;

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
	if (ctx == NULL) {
		LOG_ERR("Invalid arguments.");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct extmem_ctx *extmem_ctx = (struct extmem_ctx *)ctx;

	LOG_DBG("Erasing external memory at offset: %lu size: %u", extmem_ctx->start_offset,
		extmem_ctx->size);
	int err = extmem_erase(extmem_ctx->start_offset, extmem_ctx->size);

	if (err) {
		LOG_ERR("Could not erase external memory: %d", err);
		return SUIT_PLAT_ERR_IO;
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	if (ctx == NULL || buf == NULL || size == 0) {
		LOG_ERR("Invalid arguments.");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct extmem_ctx *extmem_ctx = (struct extmem_ctx *)ctx;
	uintptr_t current_offset = extmem_ctx->start_offset + extmem_ctx->bytes_written;
	uintptr_t write_end = current_offset + size;

	if (current_offset >= extmem_ctx->end_offset || write_end > extmem_ctx->end_offset) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	LOG_DBG("Writing data at offset: %lu size: %u", current_offset, size);
	int err = extmem_write(current_offset, buf, size);

	if (err) {
		LOG_ERR("Could not erase external memory: %d", err);
		return SUIT_PLAT_ERR_IO;
	}

	extmem_ctx->bytes_written += size;
	extmem_ctx->max_storage_used = MAX(extmem_ctx->bytes_written, extmem_ctx->max_storage_used);

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t seek(void *ctx, size_t offset)
{
	if (ctx == NULL) {
		LOG_ERR("Invalid arguments.");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct extmem_ctx *extmem_ctx = (struct extmem_ctx *)ctx;

	if (!(extmem_ctx->start_offset <= offset && offset < extmem_ctx->end_offset)) {
		LOG_ERR("Offset out of bounds (%lu <= %u < %lu)", extmem_ctx->start_offset, offset,
			extmem_ctx->end_offset);
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	extmem_ctx->bytes_written = offset;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	if (ctx == NULL || size == NULL) {
		LOG_ERR("Invalid arguments.");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct extmem_ctx *extmem_ctx = (struct extmem_ctx *)ctx;

	*size = extmem_ctx->max_storage_used;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t release(void *ctx)
{
	if (ctx == NULL) {
		LOG_ERR("Invalid arguments.");
		return SUIT_PLAT_ERR_INVAL;
	}

	free_ctx(ctx);

	return SUIT_PLAT_SUCCESS;
}
