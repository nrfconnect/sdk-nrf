/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_ECC_SIGNATURE_H
#define CRACEN_ECC_SIGNATURE_H

#include <psa/crypto.h>
#include <stdint.h>

psa_status_t cracen_signature_ecc_sign(bool is_message,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, uint8_t *signature,
				       size_t signature_size, size_t *signature_length);

psa_status_t cracen_signature_ecc_verify(bool is_message,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg, const uint8_t *input,
					 size_t input_length, const uint8_t *signature,
					 size_t signature_length);

#endif /* CRACEN_ECC_SIGNATURE_H */
