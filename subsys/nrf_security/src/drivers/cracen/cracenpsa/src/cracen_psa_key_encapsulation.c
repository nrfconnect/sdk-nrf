/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_key_encapsulation.h>
#include <internal/ml_kem/cracen_ml_kem.h>

#include <psa/crypto_extra.h>
#include <zephyr/sys/util.h>

psa_status_t cracen_encapsulate(const psa_key_attributes_t *attributes,
				const uint8_t *key, size_t key_length,
				psa_algorithm_t alg,
				const psa_key_attributes_t *output_attributes,
				uint8_t *output_key, size_t output_key_size,
				size_t *output_key_length,
				uint8_t *ciphertext, size_t ciphertext_size,
				size_t *ciphertext_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (IS_ENABLED(PSA_NEED_CRACEN_ML_KEM) && alg == PSA_ALG_ML_KEM) {
		status = cracen_ml_kem_encapsulate(attributes, key, key_length, alg,
						   output_attributes,
						   output_key, output_key_size, output_key_length,
						   ciphertext, ciphertext_size, ciphertext_length);
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	return status;
}

psa_status_t cracen_decapsulate(const psa_key_attributes_t *attributes,
				const uint8_t *key, size_t key_length,
				psa_algorithm_t alg,
				const uint8_t *ciphertext, size_t ciphertext_length,
				const psa_key_attributes_t *output_attributes,
				uint8_t *output_key, size_t output_key_size,
				size_t *output_key_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (IS_ENABLED(PSA_NEED_CRACEN_ML_KEM) && alg == PSA_ALG_ML_KEM) {
		status = cracen_ml_kem_decapsulate(attributes, key, key_length, alg,
						   ciphertext, ciphertext_length,
						   output_attributes,
						   output_key, output_key_size, output_key_length);
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	return status;
}
