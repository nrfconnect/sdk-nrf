/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_wpa3_sae
 * @{
 * @brief WPA3 SAE (Simultaneous Authentication of Equals) support for CRACEN PSA
 *        driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_PSA_WPA3_SAE_H
#define CRACEN_PSA_WPA3_SAE_H

#include <stdint.h>
#include "cracen_psa_primitives.h"

/** @brief Set up a WPA3 SAE operation.
 *
 * @param[in,out] operation      SAE operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Depends on the required PWE derivation method.
 *                              If the looping method is used, this is the
 *                              WPA3-SAE password. If the hash-to-element method
 *                              is used, this is the password token.
 * @param[in] password_length   Length of the password in bytes.
 * @param[in] cipher_suite      PAKE cipher suite.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_INVALID_ARGUMENT The algorithm in @p cipher_suite
 *         encodes an invalid hash algorithm.
 * @retval PSA_ERROR_INSUFFICIENT_MEMORY There is not enough runtime memory.
 */
psa_status_t cracen_wpa3_sae_setup(cracen_wpa3_sae_operation_t *operation,
				   const psa_key_attributes_t *attributes,
				   const uint8_t *password, size_t password_length,
				   const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Set the user identifier for a WPA3 SAE operation.
 *
 * @note This function must be called before @ref cracen_wpa3_sae_set_peer().
 *       If you are using Oberon PSA Crypto, the order is checked automatically.
 *
 * @param[in,out] operation   WPA3 SAE operation context.
 * @param[in] user_id         User identifier (6-byte MAC address).
 * @param[in] user_id_len     Length of the user identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid input argument.
 */
psa_status_t cracen_wpa3_sae_set_user(cracen_wpa3_sae_operation_t *operation,
				      const uint8_t *user_id, size_t user_id_len);

/** @brief Set the peer identifier for a WPA3 SAE operation.
 *
 * @note This function must be called after @ref cracen_wpa3_sae_set_user().
 *       If you are using Oberon PSA Crypto, the order is checked automatically.
 *
 * @param[in,out] operation   WPA3 SAE operation context.
 * @param[in] peer_id         Peer identifier (6-byte MAC address).
 * @param[in] peer_id_len     Length of the peer identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid input argument.
 */
psa_status_t cracen_wpa3_sae_set_peer(cracen_wpa3_sae_operation_t *operation,
				      const uint8_t *peer_id, size_t peer_id_len);

/** @brief Get output from a WPA3 SAE operation step.
 *
 * @param[in,out] operation      WPA3 SAE operation context.
 * @param[in] step               PAKE step.
 * @param[out] output            Buffer to store the output.
 * @param[in] output_size        Size of the output buffer in bytes.
 * @param[out] output_length     Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid input argument.
 * @retval PSA_ERROR_CORRUPTION_DETECTED A tampering attempt was detected.
 */
psa_status_t cracen_wpa3_sae_output(cracen_wpa3_sae_operation_t *operation, psa_pake_step_t step,
				    uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Provide input to a WPA3 SAE operation step.
 *
 * @param[in,out] operation  WPA3 SAE operation context.
 * @param[in] step           PAKE step.
 * @param[in] input          Input data.
 * @param[in] input_length   Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid input argument.
 * @retval PSA_ERROR_CORRUPTION_DETECTED A tampering attempt was detected.
 */
psa_status_t cracen_wpa3_sae_input(cracen_wpa3_sae_operation_t *operation, psa_pake_step_t step,
				   const uint8_t *input, size_t input_length);

/** @brief Get the shared key from a completed WPA3 SAE operation.
 *
 * @param[in,out] operation     WPA3 SAE operation context.
 * @param[in] attributes        Key attributes for the shared key.
 *                              Not used by this function; argument listed
 *                              for consistency with the PSA Crypto API.
 * @param[out] output           Buffer to store the shared key.
 * @param[in] output_size       Size of the output buffer in bytes.
 * @param[out] output_length    Length of the generated shared key in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_wpa3_sae_get_shared_key(cracen_wpa3_sae_operation_t *operation,
					    const psa_key_attributes_t *attributes,
					    uint8_t *output, size_t output_size,
					    size_t *output_length);

/** @brief Abort a WPA3 SAE operation.
 *
 * @param[in,out] operation SAE operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_wpa3_sae_abort(cracen_wpa3_sae_operation_t *operation);

/** @} */

#endif /* CRACEN_PSA_WPA3_SAE_H */
