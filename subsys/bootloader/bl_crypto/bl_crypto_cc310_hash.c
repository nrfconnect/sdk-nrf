/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <linker/sections.h>
#include <misc/util.h>
#include <nrf_cc310_bl_hash_sha256.h>
#include <generated_dts_board.h>
#include <occ_constant_time.h>
#include "bl_crypto_cc310_common.h"

#define MAX_CHUNK_LEN 0x8000 /* Must be 4 byte aligned. */
#define RAM_BUFFER_LEN_WORDS ((MAX_CHUNK_LEN) / 4)

static u32_t __noinit ram_buffer
	[RAM_BUFFER_LEN_WORDS]; /* Not stack allocated because of its size. */

static bool cc310_bl_hash(u8_t *out_hash, const u8_t *data,
			  u32_t data_len)
{
	nrf_cc310_bl_hash_context_sha256_t context;

	if (nrf_cc310_bl_hash_sha256_init(&context) != CRYS_OK) {
		return false;
	}

	if ((u32_t)data < CONFIG_SRAM_BASE_ADDRESS) {
		/* Cryptocell has DMA access to RAM only */
		u32_t remaining_copy_len = data_len;
		u32_t block_len = min(remaining_copy_len, MAX_CHUNK_LEN);

		for (u32_t i = 0; i < data_len; i += MAX_CHUNK_LEN) {
			memcpy32(ram_buffer, &data[i], block_len);

			if (nrf_cc310_bl_hash_sha256_update(
				    &context, (u8_t *)ram_buffer,
				    block_len) != CRYS_OK) {
				return false;
			}

			remaining_copy_len -= block_len;
			block_len = min(remaining_copy_len, MAX_CHUNK_LEN);
		}
	} else {
		for (u32_t i = 0; i < data_len; i += MAX_CHUNK_LEN) {
			if (nrf_cc310_bl_hash_sha256_update(
				    &context, &data[i],
				    min(data_len - i, MAX_CHUNK_LEN)) !=
			    CRYS_OK) {
				return false;
			}
		}
	}

	if (nrf_cc310_bl_hash_sha256_finalize(
		    &context, (nrf_cc310_bl_hash_digest_sha256_t *)out_hash) !=
	    CRYS_OK) {
		return false;
	}

	return true;
}

bool get_hash(u8_t *hash, const u8_t *data, u32_t data_len)
{
	if (!cc310_bl_init()) {
		return false;
	}

	cc310_bl_backend_enable();

	if (!cc310_bl_hash(hash, data, data_len)) {
		return false;
	}

	cc310_bl_backend_disable();
	return true;
}

bool verify_truncated_hash(const u8_t *data, u32_t data_len,
			   const u8_t *expected, u32_t hash_len)
{
	u8_t hash[CONFIG_SB_HASH_LEN];

	if (!get_hash(hash, data, data_len)) {
		return false;
	}

	return occ_constant_time_equal(expected, hash, hash_len);
}

bool verify_hash(const u8_t *data, u32_t data_len,
		 const u8_t *expected)
{
	return verify_truncated_hash(data, data_len, expected,
				     CONFIG_SB_HASH_LEN);
}
