/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <linker/sections.h>
#include <sys/util.h>
#include <errno.h>
#include <nrf_cc310_bl_hash_sha256.h>
#include <devicetree.h>
#include <ocrypto_constant_time.h>
#include <bl_crypto.h>
#include "bl_crypto_cc310_common.h"

#define MAX_CHUNK_LEN 0x8000 /* Must be 4 byte aligned. */
#define CHUNK_LEN_STACK 0x200
#define RAM_BUFFER_LEN_WORDS ((MAX_CHUNK_LEN) / 4)
#define STACK_BUFFER_LEN_WORDS ((CHUNK_LEN_STACK) / 4)

/*! Illegal context pointer. */
#define CRYS_HASH_INVALID_USER_CONTEXT_POINTER_ERROR \
	(CRYS_HASH_MODULE_ERROR_BASE + 0x0UL)
/*! Context is corrupted. */
#define CRYS_HASH_USER_CONTEXT_CORRUPTED_ERROR \
	(CRYS_HASH_MODULE_ERROR_BASE + 0x2UL)
/*! Illegal result buffer pointer. */
#define CRYS_HASH_INVALID_RESULT_BUFFER_POINTER_ERROR \
	(CRYS_HASH_MODULE_ERROR_BASE + 0x5UL)
/*! Last block was already processed (may happen if previous block was not a
 *  multiple of block size). */
#define CRYS_HASH_LAST_BLOCK_ALREADY_PROCESSED_ERROR \
	(CRYS_HASH_MODULE_ERROR_BASE + 0xCUL)

BUILD_ASSERT_MSG(SHA256_CTX_SIZE >= sizeof(nrf_cc310_bl_hash_context_sha256_t), \
		"nrf_cc310_bl_hash_context_sha256_t can no longer fit inside " \
		"bl_sha256_ctx_t.");

static u32_t __noinit ram_buffer
	[RAM_BUFFER_LEN_WORDS]; /* Not stack allocated because of its size. */


static inline void *memcpy32(void *restrict d, const void *restrict s, size_t n)
{
	size_t len_words = ROUND_UP(n, 4) / 4;

	for (size_t i = 0; i < len_words; i++) {
		((u32_t *)d)[i] = ((u32_t *)s)[i];
	}
	return d;
}

int crypto_init_hash(void)
{
	return cc310_bl_init();
}


int bl_sha256_init(nrf_cc310_bl_hash_context_sha256_t * const ctx)
{
	CRYSError_t retval = nrf_cc310_bl_hash_sha256_init(ctx);
	if (retval == CRYS_HASH_INVALID_USER_CONTEXT_POINTER_ERROR) {
		return -EINVAL;
	}
	return retval;
}

static int hash_blocks(nrf_cc310_bl_hash_context_sha256_t *const ctx,
		const u8_t *data, u32_t data_len, const u32_t max_chunk_len,
		u32_t *buffer)
{
	u32_t remaining_copy_len = data_len;
	u32_t chunk_len = MIN(data_len, max_chunk_len);
	CRYSError_t retval = CRYS_OK;

	cc310_bl_backend_enable();
	for (u32_t i = 0; i < data_len; i += max_chunk_len) {
		u8_t const * source = &data[i];
		if (buffer) {
			memcpy32(buffer, source, chunk_len);
			source = (u8_t *)buffer;
		}

		retval = nrf_cc310_bl_hash_sha256_update(ctx, source, chunk_len);

		if (retval != CRYS_OK) {
			break;
		}

		remaining_copy_len -= chunk_len;
		chunk_len = MIN(remaining_copy_len, max_chunk_len);
	}
	cc310_bl_backend_disable();

	return retval;
}


static inline int hash_blocks_stack(nrf_cc310_bl_hash_context_sha256_t *const ctx,
		const u8_t *data, u32_t data_len, u32_t chunk_len)
{
	u32_t stack_buffer[chunk_len/4];
	return hash_blocks(ctx, data, data_len, chunk_len, stack_buffer);
}

/* Base implementation with 'external' parameter. */
static int sha256_update(nrf_cc310_bl_hash_context_sha256_t *const ctx,
		const u8_t *data, u32_t data_len, bool external)
{
	CRYSError_t retval;

	if ((u32_t)data < CONFIG_SRAM_BASE_ADDRESS) {
		/* Copy to RAM buffer, then hash. */
		/* Cryptocell has DMA access to RAM only */
		if (external) {
			retval = hash_blocks_stack(ctx, data, data_len, CHUNK_LEN_STACK);
		} else {
			retval = hash_blocks(ctx, data, data_len, MAX_CHUNK_LEN, ram_buffer);
		}
	} else {
		retval = hash_blocks(ctx, data, data_len, MAX_CHUNK_LEN, NULL);
	}
	switch (retval) {
	case CRYS_HASH_INVALID_USER_CONTEXT_POINTER_ERROR:
	case CRYS_HASH_USER_CONTEXT_CORRUPTED_ERROR:
		return -EINVAL;
	case CRYS_HASH_LAST_BLOCK_ALREADY_PROCESSED_ERROR:
		return -ENOTSUP;
	default:
		return retval;
	}
}

int bl_sha256_update(nrf_cc310_bl_hash_context_sha256_t *ctx,
		const u8_t * data, u32_t data_len)
{
	return sha256_update(ctx, data, data_len, true);
}

int bl_sha256_finalize(nrf_cc310_bl_hash_context_sha256_t *ctx, u8_t *output)
{
	cc310_bl_backend_enable();
	CRYSError_t retval = nrf_cc310_bl_hash_sha256_finalize(ctx,
			(nrf_cc310_bl_hash_digest_sha256_t *) output);
	cc310_bl_backend_disable();

	switch (retval) {
	case CRYS_HASH_INVALID_USER_CONTEXT_POINTER_ERROR:
	case CRYS_HASH_USER_CONTEXT_CORRUPTED_ERROR:
	case CRYS_HASH_INVALID_RESULT_BUFFER_POINTER_ERROR:
		return -EINVAL;
	default:
		return retval;
	}
}

int get_hash(u8_t *hash, const u8_t *data, u32_t data_len, bool external)
{
	nrf_cc310_bl_hash_context_sha256_t ctx;
	int retval;

	retval = bl_sha256_init(&ctx);
	if (retval != 0) {
		return retval;
	}

	retval = sha256_update(&ctx, data, data_len, external);
	if (retval != 0) {
		return retval;
	}

	retval = bl_sha256_finalize(&ctx, hash);
	return retval;
}
