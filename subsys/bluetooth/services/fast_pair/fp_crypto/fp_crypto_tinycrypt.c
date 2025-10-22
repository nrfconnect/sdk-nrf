/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/hmac.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/ecc_dh.h>
#include "fp_crypto.h"

int fp_crypto_sha256(uint8_t *out, const uint8_t *in, size_t data_len)
{
	struct tc_sha256_state_struct s;

	if (tc_sha256_init(&s) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	if (tc_sha256_update(&s, in, data_len) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	if (tc_sha256_final(out, &s) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

int fp_crypto_hmac_sha256(uint8_t *out,
			  const uint8_t *in,
			  size_t data_len,
			  const uint8_t *hmac_key,
			  size_t hmac_key_len)
{
	struct tc_hmac_state_struct s;

	if (tc_hmac_set_key(&s, hmac_key, hmac_key_len) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	if (tc_hmac_init(&s) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	if (tc_hmac_update(&s, in, data_len) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	if (tc_hmac_final(out, FP_CRYPTO_SHA256_HASH_LEN, &s) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

int fp_crypto_aes128_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	struct tc_aes_key_sched_struct s;

	if (tc_aes128_set_encrypt_key(&s, k) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	if (tc_aes_encrypt(out, in, &s) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

int fp_crypto_aes128_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	struct tc_aes_key_sched_struct s;

	if (tc_aes128_set_decrypt_key(&s, k) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	if (tc_aes_decrypt(out, in, &s) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}

int fp_crypto_ecdh_shared_secret(uint8_t *secret_key, const uint8_t *public_key,
				 const uint8_t *private_key)
{
	const struct uECC_Curve_t *curve = uECC_secp256r1();

	if (uECC_valid_public_key(public_key, curve) != 0) {
		return -EINVAL;
	}
	if (uECC_shared_secret(public_key, private_key, secret_key, curve) != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}
	return 0;
}
