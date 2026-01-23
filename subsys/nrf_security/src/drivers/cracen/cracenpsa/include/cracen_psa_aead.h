/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_AEAD_H
#define CRACEN_PSA_AEAD_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

/** @brief Encrypt and authenticate a message using AEAD.
 *
 * @param[in] attributes             Key attributes.
 * @param[in] key_buffer             Key material buffer.
 * @param[in] key_buffer_size        Size of the key buffer in bytes.
 * @param[in] alg                    AEAD algorithm.
 * @param[in] nonce                  Nonce or IV.
 * @param[in] nonce_length           Length of the nonce in bytes.
 * @param[in] additional_data        Additional Authenticated Data (AAD).
 * @param[in] additional_data_length Length of Additional Authenticated Data (AAD) in bytes.
 * @param[in] plaintext              Plaintext to encrypt.
 * @param[in] plaintext_length       Length of the plaintext in bytes.
 * @param[out] ciphertext            Buffer to store the ciphertext and tag.
 * @param[in] ciphertext_size        Size of the ciphertext buffer in bytes.
 * @param[out] ciphertext_length     Length of the generated ciphertext in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE   The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED    The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The ciphertext buffer is too small.
 */
psa_status_t cracen_aead_encrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *plaintext,
				 size_t plaintext_length, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length);

/** @brief Authenticate and decrypt a message using AEAD.
 *
 * @param[in] attributes             Key attributes.
 * @param[in] key_buffer             Key material buffer.
 * @param[in] key_buffer_size        Size of the key buffer in bytes.
 * @param[in] alg                    AEAD algorithm.
 * @param[in] nonce                  Nonce or IV.
 * @param[in] nonce_length           Length of the nonce in bytes.
 * @param[in] additional_data        Additional Authenticated Data (AAD).
 * @param[in] additional_data_length Length of Additional Authenticated Data (AAD) in bytes.
 * @param[in] ciphertext             Ciphertext and tag to decrypt.
 * @param[in] ciphertext_length      Length of the ciphertext in bytes.
 * @param[out] plaintext             Buffer to store the plaintext.
 * @param[in] plaintext_size         Size of the plaintext buffer in bytes.
 * @param[out] plaintext_length      Length of the decrypted plaintext in bytes.
 *
 * @retval PSA_SUCCESS                 The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE    The key handle is invalid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The authentication tag is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED     The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL  The plaintext buffer is too small.
 */
psa_status_t cracen_aead_decrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *ciphertext,
				 size_t ciphertext_length, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length);

/** @brief Set up an AEAD encryption operation.
 *
 * @param[in,out] operation     AEAD operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               AEAD algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_aead_encrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

/** @brief Set up an AEAD decryption operation.
 *
 * @param[in,out] operation     AEAD operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               AEAD algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_aead_decrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

/** @brief Set the nonce for an AEAD operation.
 *
 * @param[in,out] operation   AEAD operation context.
 * @param[in] nonce           Nonce or IV.
 * @param[in] nonce_length    Length of the nonce in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_aead_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
				   size_t nonce_length);

/** @brief Set the lengths for an AEAD operation.
 *
 * @param[in,out] operation      AEAD operation context.
 * @param[in] ad_length          Length of Additional Authenticated Data (AAD) in bytes.
 * @param[in] plaintext_length   Length of the plaintext in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_aead_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
				     size_t plaintext_length);

/** @brief Add Additional Authenticated Data (AAD) to an AEAD operation.
 *
 * @param[in,out] operation  AEAD operation context.
 * @param[in] input          Additional Authenticated Data (AAD).
 * @param[in] input_length   Length of the Additional Authenticated Data (AAD) in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_aead_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
				   size_t input_length);

/** @brief Process input data in an AEAD operation.
 *
 * @param[in,out] operation   AEAD operation context.
 * @param[in] input           Input data to process.
 * @param[in] input_length    Length of the input data in bytes.
 * @param[out] output         Buffer to store the output.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_aead_update(cracen_aead_operation_t *operation, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length);

/** @brief Finish an AEAD encryption operation.
 *
 * @param[in,out] operation       AEAD operation context.
 * @param[out] ciphertext         Buffer to store the final ciphertext.
 * @param[in] ciphertext_size     Size of the ciphertext buffer in bytes.
 * @param[out] ciphertext_length  Length of the generated ciphertext in bytes.
 * @param[out] tag                Buffer to store the authentication tag.
 * @param[in] tag_size            Size of the tag buffer in bytes.
 * @param[out] tag_length         Length of the generated tag in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL A buffer is too small.
 */
psa_status_t cracen_aead_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				size_t tag_size, size_t *tag_length);

/** @brief Finish an AEAD decryption operation and verify the tag.
 *
 * @param[in,out] operation      AEAD operation context.
 * @param[out] plaintext         Buffer to store the final plaintext.
 * @param[in] plaintext_size     Size of the plaintext buffer in bytes.
 * @param[out] plaintext_length  Length of the generated plaintext in bytes.
 * @param[in] tag                Authentication tag to verify.
 * @param[in] tag_length         Length of the tag in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully and the tag is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The authentication tag is invalid.
 * @retval PSA_ERROR_BAD_STATE      The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The plaintext buffer is too small.
 */
psa_status_t cracen_aead_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
				size_t tag_length);

/** @brief Abort an AEAD operation.
 *
 * @param[in,out] operation AEAD operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_aead_abort(cracen_aead_operation_t *operation);

#endif /* CRACEN_PSA_AEAD_H */
