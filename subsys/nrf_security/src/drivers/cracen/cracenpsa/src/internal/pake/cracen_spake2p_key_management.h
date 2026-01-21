/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_SPAKE2P_KEY_MANAGEMENT_H
#define CRACEN_SPAKE2P_KEY_MANAGEMENT_H

#include <psa/crypto.h>
#include <stdint.h>

psa_status_t import_spake2p_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				size_t data_length, uint8_t *key_buffer,
				size_t key_buffer_size, size_t *key_buffer_length,
				size_t *key_bits);

psa_status_t cracen_derive_spake2p_key(const psa_key_attributes_t *attributes,
				       const uint8_t *input, size_t input_length,
				       uint8_t *key, size_t key_size, size_t *key_length);

psa_status_t export_spake2p_public_key_from_keypair(const psa_key_attributes_t *attributes,
						    const uint8_t *priv_key,
						    size_t priv_key_length, uint8_t *pub_key,
						    size_t pub_key_size,
						    size_t *pub_key_length);

#endif /* CRACEN_SPAKE2P_KEY_MANAGEMENT_H */
