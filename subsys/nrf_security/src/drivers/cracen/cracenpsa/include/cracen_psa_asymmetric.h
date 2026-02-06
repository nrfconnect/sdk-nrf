/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_ASYMMETRIC_H
#define CRACEN_PSA_ASYMMETRIC_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Encrypt a message using an asymmetric key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Encryption algorithm.
 * @param[in] input           Plaintext to encrypt.
 * @param[in] input_length    Length of the plaintext in bytes.
 * @param[in] salt            Optional salt.
 * @param[in] salt_length     Length of the salt in bytes.
 * @param[out] output         Buffer to store the ciphertext.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the generated ciphertext in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_asymmetric_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Decrypt a message using an asymmetric key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Decryption algorithm.
 * @param[in] input           Ciphertext to decrypt.
 * @param[in] input_length    Length of the ciphertext in bytes.
 * @param[in] salt            Optional salt.
 * @param[in] salt_length     Length of the salt in bytes.
 * @param[out] output         Buffer to store the plaintext.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the decrypted plaintext in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_asymmetric_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length);

#endif /* CRACEN_PSA_ASYMMETRIC_H */
