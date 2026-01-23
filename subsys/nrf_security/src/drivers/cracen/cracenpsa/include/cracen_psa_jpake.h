/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_JPAKE_H
#define CRACEN_PSA_JPAKE_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Set up a JPAKE operation.
 *
 * @param[in,out] operation     JPAKE operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 * @param[in] cipher_suite      PAKE cipher suite.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_jpake_setup(cracen_jpake_operation_t *operation,
				const psa_key_attributes_t *attributes, const uint8_t *password,
				size_t password_length,
				const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Set the password key for a JPAKE operation.
 *
 * @param[in,out] operation     JPAKE operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_password_key(cracen_jpake_operation_t *operation,
					   const psa_key_attributes_t *attributes,
					   const uint8_t *password, size_t password_length);

/** @brief Set the user identifier for a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 * @param[in] user_id       User identifier.
 * @param[in] user_id_len   Length of the user identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_user(cracen_jpake_operation_t *operation, const uint8_t *user_id,
				   size_t user_id_len);

/** @brief Set the peer identifier for a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 * @param[in] peer_id       Peer identifier.
 * @param[in] peer_id_len   Length of the peer identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_peer(cracen_jpake_operation_t *operation, const uint8_t *peer_id,
				   size_t peer_id_len);

/** @brief Set the role for a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 * @param[in] role          PAKE role.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_role(cracen_jpake_operation_t *operation, psa_pake_role_t role);

/** @brief Get output from a JPAKE operation step.
 *
 * @param[in,out] operation      JPAKE operation context.
 * @param[in] step               PAKE step.
 * @param[out] output            Buffer to store the output.
 * @param[in] output_size        Size of the output buffer in bytes.
 * @param[out] output_length     Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_jpake_output(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				 uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Provide input to a JPAKE operation step.
 *
 * @param[in,out] operation   JPAKE operation context.
 * @param[in] step            PAKE step.
 * @param[in] input           Input data.
 * @param[in] input_length    Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_jpake_input(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				const uint8_t *input, size_t input_length);

/** @brief Get the shared key from a completed JPAKE operation.
 *
 * @param[in,out] operation     JPAKE operation context.
 * @param[in] attributes        Key attributes for the shared key.
 * @param[out] output           Buffer to store the shared key.
 * @param[in] output_size       Size of the output buffer in bytes.
 * @param[out] output_length    Length of the generated shared key in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_jpake_get_shared_key(cracen_jpake_operation_t *operation,
					 const psa_key_attributes_t *attributes, uint8_t *output,
					 size_t output_size, size_t *output_length);

/** @brief Abort a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_abort(cracen_jpake_operation_t *operation);

#endif /* CRACEN_PSA_JPAKE_H */
