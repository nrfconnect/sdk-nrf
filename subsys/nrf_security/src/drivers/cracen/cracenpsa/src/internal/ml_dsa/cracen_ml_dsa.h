/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ML-DSA signing and signature verification for the CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * This is the implementation of the ML-DSA.Sign and ML-DSA.Verify algorithms.
 * It follows NIST FIPS 204 and uses the CRACEN hardware SHAKE128/SHAKE256 XOF algorithms.
 */

#ifndef CRACEN_ML_DSA_H
#define CRACEN_ML_DSA_H

#include <psa/crypto_types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Verify a pure ML-DSA or HashML-DSA signature over a message or hash.
 *
 * @param[in] is_message        True if @p input is a message, false if it is a hash.
 * @param[in] attributes        Key attributes (must describe an ML-DSA public key).
 * @param[in] key_buffer        ML-DSA public key, encoded as byte string.
 * @param[in] key_buffer_size   Size of @p key_buffer in bytes.
 * @param[in] alg               Asymmetric signature algorithm.
 * @param[in] input             Message that was signed.
 * @param[in] input_length      Length of @p input in bytes.
 * @param[in] context           Context string (may be NULL when empty).
 * @param[in] context_length    Length of @p context in bytes (must be < 256).
 * @param[in] signature         Signature to verify.
 * @param[in] signature_length  Length of @p signature in bytes.
 *
 * @retval PSA_SUCCESS                 The signature is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The signature is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED     Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT  Key attributes, context or signature length is invalid.
 */
psa_status_t cracen_ml_dsa_verify(bool is_message,
				  const psa_key_attributes_t *attributes,
				  const uint8_t *key_buffer, size_t key_buffer_size,
				  psa_algorithm_t alg, const uint8_t *input,
				  size_t input_length, const uint8_t *context,
				  size_t context_length, const uint8_t *signature,
				  size_t signature_length);

/** @brief Produce a pure ML-DSA or HashML-DSA signature over a message or hash.
 *
 * @param[in] is_message        True if @p input is a message, false if it is a hash.
 * @param[in] attributes        Key attributes (must describe an ML-DSA key pair).
 * @param[in] key_buffer        ML-DSA key-pair seed (32 bytes).
 * @param[in] key_buffer_size   Size of @p key_buffer in bytes.
 * @param[in] alg               Asymmetric signature algorithm.
 * @param[in] input             Message (or hash) to be signed.
 * @param[in] input_length      Length of @p input in bytes.
 * @param[in] context           Context string (may be NULL when empty).
 * @param[in] context_length    Length of @p context in bytes (must be < 256).
 * @param[out] signature        Buffer to store the signature.
 * @param[in] signature_size    Size of @p signature in bytes.
 * @param[out] signature_length Length of the produced signature in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Key attributes or context length is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The size of the signature buffer is too small.
 */
psa_status_t cracen_ml_dsa_sign(bool is_message,
				const psa_key_attributes_t *attributes,
				const uint8_t *key_buffer, size_t key_buffer_size,
				psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *context,
				size_t context_length, uint8_t *signature,
				size_t signature_size, size_t *signature_length);

/** @brief Derive and export the ML-DSA public key from a key-pair seed.
 *
 * @param[in] attributes        Key attributes (must describe an ML-DSA key pair).
 * @param[in] key_buffer        ML-DSA key-pair seed (32 bytes).
 * @param[in] key_buffer_size   Size of @p key_buffer in bytes.
 * @param[out] data             Buffer to store the encoded public key.
 * @param[in] data_size         Size of @p data in bytes.
 * @param[out] data_length      Length of the exported public key in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Key attributes or key-pair seed size is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The size of the public key buffer is too small.
 */
psa_status_t cracen_ml_dsa_export_public_key_from_seed(const psa_key_attributes_t *attributes,
						       const uint8_t *key_buffer,
						       size_t key_buffer_size, uint8_t *data,
						       size_t data_size, size_t *data_length);

#endif /* CRACEN_ML_DSA_H */
