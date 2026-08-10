/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_key_encapsulation.h>

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
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_decapsulate(const psa_key_attributes_t *attributes,
				const uint8_t *key, size_t key_length,
				psa_algorithm_t alg,
				const uint8_t *ciphertext, size_t ciphertext_length,
				const psa_key_attributes_t *output_attributes,
				uint8_t *output_key, size_t output_key_size,
				size_t *output_key_length)
{
	return PSA_ERROR_NOT_SUPPORTED;
}
