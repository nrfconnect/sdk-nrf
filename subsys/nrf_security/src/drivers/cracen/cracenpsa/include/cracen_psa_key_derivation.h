/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_KEY_DERIVATION_H
#define CRACEN_PSA_KEY_DERIVATION_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Perform a key agreement operation.
 *
 * @param[in] attributes     Key attributes for the private key.
 * @param[in] priv_key       Private key material.
 * @param[in] priv_key_size  Size of the private key in bytes.
 * @param[in] publ_key       Public key material.
 * @param[in] publ_key_size  Size of the public key in bytes.
 * @param[out] output        Buffer to store the shared secret.
 * @param[in] output_size    Size of the output buffer in bytes.
 * @param[out] output_length Length of the generated shared secret in bytes.
 * @param[in] alg            Key agreement algorithm.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_key_agreement(const psa_key_attributes_t *attributes, const uint8_t *priv_key,
				  size_t priv_key_size, const uint8_t *publ_key,
				  size_t publ_key_size, uint8_t *output, size_t output_size,
				  size_t *output_length, psa_algorithm_t alg);

/** @brief Set up a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] alg           Key derivation algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_key_derivation_setup(cracen_key_derivation_operation_t *operation,
					 psa_algorithm_t alg);

/** @brief Set the capacity for a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] capacity      Maximum capacity in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_set_capacity(cracen_key_derivation_operation_t *operation,
						size_t capacity);

/** @brief Provide a key as input to a key derivation operation.
 *
 * @param[in,out] operation     Key derivation operation context.
 * @param[in] step              Derivation step.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_input_key(cracen_key_derivation_operation_t *operation,
					     psa_key_derivation_step_t step,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size);

/** @brief Provide bytes as input to a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] step          Derivation step.
 * @param[in] data          Input data.
 * @param[in] data_length   Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_input_bytes(cracen_key_derivation_operation_t *operation,
					       psa_key_derivation_step_t step, const uint8_t *data,
					       size_t data_length);

/** @brief Provide an integer as input to a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] step          Derivation step.
 * @param[in] value         Integer value.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_input_integer(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step, uint64_t value);

/** @brief Read output from a key derivation operation.
 *
 * @param[in,out] operation    Key derivation operation context.
 * @param[out] output          Buffer to store the output.
 * @param[in] output_length    Length of the output to read in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_output_bytes(cracen_key_derivation_operation_t *operation,
						uint8_t *output, size_t output_length);

/** @brief Abort a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_key_derivation_abort(cracen_key_derivation_operation_t *operation);

#endif /* CRACEN_PSA_KEY_DERIVATION_H */
