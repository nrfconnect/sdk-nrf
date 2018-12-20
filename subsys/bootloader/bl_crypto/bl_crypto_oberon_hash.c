/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <occ_sha256.h>
#include <occ_constant_time.h>
#include <stddef.h>
#include <stdbool.h>

bool get_hash(u8_t *hash, const u8_t *data, u32_t data_len)
{
	occ_sha256(hash, data, data_len);

	/* Return true always as occ_sha256 does not have a return value. */
	return true;
}

bool verify_truncated_hash(const u8_t *data, u32_t data_len,
			   const u8_t *expected, u32_t hash_len)
{
	u8_t hash[CONFIG_SB_HASH_LEN];

	if (hash_len > CONFIG_SB_HASH_LEN) {
		return false;
	}

	if (!get_hash(hash, data, data_len)) {
		return false;
	}

	return occ_constant_time_equal(expected, hash, hash_len);
}

bool verify_hash(const u8_t *data, u32_t data_len, const u8_t *expected)
{
	return verify_truncated_hash(data, data_len, expected,
				     CONFIG_SB_HASH_LEN);
}
