/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_SP800_108_CTR_MAC_H
#define CRACEN_SP800_108_CTR_MAC_H

#include <psa/crypto.h>
#include <cracen_psa_primitives.h>
#include <stdint.h>

psa_status_t cracen_sp800_108_ctr_mac_setup(cracen_key_derivation_operation_t *operation);

psa_status_t cracen_sp800_108_ctr_mac_input_bytes(cracen_key_derivation_operation_t *operation,
						  psa_key_derivation_step_t step,
						  const uint8_t *data, size_t data_length);

psa_status_t cracen_sp800_108_ctr_cmac_input_key(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step,
						 const psa_key_attributes_t *attributes,
						 const uint8_t *key_buffer, size_t key_buffer_size);

psa_status_t cracen_sp800_108_ctr_hmac_input_key(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step,
						 const psa_key_attributes_t *attributes,
						 const uint8_t *key_buffer, size_t key_buffer_size);

psa_status_t cracen_sp800_108_ctr_cmac_output_bytes(cracen_key_derivation_operation_t *operation,
						    uint8_t *output, size_t output_length);

psa_status_t cracen_sp800_108_ctr_hmac_output_bytes(cracen_key_derivation_operation_t *operation,
						    uint8_t *output, size_t output_length);

#endif /* CRACEN_SP800_108_CTR_MAC_H */
