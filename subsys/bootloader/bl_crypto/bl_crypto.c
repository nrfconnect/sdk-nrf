/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <bl_crypto.h>
#include <fw_info.h>
#include <assert.h>
#include <ocrypto_constant_time.h>
#include "bl_crypto_internal.h"


__weak int crypto_init_signing(void)
{
	return 0;
}

__weak int crypto_init_hash(void)
{
	return 0;
}

int bl_crypto_init(void)
{
	int retval = crypto_init_signing();
	if (retval) {
		return retval;
	}

	return crypto_init_hash();
}

BUILD_ASSERT_MSG(CONFIG_SB_PUBLIC_KEY_HASH_LEN <= CONFIG_SB_HASH_LEN,
		"Invalid value for SB_PUBLIC_KEY_HASH_LEN.");

static int verify_truncated_hash(const u8_t *data, u32_t data_len,
		const u8_t *expected, u32_t hash_len, bool external)
{
	u8_t hash[CONFIG_SB_HASH_LEN];

	__ASSERT(hash_len <= CONFIG_SB_HASH_LEN, "truncated hash length too long.");

	int retval = get_hash(hash, data, data_len, external);
	if (retval != 0) {
		return retval;
	}
	if (!ocrypto_constant_time_equal(expected, hash, hash_len)) {
		return -EHASHINV;
	}
	return 0;
}

static int verify_signature(const u8_t *data, u32_t data_len,
		const u8_t *signature, const u8_t *public_key, bool external)
{
	u8_t hash1[CONFIG_SB_HASH_LEN];
	u8_t hash2[CONFIG_SB_HASH_LEN];

	int retval = get_hash(hash1, data, data_len, external);
	if (retval != 0) {
		return retval;
	}

	retval = get_hash(hash2, hash1, CONFIG_SB_HASH_LEN, external);
	if (retval != 0) {
		return retval;
	}

	return bl_secp256r1_validate(hash2, CONFIG_SB_HASH_LEN, public_key, signature);
}

/* Base implementation, with 'external' parameter. */
static int root_of_trust_verify(
		const u8_t *public_key, const u8_t *public_key_hash,
		const u8_t *signature, const u8_t *firmware,
		const u32_t firmware_len, bool external)
{
	__ASSERT(public_key && public_key_hash && signature && firmware,
			"A parameter was NULL.");
	int retval = verify_truncated_hash(public_key, CONFIG_SB_PUBLIC_KEY_LEN,
			public_key_hash, CONFIG_SB_PUBLIC_KEY_HASH_LEN, external);

	if (retval != 0) {
		return retval;
	}

	return verify_signature(firmware, firmware_len, signature, public_key,
			external);
}


/* For use by the bootloader. */
int bl_root_of_trust_verify(const u8_t *public_key, const u8_t *public_key_hash,
			 const u8_t *signature, const u8_t *firmware,
			 const u32_t firmware_len)
{
	return root_of_trust_verify(public_key, public_key_hash, signature,
					firmware, firmware_len, false);
}


/* For use through EXT_API. */
int bl_root_of_trust_verify_external(
			const u8_t *public_key, const u8_t *public_key_hash,
			const u8_t *signature, const u8_t *firmware,
			const u32_t firmware_len)
{
	return root_of_trust_verify(public_key, public_key_hash, signature,
					firmware, firmware_len, true);
}

int bl_sha256_verify(const u8_t *data, u32_t data_len, const u8_t *expected)
{
	return verify_truncated_hash(data, data_len, expected, CONFIG_SB_HASH_LEN, true);
}

#ifdef CONFIG_BL_ROT_VERIFY_EXT_API_ENABLED
EXT_API(struct bl_rot_verify_ext_api, bl_rot_verify_ext_api) = {
	.header = FW_INFO_EXT_API_INIT(BL_ROT_VERIFY_EXT_API_ID,
				CONFIG_BL_ROT_VERIFY_EXT_API_FLAGS,
				CONFIG_BL_ROT_VERIFY_EXT_API_VER,
				sizeof(struct bl_rot_verify_ext_api)),
	.ext_api = {
		.bl_root_of_trust_verify = bl_root_of_trust_verify_external,
	}
};
#endif

#ifdef CONFIG_BL_SHA256_EXT_API_ENABLED
EXT_API(struct bl_sha256_ext_api, bl_sha256_ext_api) = {
	.header = FW_INFO_EXT_API_INIT(BL_SHA256_EXT_API_ID,
				CONFIG_BL_SHA256_EXT_API_FLAGS,
				CONFIG_BL_SHA256_EXT_API_VER,
				sizeof(struct bl_sha256_ext_api)),
	.ext_api = {
		.bl_sha256_init = bl_sha256_init,
		.bl_sha256_update = bl_sha256_update,
		.bl_sha256_finalize = bl_sha256_finalize,
		.bl_sha256_verify = bl_sha256_verify,
		.bl_sha256_ctx_size = SHA256_CTX_SIZE,
	},
};
#endif

#ifdef CONFIG_BL_SECP256R1_EXT_API_ENABLED
EXT_API(struct bl_secp256r1_ext_api, bl_secp256r1_ext_api) = {
	.header = FW_INFO_EXT_API_INIT(BL_SECP256R1_EXT_API_ID,
				CONFIG_BL_SECP256R1_EXT_API_FLAGS,
				CONFIG_BL_SECP256R1_EXT_API_VER,
				sizeof(struct bl_secp256r1_ext_api)),
	.ext_api = {
		.bl_secp256r1_validate = bl_secp256r1_validate,
	},
};
#endif
