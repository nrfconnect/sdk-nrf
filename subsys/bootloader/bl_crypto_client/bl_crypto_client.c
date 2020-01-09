/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bl_crypto.h>
#include <fw_info.h>
#include <kernel.h>

enum ext_api_index {BL_ROT_VERIFY, BL_SHA256, BL_SECP256R1, LAST};

const struct fw_info_ext_api *get_bl_crypto_ext_api(enum ext_api_index ext_api)
{
	__ASSERT(ext_api < LAST, "invalid ext_api argument.");

	static const struct fw_info_ext_api *bl_crypto_ext_apis[LAST] = {NULL};

	if (!bl_crypto_ext_apis[ext_api]) {
		switch (ext_api) {
#ifdef CONFIG_BL_ROT_VERIFY_EXT_API_REQUIRED
		case BL_ROT_VERIFY:
			bl_crypto_ext_apis[ext_api] = fw_info_ext_api_find(
					BL_ROT_VERIFY_EXT_API_ID,
					CONFIG_BL_ROT_VERIFY_EXT_API_FLAGS,
					CONFIG_BL_ROT_VERIFY_EXT_API_VER,
					CONFIG_BL_ROT_VERIFY_EXT_API_MAX_VER);
			break;
#endif
#ifdef CONFIG_BL_SHA256_EXT_API_REQUIRED
		case BL_SHA256:
			bl_crypto_ext_apis[ext_api] = fw_info_ext_api_find(
					BL_SHA256_EXT_API_ID,
					CONFIG_BL_SHA256_EXT_API_FLAGS,
					CONFIG_BL_SHA256_EXT_API_VER,
					CONFIG_BL_SHA256_EXT_API_MAX_VER);
			break;
#endif
#ifdef CONFIG_BL_SECP256R1_EXT_API_REQUIRED
		case BL_SECP256R1:
			bl_crypto_ext_apis[ext_api] = fw_info_ext_api_find(
					BL_SECP256R1_EXT_API_ID,
					CONFIG_BL_SECP256R1_EXT_API_FLAGS,
					CONFIG_BL_SECP256R1_EXT_API_VER,
					CONFIG_BL_SECP256R1_EXT_API_MAX_VER);
			break;
#endif
		default:
			k_oops();
		}
	}
	if (!bl_crypto_ext_apis[ext_api]) {
		k_oops();
	}
	return bl_crypto_ext_apis[ext_api];
}

#ifdef CONFIG_BL_ROT_VERIFY_EXT_API_REQUIRED
const struct bl_rot_verify_ext_api *get_bl_rot_verify_ext_api(void)
{
	return (const struct bl_rot_verify_ext_api *)get_bl_crypto_ext_api(
								BL_ROT_VERIFY);
}
#endif

#ifdef CONFIG_BL_SHA256_EXT_API_REQUIRED
const struct bl_sha256_ext_api *get_bl_sha256_ext_api(void)
{
	return (const struct bl_sha256_ext_api *)get_bl_crypto_ext_api(
								BL_SHA256);
}
#endif

#ifdef CONFIG_BL_SECP256R1_EXT_API_REQUIRED
const struct bl_secp256r1_ext_api *get_bl_secp256r1_ext_api(void)
{
	return (const struct bl_secp256r1_ext_api *)get_bl_crypto_ext_api(
								BL_SECP256R1);
}
#endif

#ifdef CONFIG_BL_ROT_VERIFY_EXT_API_REQUIRED
int bl_root_of_trust_verify(const u8_t *public_key, const u8_t *public_key_hash,
			 const u8_t *signature, const u8_t *firmware,
			 const u32_t firmware_len)
{
	return get_bl_rot_verify_ext_api()->ext_api.bl_root_of_trust_verify(
		public_key, public_key_hash, signature, firmware, firmware_len);
}
#endif

#ifdef CONFIG_BL_SHA256_EXT_API_REQUIRED
int bl_sha256_init(bl_sha256_ctx_t *ctx)
{
	if (sizeof(*ctx)
		< get_bl_sha256_ext_api()->ext_api.bl_sha256_ctx_size) {
		return -EFAULT;
	}
	return get_bl_sha256_ext_api()->ext_api.bl_sha256_init(ctx);
}

int bl_sha256_update(bl_sha256_ctx_t *ctx, const u8_t *data, u32_t data_len)
{
	return get_bl_sha256_ext_api()->ext_api.bl_sha256_update(ctx, data,
								data_len);
}

int bl_sha256_finalize(bl_sha256_ctx_t *ctx, u8_t *output)
{
	return get_bl_sha256_ext_api()->ext_api.bl_sha256_finalize(ctx, output);
}

int bl_sha256_verify(const u8_t *data, u32_t data_len, const u8_t *expected)
{
	return get_bl_sha256_ext_api()->ext_api.bl_sha256_verify(data, data_len,
								expected);
}
#endif

#ifdef CONFIG_BL_SECP256R1_EXT_API_REQUIRED
int bl_secp256r1_validate(const u8_t *hash, u32_t hash_len,
				const u8_t *public_key, const u8_t *signature)
{
	return get_bl_secp256r1_ext_api()->ext_api.bl_secp256r1_validate(hash,
					hash_len, public_key, signature);
}
#endif
