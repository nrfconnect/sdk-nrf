/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_WPA3_SAE_H
#define CRACEN_PSA_WPA3_SAE_H

#include <stdint.h>
#include "cracen_psa_primitives.h"

psa_status_t cracen_wpa3_sae_setup(cracen_wpa3_sae_operation_t *operation,
				   const psa_key_attributes_t *attributes,
				   const uint8_t *password, size_t password_length,
				   const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t cracen_wpa3_sae_set_user(cracen_wpa3_sae_operation_t *operation,
				      const uint8_t *user_id, size_t user_id_len);

psa_status_t cracen_wpa3_sae_set_peer(cracen_wpa3_sae_operation_t *operation,
				      const uint8_t *peer_id, size_t peer_id_len);

psa_status_t cracen_wpa3_sae_output(cracen_wpa3_sae_operation_t *operation, psa_pake_step_t step,
				    uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t cracen_wpa3_sae_input(cracen_wpa3_sae_operation_t *operation, psa_pake_step_t step,
				   const uint8_t *input, size_t input_length);

psa_status_t cracen_wpa3_sae_get_shared_key(cracen_wpa3_sae_operation_t *operation,
					    const psa_key_attributes_t *attributes,
					    uint8_t *output, size_t output_size,
					    size_t *output_length);

psa_status_t cracen_wpa3_sae_abort(cracen_wpa3_sae_operation_t *operation);

#endif /* CRACEN_PSA_WPA3_SAE_H */
