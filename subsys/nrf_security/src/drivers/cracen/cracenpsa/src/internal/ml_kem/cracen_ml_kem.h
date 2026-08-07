/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ML-KEM key encapsulation for the CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * This is the implementation of the ML-KEM.Encaps and ML-KEM.Decaps algorithms.
 * It follows NIST FIPS 203 and uses the CRACEN hardware SHAKE128/SHAKE256 XOF algorithms.
 *
 * There is no implementation of ML-KEM.KeyGen_internal (FIPS203, algorithm 16)
 * since it is mainly used to generate "the complete" decapsulation key, which
 * consists of the decryption key of K-PKE, the encapsulation key, a hash
 * of the encapsulation key, and a random 32-byte value.
 * This is not needed since the only reason to have dk in this form is to store
 * it, which is not a required by the PSA API.
 */

#ifndef CRACEN_ML_KEM_H
#define CRACEN_ML_KEM_H

#include <psa/crypto_types.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Generate a shared secret key and its ciphertext from an encapsulation key.
 *
 * @param[in] attributes         Key attributes (must describe an ML-KEM public key).
 * @param[in] key                ML-KEM encapsulation key, encoded as byte string.
 * @param[in] key_length         Size of @p key in bytes.
 * @param[in] alg                Key encapsulation algorithm.
 * @param[in] output_attributes  Key attributes of the shared secret key to produce.
 * @param[out] output_key        Buffer where the shared secret key is to be written.
 * @param[in] output_key_size    Size of @p output_key in bytes.
 * @param[out] output_key_length Length of the shared secret key in bytes.
 * @param[out] ciphertext        Buffer where the ciphertext is to be written.
 * @param[in] ciphertext_size    Size of @p ciphertext in bytes.
 * @param[out] ciphertext_length Length of the ciphertext in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Key attributes or encapsulation key is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The size of the ciphertext buffer is too small.
 */
psa_status_t cracen_ml_kem_encapsulate(const psa_key_attributes_t *attributes, const uint8_t *key,
				       size_t key_length, psa_algorithm_t alg,
				       const psa_key_attributes_t *output_attributes,
				       uint8_t *output_key, size_t output_key_size,
				       size_t *output_key_length, uint8_t *ciphertext,
				       size_t ciphertext_size, size_t *ciphertext_length);

/** @brief Recover a shared secret key from a ciphertext using a decapsulation key.
 *
 * @param[in] attributes         Key attributes (must describe an ML-KEM key pair).
 * @param[in] key                ML-KEM decapsulation key, encoded as the (d || z) seed.
 * @param[in] key_length         Size of @p key in bytes.
 * @param[in] alg                Key encapsulation algorithm.
 * @param[in] ciphertext         Ciphertext produced by the peer.
 * @param[in] ciphertext_length  Length of @p ciphertext in bytes.
 * @param[in] output_attributes  Key attributes of the shared secret key to produce.
 * @param[out] output_key        Buffer where the shared secret key is to be written.
 * @param[in] output_key_size    Size of @p output_key in bytes.
 * @param[out] output_key_length Length of the shared secret key in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid key attributes or @p ciphertext_length.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The size of the ciphertext buffer is too small.
 */
psa_status_t cracen_ml_kem_decapsulate(const psa_key_attributes_t *attributes, const uint8_t *key,
				       size_t key_length, psa_algorithm_t alg,
				       const uint8_t *ciphertext, size_t ciphertext_length,
				       const psa_key_attributes_t *output_attributes,
				       uint8_t *output_key, size_t output_key_size,
				       size_t *output_key_length);

/** @brief Derive the encapsulation key of a key pair from its (d || z) seed.
 *
 * @param[in] key_bits   PSA key bits selecting the parameter set.
 * @param[in] seed       64-byte (d || z) seed. Only d is used.
 * @param[out] ek        Buffer where the encapsulation key is to be written.
 * @param[in] ek_size    Size of @p ek in bytes.
 * @param[out] ek_length Length of the encapsulation key in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    Unsupported algorithm.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The size of the encapsulation key buffer is too small.
 */
psa_status_t cracen_ml_kem_public_key_from_seed(size_t key_bits, const uint8_t *seed, uint8_t *ek,
					       size_t ek_size, size_t *ek_length);

#endif /* CRACEN_ML_KEM_H */
