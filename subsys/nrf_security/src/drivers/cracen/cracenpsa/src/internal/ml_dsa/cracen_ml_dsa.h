/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_psa_driver_api
 * @{
 * @brief ML-DSA signature verification for the CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * This is the implementation of ML-DSA.Verify that follows NIST FIPS 204,
 * using the CRACEN hardware SHAKE128/SHAKE256 XOFs.
 * Only signature verification is implemented; key generation and signing are not supported now.
 */

#ifndef CRACEN_PSA_ML_DSA_H
#define CRACEN_PSA_ML_DSA_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Verify a pure ML-DSA signature over a message or hash.
 *
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
 * @retval PSA_SUCCESS
 * @retval PSA_ERROR_INVALID_SIGNATURE
 * @retval PSA_ERROR_NOT_SUPPORTED
 * @retval PSA_ERROR_INVALID_ARGUMENT
 */
psa_status_t cracen_ml_dsa_verify(bool is_message,
				  const psa_key_attributes_t *attributes,
				  const uint8_t *key_buffer, size_t key_buffer_size,
				  psa_algorithm_t alg, const uint8_t *input,
				  size_t input_length, const uint8_t *context,
				  size_t context_length, const uint8_t *signature,
				  size_t signature_length);

/** @} */

#endif /* CRACEN_PSA_ML_DSA_H */
