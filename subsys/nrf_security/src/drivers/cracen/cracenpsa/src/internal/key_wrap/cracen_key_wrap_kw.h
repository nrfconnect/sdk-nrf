/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_KEY_WRAP_KW_H
#define CRACEN_KEY_WRAP_KW_H

#include <stddef.h>
#include <stdint.h>
#include <cracen/common.h>
#include <silexpk/core.h>

psa_status_t cracen_key_wrap_kw_wrap(const psa_key_attributes_t *wrapping_key_attributes,
				     const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				     const uint8_t *key_data, size_t key_size,
				     uint8_t *data, size_t data_size, size_t *data_length);

psa_status_t cracen_key_wrap_kw_unwrap(const psa_key_attributes_t *wrapping_key_attributes,
				       const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				       const uint8_t *data, size_t data_size,
				       uint8_t *key_data, size_t key_data_size,
				       size_t *key_data_length);

#endif /* CRACEN_KEY_WRAP_KW_H */
