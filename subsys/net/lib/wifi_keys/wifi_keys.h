/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_KEYS_H
#define WIFI_KEYS_H

#include <psa/crypto.h>

static const psa_key_location_t PSA_KEY_LOCATION_WIFI_KEYS =
	(psa_key_location_t)(PSA_KEY_LOCATION_VENDOR_FLAG | ('N' << 8) | 'W');

typedef enum {
	PEER_UCST_ENC = PSA_KEY_TYPE_VENDOR_FLAG,
	PEER_UCST_MIC,
	PEER_BCST_ENC,
	PEER_BCST_MIC,
	VIF_ENC,
	VIF_MIC
} wifi_keys_key_type_t;

psa_key_attributes_t wifi_keys_key_attributes_init(wifi_keys_key_type_t type, uint32_t db_id,
						   uint32_t key_index);

psa_status_t wifi_keys_import_key(const psa_key_attributes_t *attr, const uint8_t *data,
				  size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
				  size_t *key_buffer_length, size_t *key_bits);

uint32_t wifi_keys_get_key_size_in_bytes(wifi_keys_key_type_t type);

uint32_t wifi_keys_get_key_size_in_bits(wifi_keys_key_type_t type);

#endif
