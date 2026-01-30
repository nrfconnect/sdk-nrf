/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_KEY_WRAP_H
#define CRACEN_PSA_KEY_WRAP_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <cracen_psa_primitives.h>

/** @brief Perform a key wrap operation.
 *
 * @param[in] wrapping_key_attributes Key attributes for the wrapping key.
 * @param[in] wrapping_key_data       Wrapping key material.
 * @param[in] wrapping_key_size       Size of the wrapping key in bytes.
 * @param[in] alg                     Key wrapping algorithm.
 * @param[in] key_attributes          Key attributes for the key that must be wrapped.
 * @param[in] key_data                Key material of the key to wrap.
 * @param[in] key_size                Size of the key that must be wrapped in bytes.
 * @param[out] data                   Buffer where the wrapped key data is to be written.
 * @param[in] data_size               Size of the data buffer in bytes.
 * @param[out] data_length            On success, the number of bytes that make up
 *                                    the wrapped key data.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT
 * @retval PSA_ERROR_NOT_SUPPORTED
 * @retval PSA_ERROR_BUFFER_TOO_SMALL
 */
psa_status_t cracen_wrap_key(const psa_key_attributes_t *wrapping_key_attributes,
			     const uint8_t *wrapping_key_data, size_t wrapping_key_size,
			     psa_algorithm_t alg,
			     const psa_key_attributes_t *key_attributes,
			     const uint8_t *key_data, size_t key_size,
			     uint8_t *data, size_t data_size, size_t *data_length);

/** @brief Perform a key unwrap operation.
 *
 * @param[in] attributes              The attributes for the new key.
 * @param[in] wrapping_key_attributes The attributes for the key to use
 *                                    for the unwrapping operation.
 * @param[in] wrapping_key_data       Wrapping key material.
 * @param[in] wrapping_key_size       Size of the wrapping key in bytes.
 * @param[in] alg                     Key wrapping algorithm.
 * @param[in] data                    Buffer containing the wrapped key data.
 *                                    The content of this buffer is unwrapped.
 * @param[in] data_length             Size of the data buffer in bytes.
 * @param[out] key                    Key material of the unwrapped key.
 * @param[in] key_size                Size of the buffer containing the unwrapped key.
 * @param[out] key_length             On success, the number of bytes that make up
 *                                    the unwrapped key data
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT
 * @retval PSA_ERROR_NOT_SUPPORTED
 * @retval PSA_ERROR_BUFFER_TOO_SMALL
 * @retval PSA_ERROR_INVALID_SIGNATURE
 */
psa_status_t cracen_unwrap_key(const psa_key_attributes_t *attributes,
			       const psa_key_attributes_t *wrapping_key_attributes,
			       const uint8_t *wrapping_key_data, size_t wrapping_key_size,
			       psa_algorithm_t alg,
			       const uint8_t *data, size_t data_length,
			       uint8_t *key, size_t key_size, size_t *key_length);

#endif /* CRACEN_PSA_KEY_WRAP_H */
