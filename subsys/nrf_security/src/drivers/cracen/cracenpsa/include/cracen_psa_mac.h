/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_MAC_H
#define CRACEN_PSA_MAC_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Compute a MAC in a single operation.
 *
 * @param[in] attributes     Key attributes.
 * @param[in] key_buffer     Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg            MAC algorithm.
 * @param[in] input          Input data to authenticate.
 * @param[in] input_length   Length of the input data in bytes.
 * @param[out] mac           Buffer to store the MAC.
 * @param[in] mac_size       Size of the MAC buffer in bytes.
 * @param[out] mac_length    Length of the generated MAC in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The MAC buffer is too small.
 */
psa_status_t cracen_mac_compute(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *mac, size_t mac_size,
				size_t *mac_length);

/** @brief Set up a MAC signing operation.
 *
 * @param[in,out] operation     MAC operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               MAC algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_mac_sign_setup(cracen_mac_operation_t *operation,
				   const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg);

/** @brief Set up a MAC verification operation.
 *
 * @param[in,out] operation     MAC operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               MAC algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_mac_verify_setup(cracen_mac_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg);

/** @brief Add input data to a MAC operation.
 *
 * @param[in,out] operation  MAC operation context.
 * @param[in] input          Input data to add.
 * @param[in] input_length   Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_mac_update(cracen_mac_operation_t *operation, const uint8_t *input,
			       size_t input_length);

/** @brief Finish a MAC signing operation.
 *
 * @param[in,out] operation MAC operation context.
 * @param[out] mac          Buffer to store the MAC.
 * @param[in] mac_size      Size of the MAC buffer in bytes.
 * @param[out] mac_length   Length of the generated MAC in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The MAC buffer is too small.
 */
psa_status_t cracen_mac_sign_finish(cracen_mac_operation_t *operation, uint8_t *mac,
				    size_t mac_size, size_t *mac_length);

/** @brief Finish a MAC verification operation.
 *
 * @param[in,out] operation MAC operation context.
 * @param[in] mac           MAC to verify.
 * @param[in] mac_length    Length of the MAC in bytes.
 *
 * @retval PSA_SUCCESS              The MAC is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The MAC is invalid.
 * @retval PSA_ERROR_BAD_STATE      The operation is not in a valid state.
 */
psa_status_t cracen_mac_verify_finish(cracen_mac_operation_t *operation, const uint8_t *mac,
				      size_t mac_length);

/** @brief Abort a MAC operation.
 *
 * @param[in,out] operation MAC operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_mac_abort(cracen_mac_operation_t *operation);

#endif /* CRACEN_PSA_MAC_H */
