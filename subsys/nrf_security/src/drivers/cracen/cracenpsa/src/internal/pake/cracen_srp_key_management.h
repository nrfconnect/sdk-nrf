/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_SRP_KEY_MANAGEMENT_H
#define CRACEN_SRP_KEY_MANAGEMENT_H

#include <psa/crypto.h>
#include <stdint.h>

psa_status_t import_srp_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			    size_t *key_buffer_length, size_t *key_bits);

#endif /* CRACEN_SRP_KEY_MANAGEMENT_H */
