/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_ML_KEM_KEY_MANAGEMENT_H
#define CRACEN_ML_KEM_KEY_MANAGEMENT_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>

psa_status_t cracen_import_ml_kem_private_key(const psa_key_attributes_t *attributes,
					      const uint8_t *data, size_t data_length,
					      uint8_t *key_buffer, size_t key_buffer_size,
					      size_t *key_buffer_length, size_t *key_bits);

psa_status_t cracen_import_ml_kem_public_key(const psa_key_attributes_t *attributes,
					     const uint8_t *data, size_t data_length,
					     uint8_t *key_buffer, size_t key_buffer_size,
					     size_t *key_buffer_length, size_t *key_bits);

psa_status_t cracen_export_ml_kem_public_key(const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     uint8_t *data, size_t data_size, size_t *data_length);

psa_status_t cracen_export_ml_kem_public_key_from_keypair(const psa_key_attributes_t *attributes,
							  const uint8_t *key_buffer,
							  size_t key_buffer_size, uint8_t *data,
							  size_t data_size, size_t *data_length);

psa_status_t cracen_export_ml_kem_key(const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      uint8_t *data, size_t data_size, size_t *data_length);

#endif /* CRACEN_ML_KEM_KEY_MANAGEMENT_H */
