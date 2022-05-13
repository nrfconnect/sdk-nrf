/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include "fp_crypto.h"


int fp_crypto_aes_key_compute(uint8_t *out, const uint8_t *in)
{
	uint8_t hashed_key_buf[FP_CRYPTO_SHA256_HASH_LEN];
	int err = fp_crypto_sha256(hashed_key_buf, in, FP_CRYPTO_ECDH_SHARED_KEY_LEN);

	if (err) {
		return err;
	}
	memcpy(out, hashed_key_buf, FP_CRYPTO_AES128_BLOCK_LEN);
	return 0;
}

size_t fp_crypto_account_key_filter_size(size_t n)
{
	if (n == 0) {
		return 0;
	} else {
		return 1.2 * n + 3;
	}
}

int fp_crypto_account_key_filter(uint8_t *out,
				 const uint8_t account_key_list[][FP_CRYPTO_ACCOUNT_KEY_LEN],
				 size_t n, uint8_t salt)
{
	size_t s = fp_crypto_account_key_filter_size(n);
	uint8_t v[FP_CRYPTO_ACCOUNT_KEY_LEN + sizeof(salt)];
	uint8_t h[FP_CRYPTO_SHA256_HASH_LEN];
	uint32_t x;
	uint32_t m;

	memset(out, 0, s);
	for (size_t i = 0; i < n; i++) {
		memcpy(v, account_key_list[i], FP_CRYPTO_ACCOUNT_KEY_LEN);
		v[FP_CRYPTO_ACCOUNT_KEY_LEN] = salt;
		int err = fp_crypto_sha256(h, v, sizeof(v));

		if (err) {
			return err;
		}
		for (size_t j = 0; j < FP_CRYPTO_SHA256_HASH_LEN / sizeof(x); j++) {
			x = sys_get_be32(&h[j * sizeof(x)]);
			m = x % (s * __CHAR_BIT__);
			WRITE_BIT(out[m / __CHAR_BIT__], m % __CHAR_BIT__, 1);
		}
	}
	return 0;
}
