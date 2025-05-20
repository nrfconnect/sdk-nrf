/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include "psa_crypto_driver_wrappers.h"
#include <stdint.h>
#include <string.h>

#include "aead_crypt.h"

#define CHACHA20_KEY_SIZE 32

psa_status_t trusted_storage_aead_init(void)
{
	return psa_crypto_init();
}

static psa_status_t trusted_storage_aead_psa_crypt(psa_key_usage_t key_usage, const void *key_buf,
						   size_t key_len, const void *nonce_buf,
						   size_t nonce_len, const void *add_buf,
						   size_t add_len, const void *input_buf,
						   size_t input_len, void *output_buf,
						   size_t output_size, size_t *output_len)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (key_len < CHACHA20_KEY_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_set_key_usage_flags(&key_attributes, key_usage);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CHACHA20_POLY1305);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(CHACHA20_KEY_SIZE));

	/* Here we cannot use the PSA APIs since this storage solution provides implementation
	 * of the PSA ITS APIs. Since the PSA crypto is using the PSA ITS APIs for persistent keys
	 * if we use the PSA APIs here they will corrupt the PSA keystore (psa_global_data_t) which
	 * holds all the active keys in the PSA core.
	 */
	if (key_usage == PSA_KEY_USAGE_ENCRYPT) {
		status = psa_driver_wrapper_aead_encrypt(
			&key_attributes, key_buf, key_len, PSA_ALG_CHACHA20_POLY1305, nonce_buf,
			nonce_len, add_buf, add_len, input_buf, input_len, output_buf, output_size,
			output_len);
	} else {
		status = psa_driver_wrapper_aead_decrypt(
			&key_attributes, key_buf, key_len, PSA_ALG_CHACHA20_POLY1305, nonce_buf,
			nonce_len, add_buf, add_len, input_buf, input_len, output_buf, output_size,
			output_len);
	}

	return status;
}

psa_status_t trusted_storage_aead_encrypt(const void *key_buf, size_t key_len,
					  const void *nonce_buf, size_t nonce_len,
					  const void *add_buf, size_t add_len,
					  const void *input_buf, size_t input_len, void *output_buf,
					  size_t output_size, size_t *output_len)
{
	return trusted_storage_aead_psa_crypt(PSA_KEY_USAGE_ENCRYPT, key_buf, key_len, nonce_buf,
					      nonce_len, add_buf, add_len, input_buf, input_len,
					      output_buf, output_size, output_len);
}

psa_status_t trusted_storage_aead_decrypt(const void *key_buf, size_t key_len,
					  const void *nonce_buf, size_t nonce_len,
					  const void *add_buf, size_t add_len,
					  const void *input_buf, size_t input_len, void *output_buf,
					  size_t output_size, size_t *output_len)
{
	return trusted_storage_aead_psa_crypt(PSA_KEY_USAGE_DECRYPT, key_buf, key_len, nonce_buf,
					      nonce_len, add_buf, add_len, input_buf, input_len,
					      output_buf, output_size, output_len);
}
