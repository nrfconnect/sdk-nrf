/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bl_crypto.h>
#include <fw_info.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_BL_ROT_VERIFY_EXT_API_REQUIRED
EXT_API_REQ(BL_ROT_VERIFY, 1, struct bl_rot_verify_ext_api, bl_rot_verify);

int root_of_trust_verify(const uint8_t *public_key, const uint8_t *public_key_hash,
			 const uint8_t *signature, const uint8_t *firmware,
			 const uint32_t firmware_len, bool external)
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

int bl_sha256_update(bl_sha256_ctx_t *ctx, const uint8_t *data, uint32_t data_len)
{
	return bl_sha256->ext_api.bl_sha256_update(ctx, data, data_len);
}

int bl_sha256_finalize(bl_sha256_ctx_t *ctx, uint8_t *output)
{
	return bl_sha256->ext_api.bl_sha256_finalize(ctx, output);
}

int bl_sha256_verify(const uint8_t *data, uint32_t data_len, const uint8_t *expected)
{
	return bl_sha256->ext_api.bl_sha256_verify(data, data_len, expected);
}

int get_hash(uint8_t *hash, const uint8_t *data, uint32_t data_len, bool external)
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

int bl_secp256r1_validate(const uint8_t *hash, uint32_t hash_len,
			const uint8_t *public_key, const uint8_t *signature)
{
	return bl_secp256r1->ext_api.bl_secp256r1_validate(hash, hash_len,
							public_key, signature);
}
#endif
