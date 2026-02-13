/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_TLS12_H
#define CRACEN_TLS12_H

#include <psa/crypto.h>
#include <cracen_psa_primitives.h>
#include <stdint.h>

psa_status_t cracen_tls12_ecjpake_to_pms_setup(cracen_key_derivation_operation_t *operation);

psa_status_t cracen_tls12_ecjpake_to_pms_input_bytes(cracen_key_derivation_operation_t *operation,
						     psa_key_derivation_step_t step,
						     const uint8_t *data, size_t data_length);

psa_status_t cracen_tls12_ecjpake_to_pms_output_bytes(cracen_key_derivation_operation_t *operation,
						      uint8_t *output, size_t output_length);

psa_status_t cracen_tls12_prf_setup(cracen_key_derivation_operation_t *operation);

psa_status_t cracen_tls12_prf_input_bytes(cracen_key_derivation_operation_t *operation,
					  psa_key_derivation_step_t step,
					  const uint8_t *data, size_t data_length);

psa_status_t cracen_tls12_prf_output_bytes(cracen_key_derivation_operation_t *operation,
					   uint8_t *output, size_t output_length);

psa_status_t cracen_tls12_psk_to_ms_setup(cracen_key_derivation_operation_t *operation);

psa_status_t cracen_tls12_psk_to_ms_input_bytes(cracen_key_derivation_operation_t *operation,
						psa_key_derivation_step_t step,
						const uint8_t *data, size_t data_length);

psa_status_t cracen_tls12_psk_to_ms_output_bytes(cracen_key_derivation_operation_t *operation,
						 uint8_t *output, size_t output_length);

#endif /* CRACEN_TLS12_H */
