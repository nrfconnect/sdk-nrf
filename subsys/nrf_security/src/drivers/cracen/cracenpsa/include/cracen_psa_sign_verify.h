/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_SIGN_VERIFY_H
#define CRACEN_PSA_SIGN_VERIFY_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Sign a message.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Signing algorithm.
 * @param[in] input           Message to sign.
 * @param[in] input_length    Length of the message in bytes.
 * @param[out] signature      Buffer to store the signature.
 * @param[in] signature_size  Size of the signature buffer in bytes.
 * @param[out] signature_length Length of the generated signature in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, uint8_t *signature, size_t signature_size,
				 size_t *signature_length);

/** @brief Verify a message signature.
 *
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               Verification algorithm.
 * @param[in] input             Message that was signed.
 * @param[in] input_length      Length of the message in bytes.
 * @param[in] signature         Signature to verify.
 * @param[in] signature_length  Length of the signature in bytes.
 *
 * @retval PSA_SUCCESS              The signature is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The signature is invalid.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_verify_message(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   const uint8_t *signature, size_t signature_length);

/** @brief Sign a hash.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Signing algorithm.
 * @param[in] hash            Hash of a message to sign.
 * @param[in] hash_length     Length of the hash in bytes.
 * @param[out] signature      Buffer to store the signature.
 * @param[in] signature_size  Size of the signature buffer in bytes.
 * @param[out] signature_length Length of the generated signature in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
			      size_t hash_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length);

/** @brief Verify the signature of a hash.
 *
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               Verification algorithm.
 * @param[in] hash              Hash of the message that was signed.
 * @param[in] hash_length       Length of the hash in bytes.
 * @param[in] signature         Signature to verify.
 * @param[in] signature_length  Length of the signature in bytes.
 *
 * @retval PSA_SUCCESS              The signature is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The signature is invalid.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
				size_t hash_length, const uint8_t *signature,
				size_t signature_length);

#endif /* CRACEN_PSA_SIGN_VERIFY_H */
