/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_SPAKE2P_H
#define CRACEN_PSA_SPAKE2P_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Set up a SPAKE2+ operation.
 *
 * @param[in,out] operation     SPAKE2+ operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 * @param[in] cipher_suite      PAKE cipher suite.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_spake2p_setup(cracen_spake2p_operation_t *operation,
				  const psa_key_attributes_t *attributes, const uint8_t *password,
				  size_t password_length,
				  const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Set the password key for a SPAKE2+ operation.
 *
 * @param[in,out] operation     SPAKE2+ operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_password_key(cracen_spake2p_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *password, size_t password_length);

/** @brief Set the user identifier for a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 * @param[in] user_id       User identifier.
 * @param[in] user_id_len   Length of the user identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_user(cracen_spake2p_operation_t *operation, const uint8_t *user_id,
				     size_t user_id_len);

/** @brief Set the peer identifier for a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 * @param[in] peer_id       Peer identifier.
 * @param[in] peer_id_len   Length of the peer identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_peer(cracen_spake2p_operation_t *operation, const uint8_t *peer_id,
				     size_t peer_id_len);

/** @brief Set the context for a SPAKE2+ operation.
 *
 * @param[in,out] operation    SPAKE2+ operation context.
 * @param[in] context          Context data.
 * @param[in] context_length   Length of the context data in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_context(cracen_spake2p_operation_t *operation,
					const uint8_t *context, size_t context_length);

/** @brief Set the role for a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 * @param[in] role         PAKE role.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_role(cracen_spake2p_operation_t *operation, psa_pake_role_t role);

/** @brief Get output from a SPAKE2+ operation step.
 *
 * @param[in,out] operation      SPAKE2+ operation context.
 * @param[in] step               PAKE step.
 * @param[out] output            Buffer to store the output.
 * @param[in] output_size        Size of the output buffer in bytes.
 * @param[out] output_length     Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_spake2p_output(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				   uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Provide input to a SPAKE2+ operation step.
 *
 * @param[in,out] operation   SPAKE2+ operation context.
 * @param[in] step            PAKE step.
 * @param[in] input           Input data.
 * @param[in] input_length    Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_spake2p_input(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				  const uint8_t *input, size_t input_length);

/** @brief Get the shared key from a completed SPAKE2+ operation.
 *
 * @param[in,out] operation     SPAKE2+ operation context.
 * @param[in] attributes        Key attributes for the shared key.
 * @param[out] output           Buffer to store the shared key.
 * @param[in] output_size       Size of the output buffer in bytes.
 * @param[out] output_length    Length of the generated shared key in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_spake2p_get_shared_key(cracen_spake2p_operation_t *operation,
					   const psa_key_attributes_t *attributes, uint8_t *output,
					   size_t output_size, size_t *output_length);

/** @brief Abort a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_abort(cracen_spake2p_operation_t *operation);

#endif /* CRACEN_PSA_SPAKE2P_H */
