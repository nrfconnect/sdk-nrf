/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdint.h>
#include <psa/crypto.h>
#include <psa_crypto_driver_wrappers.h>

psa_status_t psa_verify_message_with_context_builtin(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length,
	const uint8_t *context,	size_t context_length,	const uint8_t *signature,
	size_t signature_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t hash[PSA_HASH_MAX_SIZE] = {0};
	size_t hash_length = 0;

	if (!PSA_ALG_IS_SIGN_HASH(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = psa_driver_wrapper_hash_compute(PSA_ALG_SIGN_GET_HASH(alg), input, input_length,
						 hash, PSA_HASH_MAX_SIZE, &hash_length);
	if (status != PSA_SUCCESS) {
		return status;
	}


	return psa_driver_wrapper_verify_hash_with_context(attributes, key_buffer, key_buffer_size,
							   alg, hash, hash_length,
							   context, context_length,
							   signature, signature_length);
}
