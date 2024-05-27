/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_flash_sink.h>
#include <zephyr/drivers/flash.h>
#include <suit_memory_layout.h>

#define SWAP_BUFFER_SIZE 16
#define WRITE_OFFSET(a)	 (a->ptr + a->offset)

/* Set to more than one to allow multiple contexts in case of parallel execution */
#define SUIT_MAX_FLASH_COMPONENTS 1

#define IS_COND_TRUE(c) ((c) ? "True" : "False")

LOG_MODULE_REGISTER(suit_flash_sink, CONFIG_SUIT_LOG_LEVEL);

static suit_plat_err_t erase(void *ctx);
static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size);
static suit_plat_err_t seek(void *ctx, size_t offset);
static suit_plat_err_t flush(void *ctx);
static suit_plat_err_t used_storage(void *ctx, size_t *size);
static suit_plat_err_t release(void *ctx);

struct flash_ctx {
	size_t size_used;
	size_t offset;
	size_t offset_limit;
	uintptr_t ptr;
	const struct device *fdev;
	size_t flash_write_size;
	bool in_use;
};

static struct flash_ctx ctx[SUIT_MAX_FLASH_COMPONENTS];

/**
 * @brief Get the new, free ctx object
 *
 * @return struct flash_ctx* or NULL if no free ctx was found
 */
static struct flash_ctx *new_ctx_get(void)
{
	for (size_t i = 0; i < SUIT_MAX_FLASH_COMPONENTS; i++) {
		if (!ctx[i].in_use) {
			return &ctx[i];
		}
	}

	return NULL; /* No free ctx */
}

/**
 * @brief Register write by updating appropriate offsets and sizes
 *
 * @param flash_ctx Flash sink context pointer
 * @param write_size Size of written data
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t register_write(struct flash_ctx *flash_ctx, size_t write_size)
{
	flash_ctx->offset += write_size;

	if (flash_ctx->offset > flash_ctx->size_used) {
		flash_ctx->size_used = flash_ctx->offset;
	}

	return SUIT_PLAT_SUCCESS;
}

/**
 * @brief Get the flash write size for given flash driver
 *
 * @param fdev Flash driver to get the size from
 * @return size_t Write block size retrieved from driver
 */
static size_t flash_write_size_get(const struct device *fdev)
{
	const struct flash_parameters *parameters = flash_get_parameters(fdev);

	return parameters->write_block_size;
}

bool suit_flash_sink_is_address_supported(uint8_t *address)
{
	if (!suit_memory_global_address_is_in_nvm((uintptr_t)address)) {
		LOG_INF("Failed to find nvm area corresponding to address: %p", (void *)address);
		return false;
	}

	return true;
}

suit_plat_err_t erase(void *ctx)
{
	if (ctx != NULL) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;
		size_t size = flash_ctx->offset_limit - (size_t)flash_ctx->ptr;

		LOG_DBG("flash_sink_init_mem size %u", size);

		/* Erase requested area in preparation for data. */
		int res = flash_erase(flash_ctx->fdev, flash_ctx->ptr, size);

		if (res != 0) {
			LOG_ERR("Failed to erase requested memory area: %i", res);
			return SUIT_PLAT_ERR_IO;
		}

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_flash_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size)
{
	if ((dst != NULL) && (size > 0) && (sink != NULL)) {
		struct flash_ctx *ctx = new_ctx_get();

		if (ctx != NULL) {
			struct nvm_address nvm_address;

			if (!suit_memory_global_address_to_nvm_address((uintptr_t)dst,
								       &nvm_address)) {
				LOG_ERR("Failed to find nvm area corresponding to requested "
					"address.");
				return SUIT_PLAT_ERR_HW_NOT_READY;
			}

			if (!device_is_ready(nvm_address.fdev)) {
				LOG_ERR("Flash device not ready.");
				return SUIT_PLAT_ERR_HW_NOT_READY;
			}

			/* Check if requested area fits in found nvm */
			if (!suit_memory_global_address_range_is_in_nvm((uintptr_t)dst, size)) {
				LOG_ERR("Requested memory area out of bounds of corresponding nvm");
				return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
			}

			memset(ctx, 0, sizeof(*ctx));
			ctx->flash_write_size = flash_write_size_get(nvm_address.fdev);
			ctx->fdev = nvm_address.fdev;
			ctx->offset = 0;
			ctx->offset_limit = nvm_address.offset + size; /* max address */
			ctx->size_used = 0;
			ctx->ptr = nvm_address.offset;
			ctx->in_use = true;

			if (ctx->flash_write_size > SWAP_BUFFER_SIZE) {

				memset(ctx, 0, sizeof(*ctx));
				LOG_ERR("Write block size exceeds set safety limits");
				return SUIT_PLAT_ERR_INVAL;
			}

			sink->erase = erase;
			sink->write = write;
			sink->seek = seek;
			sink->flush = flush;
			sink->used_storage = used_storage;
			sink->release = release;
			sink->ctx = ctx;

			return SUIT_PLAT_SUCCESS; /* SUCCESS */
		}

		LOG_ERR("ERROR - SUIT_MAX_FLASH_COMPONENTS reached.");
		return SUIT_PLAT_ERR_NO_RESOURCES;
	}

	LOG_ERR("%s: Invalid arguments: %s, %s, %s", __func__, IS_COND_TRUE(dst != NULL),
		IS_COND_TRUE(size > 0), IS_COND_TRUE(sink != NULL));
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t write_unaligned_start(struct flash_ctx *flash_ctx, size_t *size_left,
					     const uint8_t **buf)
{
	uint8_t edit_buffer[SWAP_BUFFER_SIZE];

	size_t start_offset = 0;
	size_t block_start = 0;
	size_t write_size = 0;

	block_start = ((size_t)(WRITE_OFFSET(flash_ctx) / flash_ctx->flash_write_size)) *
		      flash_ctx->flash_write_size;
	start_offset = WRITE_OFFSET(flash_ctx) - block_start;
	write_size = 0;

	if (flash_read(flash_ctx->fdev, block_start, edit_buffer, flash_ctx->flash_write_size) !=
	    0) {
		LOG_ERR("Flash read failed.");
		return SUIT_PLAT_ERR_IO;
	}

	/* write_size - how much data from buf will be written */
	write_size = MIN(*size_left, flash_ctx->flash_write_size - start_offset);

	memcpy(edit_buffer + start_offset, *buf, write_size);

	/* Write back edit_buffer that now contains unaligned bytes from the start of buf */
	if (flash_write(flash_ctx->fdev, block_start, edit_buffer, flash_ctx->flash_write_size) !=
	    0) {
		LOG_ERR("Writing initial unaligned data failed.");
		return SUIT_PLAT_ERR_IO;
	}

	/* Move offset for bytes written */
	suit_plat_err_t ret = register_write(flash_ctx, write_size);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to update size after write");
		return ret;
	}

	/* Move input buffer ptr */
	*buf += write_size;

	/* Decrease size by the number of bytes written */
	*size_left -= write_size;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t write_aligned(struct flash_ctx *flash_ctx, size_t *size_left,
				     const uint8_t **buf, size_t write_size)
{
	size_t block_start = 0;

	/* Write part that is aligned */
	block_start = ((size_t)(WRITE_OFFSET(flash_ctx) / flash_ctx->flash_write_size)) *
		      flash_ctx->flash_write_size;

	if (flash_write(flash_ctx->fdev, block_start, *buf, write_size) != 0) {
		LOG_ERR("Writing aligned blocks failed.");
		return SUIT_PLAT_ERR_IO;
	}

	write_size = *size_left >= write_size ? write_size : *size_left;

	/* Move offset for bytes written */
	suit_plat_err_t ret = register_write(flash_ctx, write_size);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to update size after write");
		return ret;
	}

	/* Move input buffer ptr */
	*buf += write_size;

	/* Decrease size by the number of bytes written */
	*size_left -= write_size;
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t write_remaining(struct flash_ctx *flash_ctx, size_t *size_left,
				       const uint8_t **buf)
{
	uint8_t edit_buffer[SWAP_BUFFER_SIZE];
	size_t block_start = 0;

	/* Write remaining data */
	block_start = ((size_t)(WRITE_OFFSET(flash_ctx) / flash_ctx->flash_write_size)) *
		      flash_ctx->flash_write_size;

	if (flash_read(flash_ctx->fdev, block_start, edit_buffer, flash_ctx->flash_write_size) ==
	    0) {
		memcpy(edit_buffer, *buf, *size_left);

		/* Write back edit_buffer that now contains unaligned bytes from the start of buf */
		if (flash_write(flash_ctx->fdev, block_start, edit_buffer,
				flash_ctx->flash_write_size) != 0) {
			LOG_ERR("Writing remaining unaligned data failed.");
			return SUIT_PLAT_ERR_IO;
		}

		/* Move offset for bytes written */
		suit_plat_err_t ret = register_write(flash_ctx, *size_left);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to update size after write");
			return ret;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Flash read failed.");
	return SUIT_PLAT_ERR_IO;
}

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	size_t size_left = size;

	if ((ctx != NULL) && (buf != NULL) && (size_left > 0)) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		if (!flash_ctx->in_use) {
			LOG_ERR("flash_sink not initialized.");
			return SUIT_PLAT_ERR_INVAL;
		}

		if (flash_ctx->fdev == NULL) {
			LOG_ERR("%s: fdev is NULL.", __func__);
			return SUIT_PLAT_ERR_INVAL;
		}

		if ((flash_ctx->offset_limit - (size_t)flash_ctx->ptr) >= size_left) {
			if (flash_ctx->flash_write_size == 1) {
				int ret = flash_write(flash_ctx->fdev, WRITE_OFFSET(flash_ctx), buf,
						      size_left);

				if (ret == 0) {
					ret = register_write(flash_ctx, size_left);
					if (ret != SUIT_PLAT_SUCCESS) {
						LOG_ERR("Failed to update size after write");
						return ret;
					}
				} else {
					ret = SUIT_PLAT_ERR_IO;
				}

				return ret;
			}

			size_t write_size = 0;
			suit_plat_err_t err = 0;

			if (WRITE_OFFSET(flash_ctx) % flash_ctx->flash_write_size) {
				/* Write offset is not aligned with start of block */
				err = write_unaligned_start(flash_ctx, &size_left, &buf);

				if (err != SUIT_PLAT_SUCCESS) {
					return err;
				}

				if (size_left == 0) {
					/* All data written */
					return SUIT_PLAT_SUCCESS;
				}
			}

			/* Number of bytes to be written in context of whole blocks */
			write_size = (size_left / flash_ctx->flash_write_size) *
				     flash_ctx->flash_write_size;

			if (write_size > 0) {
				err = write_aligned(flash_ctx, &size_left, &buf, write_size);

				if (err != SUIT_PLAT_SUCCESS) {
					return err;
				}

				if (size_left == 0) {
					/* All data written */
					return SUIT_PLAT_SUCCESS;
				}
			}

			return write_remaining(flash_ctx, &size_left, &buf);

		} else {
			LOG_ERR("Write out of bounds.");
			return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
		}
	}

	LOG_ERR("%s: Invalid arguments. %s, %s, %s", __func__, IS_COND_TRUE(ctx != NULL),
		IS_COND_TRUE(buf != NULL), IS_COND_TRUE(size_left > 0));
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t seek(void *ctx, size_t offset)
{
	if (ctx != NULL) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		if (offset < (flash_ctx->offset_limit - (size_t)flash_ctx->ptr)) {
			flash_ctx->offset = offset;
			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Invalid argument - offset value out of range");
		return SUIT_PLAT_ERR_INVAL;
	}

	LOG_ERR("%s: Invalid arguments - ctx is NULL", __func__);
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t flush(void *ctx)
{
	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		*size = flash_ctx->size_used;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("%s: Invalid arguments. %s, %s", __func__, IS_COND_TRUE(ctx != NULL),
		IS_COND_TRUE(size != NULL));
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t release(void *ctx)
{
	if (ctx != NULL) {
		struct flash_ctx *flash_ctx = (struct flash_ctx *)ctx;

		flash_ctx->offset = 0;
		flash_ctx->offset_limit = 0;
		flash_ctx->size_used = 0;
		flash_ctx->ptr = 0;
		flash_ctx->fdev = NULL;
		flash_ctx->in_use = false;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("%s: Invalid arguments - ctx is NULL", __func__);
	return SUIT_PLAT_ERR_INVAL;
}
