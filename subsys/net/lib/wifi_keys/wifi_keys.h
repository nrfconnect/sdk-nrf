/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_KEYS_H
#define WIFI_KEYS_H

#include <stdint.h>
#include <psa/crypto.h>
#include "kmu_push.h"

static const uint32_t WIFI_KEYS_ADDR_INVALID = ~0;

typedef enum {
	PEER_UCST_ENC,
	PEER_UCST_MIC,
	PEER_BCST_ENC,
	PEER_BCST_MIC,
	VIF_ENC,
	VIF_MIC
} wifi_keys_type_t;

uint32_t wifi_keys_get_key_start_addr(wifi_keys_type_t type, uint32_t db_id, uint32_t key_index);

uint32_t wifi_keys_get_key_size_in_bytes(wifi_keys_type_t type);

uint32_t wifi_keys_get_key_size_in_bits(wifi_keys_type_t type);

psa_key_attributes_t wifi_keys_key_attributes_init(uint32_t key_length_bytes);

int wifi_keys_reverse_byte_order(void *restrict dst, const void *restrict src,
				 uint32_t key_length_bytes);

int wifi_keys_reverse_byte_order_in_place(void *buf, uint32_t key_length_bytes);

#endif
