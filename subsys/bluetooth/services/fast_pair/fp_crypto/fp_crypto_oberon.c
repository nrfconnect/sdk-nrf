/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "fp_crypto.h"

#include <ocrypto_hmac_sha256.h>
#include <ocrypto_sha256.h>
#include <ocrypto_aes_ecb.h>
#include <ocrypto_ecdh_p256.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_crypto, CONFIG_FP_CRYPTO_LOG_LEVEL);

int fp_crypto_sha256(uint8_t *out, const uint8_t *in, size_t data_len)
{
	ocrypto_sha256(out, in, data_len);

	return 0;
}

int fp_crypto_hmac_sha256(uint8_t *out,
			  const uint8_t *in,
			  size_t data_len,
			  const uint8_t *hmac_key,
			  size_t hmac_key_len)
{
	ocrypto_hmac_sha256(out, hmac_key, hmac_key_len, in, data_len);

	return 0;
}

int fp_crypto_aes128_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	ocrypto_aes_ecb_encrypt(out, in, FP_CRYPTO_AES128_BLOCK_LEN, k, FP_CRYPTO_AES128_KEY_LEN);

	return 0;
}

int fp_crypto_aes128_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	ocrypto_aes_ecb_decrypt(out, in, FP_CRYPTO_AES128_BLOCK_LEN, k, FP_CRYPTO_AES128_KEY_LEN);

	return 0;
}

int fp_crypto_ecdh_shared_secret(uint8_t *secret_key,
				 const uint8_t *public_key,
				 const uint8_t *private_key)
{
	int ret;

	ret = ocrypto_ecdh_p256_common_secret(secret_key, private_key, public_key);
	if (ret) {
		LOG_ERR("ecdh: ocrypto_ecdh_p256_common_secret failed:"
			" invalid private or public key");
		return ret;
	}

	return 0;
}
