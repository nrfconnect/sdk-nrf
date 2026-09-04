/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_psa_driver_api
 * @{
 * @brief Key encapsulation and decapsulation functions for the CRACEN PSA driver.
 */

#ifndef CRACEN_PSA_KEY_ENCAPSULATION_H
#define CRACEN_PSA_KEY_ENCAPSULATION_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <cracen_psa_primitives.h>

/** @brief Use a public key to generate a new shared secret key and associated ciphertext.
 *
 * @param[in] attributes         Key attributes of the encapsulation (public) key.
 * @param[in] key                Key material of the encapsulation key.
 * @param[in] key_length         Size of the encapsulation key in bytes.
 * @param[in] alg                Key encapsulation algorithm.
 * @param[in] output_attributes  Key attributes of the shared secret key to produce.
 * @param[out] output_key        Buffer where the shared secret key is to be written.
 * @param[in] output_key_size    Size of the output key buffer in bytes.
 * @param[out] output_key_length On success, the number of bytes that make up
 *                               the shared secret key.
 * @param[out] ciphertext        Buffer where the ciphertext is to be written. The peer
 *                               needs it to decapsulate the same shared secret key.
 * @param[in] ciphertext_size    Size of the ciphertext buffer in bytes.
 * @param[out] ciphertext_length On success, the number of bytes that make up the ciphertext.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    The algorithm or the key type is not supported.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid parameters.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_encapsulate(const psa_key_attributes_t *attributes,
				const uint8_t *key, size_t key_length,
				psa_algorithm_t alg,
				const psa_key_attributes_t *output_attributes,
				uint8_t *output_key, size_t output_key_size,
				size_t *output_key_length,
				uint8_t *ciphertext, size_t ciphertext_size,
				size_t *ciphertext_length);

/** @brief Use a private key to decapsulate a shared secret key from a ciphertext.
 *
 * @note ML-KEM uses implicit rejection: an invalid ciphertext does not fail the
 *       operation, it yields a shared secret key that differs from the one the
 *       peer encapsulated.
 *
 * @param[in] attributes         Key attributes of the decapsulation (private) key.
 * @param[in] key                Key material of the decapsulation key.
 * @param[in] key_length         Size of the decapsulation key in bytes.
 * @param[in] alg                Key encapsulation algorithm.
 * @param[in] ciphertext         Buffer containing the ciphertext produced by the peer.
 * @param[in] ciphertext_length  Size of the ciphertext in bytes.
 * @param[in] output_attributes  Key attributes of the shared secret key to produce.
 * @param[out] output_key        Buffer where the shared secret key is to be written.
 * @param[in] output_key_size    Size of the output key buffer in bytes.
 * @param[out] output_key_length On success, the number of bytes that make up
 *                               the shared secret key.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED    The algorithm or the key type is not supported.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid parameters.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_decapsulate(const psa_key_attributes_t *attributes,
				const uint8_t *key, size_t key_length,
				psa_algorithm_t alg,
				const uint8_t *ciphertext, size_t ciphertext_length,
				const psa_key_attributes_t *output_attributes,
				uint8_t *output_key, size_t output_key_size,
				size_t *output_key_length);

/** @} */

#endif /* CRACEN_PSA_KEY_ENCAPSULATION_H */
