/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <occ_ecdsa_p256.h>
#include "bl_crypto_internal.h"

bool verify_signature(const u8_t *data, u32_t data_len, const u8_t *signature,
		const u8_t *public_key)
{
	u8_t hash1[CONFIG_SB_HASH_LEN];
	u8_t hash2[CONFIG_SB_HASH_LEN];

	if (!get_hash(hash1, data, data_len)) {
		return false;
	}

	if (!get_hash(hash2, hash1, CONFIG_SB_HASH_LEN)) {
		return false;
	}

	int retval = occ_ecdsa_p256_verify_hash(signature, hash2, public_key);

	return (retval == 0);
}
