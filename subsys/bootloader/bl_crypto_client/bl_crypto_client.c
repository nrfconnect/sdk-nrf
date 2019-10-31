/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bl_crypto.h>
#include <fw_info.h>
#include <kernel.h>

#ifdef CONFIG_BL_ROT_VERIFY_EXT_API_REQUIRED
EXT_API_REQ(BL_ROT_VERIFY, 1, struct bl_rot_verify_ext_api, bl_rot_verify);

int root_of_trust_verify(const u8_t *public_key, const u8_t *public_key_hash,
			 const u8_t *signature, const u8_t *firmware,
			 const u32_t firmware_len, bool external)
{
	(void)external; /* Unused parameter, only for compatibility. */
	return bl_rot_verify->ext_api.bl_root_of_trust_verify(public_key,
			public_key_hash, signature, firmware, firmware_len);
}
#endif


#ifdef CONFIG_BL_SHA256_EXT_API_REQUIRED
EXT_API_REQ(BL_SHA256, 1, struct bl_sha256_ext_api, bl_sha256);

int bl_sha256_init(bl_sha256_ctx_t *ctx)
{
	if (sizeof(*ctx) < bl_sha256->ext_api.bl_sha256_ctx_size) {
		return -EFAULT;
	}
	return bl_sha256->ext_api.bl_sha256_init(ctx);
}

int bl_sha256_update(bl_sha256_ctx_t *ctx, const u8_t *data, u32_t data_len)
{
	return bl_sha256->ext_api.bl_sha256_update(ctx, data, data_len);
}

int bl_sha256_finalize(bl_sha256_ctx_t *ctx, u8_t *output)
{
	return bl_sha256->ext_api.bl_sha256_finalize(ctx, output);
}

int bl_sha256_verify(const u8_t *data, u32_t data_len, const u8_t *expected)
{
	return bl_sha256->ext_api.bl_sha256_verify(data, data_len, expected);
}

int get_hash(u8_t *hash, const u8_t *data, u32_t data_len, bool external)
{
	bl_sha256_ctx_t ctx;
	int retval;

	retval = bl_sha256_init(&ctx);
	if (retval != 0) {
		return retval;
	}

	retval = bl_sha256_update(&ctx, data, data_len);
	if (retval != 0) {
		return retval;
	}

	retval = bl_sha256_finalize(&ctx, hash);
	return retval;
}
#endif


#ifdef CONFIG_BL_SECP256R1_EXT_API_REQUIRED
EXT_API_REQ(BL_SECP256R1, 1, struct bl_secp256r1_ext_api, bl_secp256r1);

int bl_secp256r1_validate(const u8_t *hash, u32_t hash_len,
			const u8_t *public_key, const u8_t *signature)
{
	return bl_secp256r1->ext_api.bl_secp256r1_validate(hash, hash_len,
							public_key, signature);
}
#endif
