/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_RSA_KEY_MANAGEMENT_H
#define CRACEN_RSA_KEY_MANAGEMENT_H

#include <psa/crypto.h>
#include <stdint.h>

psa_status_t export_rsa_public_key_from_keypair(const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer,
						size_t key_buffer_size, uint8_t *data,
						size_t data_size, size_t *data_length);

psa_status_t generate_rsa_private_key(const psa_key_attributes_t *attributes, uint8_t *key,
				      size_t key_size, size_t *key_length);

psa_status_t import_rsa_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			    size_t *key_buffer_length, size_t *key_bits);

psa_status_t rsa_export_public_key(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   uint8_t *data, size_t data_size, size_t *data_length);

#endif /* CRACEN_RSA_KEY_MANAGEMENT_H */
