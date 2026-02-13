/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PBKDF2_HMAC_H
#define CRACEN_PBKDF2_HMAC_H

#include <psa/crypto.h>
#include <cracen_psa_primitives.h>
#include <stdint.h>

psa_status_t cracen_pbkdf2_hmac_setup(cracen_key_derivation_operation_t *operation);

psa_status_t cracen_pbkdf2_hmac_input_bytes(cracen_key_derivation_operation_t *operation,
					    psa_key_derivation_step_t step, const uint8_t *data,
					    size_t data_length);

psa_status_t cracen_pbkdf2_hmac_input_integer(cracen_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step, uint64_t value);

psa_status_t cracen_pbkdf2_hmac_output_bytes(cracen_key_derivation_operation_t *operation,
					     uint8_t *output, size_t output_length);

#endif /* CRACEN_PBKDF2_HMAC_H */
