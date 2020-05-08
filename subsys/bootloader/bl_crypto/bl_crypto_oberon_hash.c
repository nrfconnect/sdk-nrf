/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <ocrypto_sha256.h>
#include <ocrypto_constant_time.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <bl_crypto.h>


BUILD_ASSERT(SHA256_CTX_SIZE >= sizeof(ocrypto_sha256_ctx), \
		"ocrypto_sha256_ctx can no longer fit inside bl_sha256_ctx_t.");

int get_hash(u8_t *hash, const u8_t *data, u32_t data_len, bool external)
{
	(void) external;
	ocrypto_sha256(hash, data, data_len);

	/* Return success always as ocrypto_sha256 does not have a return value. */
	return 0;
}

int bl_sha256_init(ocrypto_sha256_ctx *ctx)
{
	if (ctx == NULL) {
		return -EFAULT;
	}
	ocrypto_sha256_init(ctx);
	return 0;
}

int bl_sha256_update(ocrypto_sha256_ctx *ctx, const u8_t *data, u32_t data_len)
{
	if (!ctx || !data) {
		return -EINVAL;
	}
	ocrypto_sha256_update(ctx, data, (size_t) data_len);
	return 0;
}

int bl_sha256_finalize(ocrypto_sha256_ctx *ctx, u8_t *output)
{
	if (!ctx || !output) {
		return -EINVAL;
	}
	ocrypto_sha256_final(ctx, output);
	return 0;
}
