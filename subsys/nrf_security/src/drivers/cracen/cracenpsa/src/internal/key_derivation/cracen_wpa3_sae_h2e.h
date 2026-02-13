/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_WPA3_SAE_H2E_H
#define CRACEN_WPA3_SAE_H2E_H

#include <psa/crypto.h>
#include <cracen_psa_primitives.h>
#include <stdint.h>

psa_status_t cracen_wpa3_sae_h2e_setup(cracen_key_derivation_operation_t *operation);

psa_status_t cracen_wpa3_sae_h2e_input_bytes(cracen_key_derivation_operation_t *operation,
					     psa_key_derivation_step_t step, const uint8_t *data,
					     size_t data_length);

psa_status_t cracen_wpa3_sae_h2e_output_bytes(cracen_key_derivation_operation_t *operation,
					      uint8_t *output, size_t output_length);

psa_status_t cracen_wpa3_sae_h2e_abort(cracen_key_derivation_operation_t *operation);

#endif /* CRACEN_WPA3_SAE_H2E_H */
