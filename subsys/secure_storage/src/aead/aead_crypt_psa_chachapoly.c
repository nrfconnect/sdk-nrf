/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <stdint.h>
#include <string.h>

#include "aead_crypt.h"

#define CHACHA20_KEY_SIZE 32

psa_status_t secure_storage_aead_init(void)
{
	return psa_crypto_init();
}

size_t secure_storage_aead_get_encrypted_size(size_t data_size)
{
	return PSA_AEAD_ENCRYPT_OUTPUT_SIZE(PSA_KEY_TYPE_CHACHA20, PSA_ALG_CHACHA20_POLY1305,
					    data_size);
}

static psa_status_t secure_storage_aead_psa_crypt(psa_key_usage_t key_usage, const void *key_buf,
						  size_t key_len, const void *nonce_buf,
						  size_t nonce_len, const void *add_buf,
						  size_t add_len, const void *input_buf,
						  size_t input_len, void *output_buf,
						  size_t output_size, size_t *output_len)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_handle;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (key_len < CHACHA20_KEY_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_set_key_usage_flags(&key_attributes, key_usage);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CHACHA20_POLY1305);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_CHACHA20);
	psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(CHACHA20_KEY_SIZE));

	status = psa_import_key(&key_attributes, key_buf, CHACHA20_KEY_SIZE, &key_handle);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (key_usage == PSA_KEY_USAGE_ENCRYPT) {
		status = psa_aead_encrypt(key_handle, PSA_ALG_CHACHA20_POLY1305, nonce_buf,
					  nonce_len, add_buf, add_len, input_buf, input_len,
					  output_buf, output_size, output_len);
	} else {
		status = psa_aead_decrypt(key_handle, PSA_ALG_CHACHA20_POLY1305, nonce_buf,
					  nonce_len, add_buf, add_len, input_buf, input_len,
					  output_buf, output_size, output_len);
	}

	psa_destroy_key(key_handle);

	return status;
}

psa_status_t secure_storage_aead_encrypt(const void *key_buf, size_t key_len, const void *nonce_buf,
					 size_t nonce_len, const void *add_buf, size_t add_len,
					 const void *input_buf, size_t input_len, void *output_buf,
					 size_t output_size, size_t *output_len)
{
	return secure_storage_aead_psa_crypt(PSA_KEY_USAGE_ENCRYPT, key_buf, key_len, nonce_buf,
					     nonce_len, add_buf, add_len, input_buf, input_len,
					     output_buf, output_size, output_len);
}

psa_status_t secure_storage_aead_decrypt(const void *key_buf, size_t key_len, const void *nonce_buf,
					 size_t nonce_len, const void *add_buf, size_t add_len,
					 const void *input_buf, size_t input_len, void *output_buf,
					 size_t output_size, size_t *output_len)
{
	return secure_storage_aead_psa_crypt(PSA_KEY_USAGE_DECRYPT, key_buf, key_len, nonce_buf,
					     nonce_len, add_buf, add_len, input_buf, input_len,
					     output_buf, output_size, output_len);
}
