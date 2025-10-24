/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include "cracen_psa_primitives.h"

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <cracen/statuscodes.h>
#include <silexpk/blinding.h>
#include <cracen_psa_rsa_encryption.h>

static bool is_alg_supported(psa_algorithm_t alg)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_OAEP)) {
		if (PSA_ALG_IS_RSA_OAEP(alg)) {
			return true;
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_PKCS1V15_CRYPT)) {
		if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT) {
			return true;
		}
	}

	return false;
}

static psa_status_t
cracen_asymmetric_crypt_internal(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, const uint8_t *salt, size_t salt_length,
				 uint8_t *output, size_t output_size, size_t *output_length,
				 enum cipher_operation dir)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	psa_status_t status;
	const struct sxhashalg *hashalg;
	struct cracen_rsa_key pubkey;
	struct sx_buf modulus;
	struct sx_buf exponent;

	if (!is_alg_supported(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	int sx_status = cracen_signature_get_rsa_key(
		&pubkey, dir == CRACEN_ENCRYPT,
		psa_get_key_type(attributes) == PSA_KEY_TYPE_RSA_KEY_PAIR, key_buffer,
		key_buffer_size, &modulus, &exponent);

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	struct cracen_crypt_text text = {(uint8_t *)input, input_length};

	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_OAEP)) {
		if (PSA_ALG_IS_RSA_OAEP(alg)) {
			status = hash_get_algo(PSA_ALG_RSA_OAEP_GET_HASH(alg), &hashalg);
			if (status != PSA_SUCCESS) {
				return status;
			}

			struct sx_buf label = {salt_length, (uint8_t *)salt};

			if (dir == CRACEN_ENCRYPT) {
				sx_status = cracen_rsa_oaep_encrypt(hashalg, &pubkey, &text, &label,
								    output, output_length);
			} else {
				sx_status = cracen_rsa_oaep_decrypt(hashalg, &pubkey, &text, &label,
								    output, output_length);
			}
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_PKCS1V15_CRYPT)) {
		if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT) {
			if (dir == CRACEN_ENCRYPT) {
				sx_status = cracen_rsa_pkcs1v15_encrypt(&pubkey, &text, output,
									output_length);
			} else {
				sx_status = cracen_rsa_pkcs1v15_decrypt(&pubkey, &text, output,
									output_length);
			}
		}
	}

	if (sx_status != SX_OK) {
		safe_memzero(output, output_size);
		return silex_statuscodes_to_psa(sx_status);
	}
	if (*output_length > output_size) {
		safe_memzero(output, output_size);
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	return PSA_SUCCESS;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */
}

psa_status_t cracen_asymmetric_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length)
{
	return cracen_asymmetric_crypt_internal(attributes, key_buffer, key_buffer_size, alg, input,
						input_length, salt, salt_length, output,
						output_size, output_length, CRACEN_ENCRYPT);
}

psa_status_t cracen_asymmetric_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length)
{
	return cracen_asymmetric_crypt_internal(attributes, key_buffer, key_buffer_size, alg, input,
						input_length, salt, salt_length, output,
						output_size, output_length, CRACEN_DECRYPT);
}
