/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <bl_crypto.h>
#include <bl_storage.h>
#include <fw_info.h>
#include <psa/crypto.h>

int bl_sha512_init(bl_sha512_ctx_t * const ctx)
{
	*ctx = psa_hash_operation_init();
	return (int)psa_hash_setup(ctx, PSA_ALG_SHA_512);
}

int bl_sha512_update(bl_sha512_ctx_t *ctx, const uint8_t *data, uint32_t data_len)
{
	return (int)psa_hash_update(ctx, data, data_len);
}

int bl_sha512_finalize(bl_sha512_ctx_t *ctx, uint8_t *output)
{
	size_t hash_length = 0;
	/* Assumes the output buffer is at least the expected size of the hash */
	return (int)psa_hash_finish(ctx, output, PSA_HASH_LENGTH(PSA_ALG_SHA_512), &hash_length);
}

int get_hash(uint8_t *hash, const uint8_t *data, uint32_t data_len, bool external)
{
	bl_sha512_ctx_t ctx;
	int rc;

	rc = bl_sha512_init(&ctx);

	if (rc != 0) {
		return rc;
	}

	rc = bl_sha512_update(&ctx, data, data_len);

	if (rc != 0) {
		return rc;
	}

	rc = bl_sha512_finalize(&ctx, hash);
	return rc;
}
