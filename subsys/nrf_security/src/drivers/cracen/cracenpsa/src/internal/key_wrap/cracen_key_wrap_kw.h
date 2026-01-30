/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_KEY_WRAP_KW_H
#define CRACEN_KEY_WRAP_KW_H

#include <stddef.h>
#include <stdint.h>
#include <cracen/common.h>
#include <silexpk/core.h>

/** @brief Perform a key wrap operation using AES-KW algorithm.
 *
 * @param[in] wrapping_key_attributes Key attributes for the wrapping key.
 * @param[in] wrapping_key_data       Wrapping key material.
 * @param[in] wrapping_key_size       Size of the wrapping key in bytes.
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
psa_status_t cracen_key_wrap_kw_wrap(const psa_key_attributes_t *wrapping_key_attributes,
				     const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				     const uint8_t *key_data, size_t key_size,
				     uint8_t *data, size_t data_size, size_t *data_length);

/** @brief Perform a key unwrap operation using AES-KW algorithm.
 *
 * @param[in] wrapping_key_attributes The attributes for the key to use
 *                                    for the unwrapping operation.
 * @param[in] wrapping_key_data       Wrapping key material.
 * @param[in] wrapping_key_size       Size of the wrapping key in bytes.
 * @param[in] data                    Buffer containing the wrapped key data.
 *                                    The content of this buffer is unwrapped.
 * @param[in] data_size               Size of the data buffer in bytes.
 * @param[out] key_data               Key material of the unwrapped key.
 * @param[in] key_data_size           Size of the buffer containing the unwrapped key.
 * @param[out] key_data_length        On success, the number of bytes that make up
 *                                    the unwrapped key data
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT
 * @retval PSA_ERROR_NOT_SUPPORTED
 * @retval PSA_ERROR_BUFFER_TOO_SMALL
 * @retval PSA_ERROR_INVALID_SIGNATURE
 */
psa_status_t cracen_key_wrap_kw_unwrap(const psa_key_attributes_t *wrapping_key_attributes,
				       const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				       const uint8_t *data, size_t data_size,
				       uint8_t *key_data, size_t key_data_size,
				       size_t *key_data_length);

#endif /* CRACEN_KEY_WRAP_KW_H */
