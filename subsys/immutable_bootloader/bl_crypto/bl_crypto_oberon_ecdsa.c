/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <errno.h>
#include <stdbool.h>
#include <bl_crypto.h>
#include <ocrypto_ecdsa_p256.h>
#include "bl_crypto_internal.h"


int bl_secp256r1_validate(const u8_t *hash, u32_t hash_len,
			const u8_t *public_key, const u8_t *signature)
{
	if (!hash || (hash_len != CONFIG_SB_HASH_LEN) || !public_key
			|| ! signature) {
		return -EINVAL;
	}
	int retval = ocrypto_ecdsa_p256_verify_hash(signature, hash, public_key);
	if (retval == -1) {
		return -ESIGINV;
	}
	return 0;
}
