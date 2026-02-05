/**
 * @file
 *
 * @copyright Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_sign_verify.h>

#include <cracen/common.h>
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <silexpk/blinding.h>
#include <string.h>

#include <internal/ikg/cracen_ikg_operations.h>
#include <internal/ecc/cracen_ecc_signature.h>
#include <internal/rsa/cracen_rsa_signature.h>

#define CRACEN_IS_MESSAGE      (1)
#define CRACEN_IS_HASH	       (0)
#define CRACEN_EXTRACT_PUBKEY  (1)
#define CRACEN_EXTRACT_PRIVKEY (0)

psa_status_t cracen_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, uint8_t *signature, size_t signature_size,
				 size_t *signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC) &&
	    PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		return cracen_signature_ecc_sign(
			CRACEN_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg,
			input, input_length, signature, signature_size, signature_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA) &&
	    PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
		return cracen_signature_rsa_sign(
			CRACEN_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg,
			input, input_length, signature, signature_size, signature_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
			      size_t hash_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC) &&
	    PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		return cracen_signature_ecc_sign(
			CRACEN_IS_HASH, attributes, key_buffer, key_buffer_size, alg, hash,
			hash_length, signature, signature_size, signature_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA) &&
	    PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
		return cracen_signature_rsa_sign(
			CRACEN_IS_HASH, attributes, key_buffer, key_buffer_size, alg, hash,
			hash_length, signature, signature_size, signature_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_verify_message(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   const uint8_t *signature, size_t signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC) &&
	    PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		return cracen_signature_ecc_verify(
			CRACEN_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg,
			input, input_length, signature, signature_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA) &&
	    PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
		return cracen_signature_rsa_verify(
			CRACEN_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg,
			input, input_length, signature, signature_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
				size_t hash_length, const uint8_t *signature,
				size_t signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC) &&
	    PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		return cracen_signature_ecc_verify(CRACEN_IS_HASH, attributes, key_buffer,
						   key_buffer_size, alg, hash, hash_length,
						   signature, signature_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA) &&
	    PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
		return cracen_signature_rsa_verify(CRACEN_IS_HASH, attributes, key_buffer,
						   key_buffer_size, alg, hash, hash_length,
						   signature, signature_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
