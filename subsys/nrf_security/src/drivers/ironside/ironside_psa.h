/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IRONSIDE_PSA_H
#define IRONSIDE_PSA_H

/**
 * @file ironside_psa.h
 * @brief IronSide PSA crypto driver interface
 *
 * This driver is to be implemented by IronSide firmware for these purposes:
 *
 * - Leverage Mbed TLS' built-in key concept to support additional keys,
 *   with implementation-defined properties, in the PSA_KEY_ID_VENDOR range.
 * - Hijack the key creation functions of the PSA Crypto API to control
 *   provisioning of such keys at different product life cycles.
 * - Capture the above functionality in its own driver, independent of other
 *   crypto accelerators, for portability to future IronSide firmware variants.
 */

#include <psa/crypto.h>

/* The following header must be provided externally and with these types:
 * - ironside_psa_pake_operation_t
 */
#include "ironside_psa_types.h"

psa_status_t ironside_psa_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				       psa_drv_slot_number_t *slot_number);

psa_status_t ironside_psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				     size_t data_length, uint8_t *key_buffer,
				     size_t key_buffer_size, size_t *key_buffer_length,
				     size_t *bits);

psa_status_t ironside_psa_get_key_buffer_size(const psa_key_attributes_t *attributes,
					      size_t *key_buffer_size);

psa_status_t ironside_psa_generate_key(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				       size_t key_buffer_size, size_t *key_buffer_length);

psa_status_t ironside_psa_get_builtin_key(psa_drv_slot_number_t slot_number,
					  psa_key_attributes_t *attributes, uint8_t *key_buffer,
					  size_t key_buffer_size, size_t *key_buffer_length);

psa_status_t ironside_psa_copy_key(psa_key_attributes_t *attributes, const uint8_t *source_key,
				   size_t source_key_length, uint8_t *target_key_buffer,
				   size_t target_key_buffer_size, size_t *target_key_buffer_length);

psa_status_t ironside_psa_derive_key(const psa_key_attributes_t *attributes, const uint8_t *input,
				     size_t input_length, uint8_t *key_buffer,
				     size_t key_buffer_size, size_t *key_buffer_length);

psa_status_t ironside_psa_destroy_builtin_key(const psa_key_attributes_t *attributes);

psa_status_t ironside_psa_key_agreement(const psa_key_attributes_t *attributes,
					const uint8_t *priv_key, size_t priv_key_size,
					psa_algorithm_t alg, const uint8_t *publ_key,
					size_t publ_key_size, uint8_t *output, size_t output_size,
					size_t *output_length);

psa_status_t ironside_psa_key_encapsulate(const psa_key_attributes_t *attributes,
					  const uint8_t *key, size_t key_length,
					  psa_algorithm_t alg,
					  const psa_key_attributes_t *output_attributes,
					  uint8_t *output_key, size_t output_key_size,
					  size_t *output_key_length, uint8_t *ciphertext,
					  size_t ciphertext_size, size_t *ciphertext_length);

psa_status_t ironside_psa_key_decapsulate(const psa_key_attributes_t *attributes,
					  const uint8_t *key, size_t key_length,
					  psa_algorithm_t alg, const uint8_t *ciphertext,
					  size_t ciphertext_length,
					  const psa_key_attributes_t *output_attributes,
					  uint8_t *output_key, size_t output_key_size,
					  size_t *output_key_length);

psa_status_t ironside_psa_pake_setup(ironside_psa_pake_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *password, size_t password_length,
				     const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t ironside_psa_pake_set_role(ironside_psa_pake_operation_t *operation,
					psa_pake_role_t role);

psa_status_t ironside_psa_pake_set_user(ironside_psa_pake_operation_t *operation,
					const uint8_t *user_id, size_t user_id_length);

psa_status_t ironside_psa_pake_set_peer(ironside_psa_pake_operation_t *operation,
					const uint8_t *peer_id, size_t peer_id_length);

psa_status_t ironside_psa_pake_set_context(ironside_psa_pake_operation_t *operation,
					   const uint8_t *context, size_t context_length);

psa_status_t ironside_psa_pake_output(ironside_psa_pake_operation_t *operation,
				      psa_pake_step_t step, uint8_t *output, size_t output_size,
				      size_t *output_length);

psa_status_t ironside_psa_pake_input(ironside_psa_pake_operation_t *operation, psa_pake_step_t step,
				     const uint8_t *input, size_t input_length);

psa_status_t ironside_psa_pake_get_shared_key(ironside_psa_pake_operation_t *operation,
					      const psa_key_attributes_t *attributes,
					      uint8_t *key_buffer, size_t key_buffer_size,
					      size_t *key_buffer_length);

psa_status_t ironside_psa_pake_abort(ironside_psa_pake_operation_t *operation);

#endif /* IRONSIDE_PSA_H */
