/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file cracen_sw_aead.h
 *
 * @defgroup cracen_sw_aead Software AEAD dispatcher
 * @{
 * @brief Dispatcher layer for the software AEAD implementations.
 *
 * This module provides a common interface that dispatches to specific
 * software AEAD implementations (such as AES-CCM) as a workaround for
 * hardware limitations with multipart operations.
 */

#ifndef CRACEN_SW_AEAD_H
#define CRACEN_SW_AEAD_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <stdint.h>
#include "cracen_psa_primitives.h"

/**
 * @brief Set up the software AEAD encryption operation.
 *
 * @param[in,out] operation       Pointer to the AEAD operation structure.
 * @param[in]     attributes      Key attributes.
 * @param[in]     key_buffer      Key material.
 * @param[in]     key_buffer_size Size of the key material.
 * @param[in]     alg             AEAD algorithm to use.
 *
 * @retval PSA_SUCCESS Operation setup successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid key size.
 */
psa_status_t cracen_aead_encrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

/**
 * @brief Set up the software AEAD decryption operation.
 *
 * @param[in,out] operation       Pointer to the AEAD operation structure.
 * @param[in]     attributes      Key attributes.
 * @param[in]     key_buffer      Key material.
 * @param[in]     key_buffer_size Size of the key material.
 * @param[in]     alg             AEAD algorithm to use.
 *
 * @retval PSA_SUCCESS Operation setup successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid key size.
 */
psa_status_t cracen_aead_decrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

/**
 * @brief Set nonce for the software AEAD operation.
 *
 * @param[in,out] operation    Pointer to the AEAD operation structure.
 * @param[in]     nonce        Nonce/IV value.
 * @param[in]     nonce_length Length of the nonce.
 *
 * @retval PSA_SUCCESS Nonce set successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid nonce length.
 */
psa_status_t cracen_aead_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
				   size_t nonce_length);

/**
 * @brief Set lengths for the software AEAD operation.
 *
 * Some AEAD algorithms (such as CCM) require lengths to be known up-front.
 *
 * @param[in,out] operation        Pointer to the AEAD operation structure.
 * @param[in]     ad_length        Length of additional data.
 * @param[in]     plaintext_length Length of plaintext/ciphertext.
 *
 * @retval PSA_SUCCESS Lengths set successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 */
psa_status_t cracen_aead_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
				     size_t plaintext_length);

/**
 * @brief Update the software AEAD operation with additional data.
 *
 * @param[in,out] operation    Pointer to the AEAD operation structure.
 * @param[in]     input        Additional data.
 * @param[in]     input_length Length of additional data.
 *
 * @retval PSA_SUCCESS Additional data processed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_BAD_STATE AD already finalized or operation not initialized.
 */
psa_status_t cracen_aead_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
				   size_t input_length);

/**
 * @brief Update the software AEAD operation with plaintext/ciphertext.
 *
 * @param[in,out] operation     Pointer to the AEAD operation structure.
 * @param[in]     input         Input data (plaintext for encrypt, ciphertext for decrypt).
 * @param[in]     input_length  Length of input data.
 * @param[out]    output        Output buffer.
 * @param[in]     output_size   Size of output buffer.
 * @param[out]    output_length Pointer to store actual output length.
 *
 * @retval PSA_SUCCESS Data processed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Output buffer too small.
 * @retval PSA_ERROR_BAD_STATE Operation not initialized.
 */
psa_status_t cracen_aead_update(cracen_aead_operation_t *operation, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length);

/**
 * @brief Finish the software AEAD encryption and generate the tag.
 *
 * @param[in,out] operation         Pointer to the AEAD operation structure.
 * @param[out]    ciphertext        Final ciphertext output buffer.
 * @param[in]     ciphertext_size   Size of ciphertext buffer.
 * @param[out]    ciphertext_length Pointer to store actual ciphertext length.
 * @param[out]    tag               Tag output buffer.
 * @param[in]     tag_size          Size of tag buffer.
 * @param[out]    tag_length        Pointer to store actual tag length.
 *
 * @retval PSA_SUCCESS Encryption finished successfully; tag generated.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Tag buffer too small.
 * @retval PSA_ERROR_BAD_STATE Operation not initialized.
 */
psa_status_t cracen_aead_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				size_t tag_size, size_t *tag_length);

/**
 * @brief Finish the software AEAD decryption and verify the tag.
 *
 * @param[in,out] operation        Pointer to the AEAD operation structure.
 * @param[out]    plaintext        Final plaintext output buffer.
 * @param[in]     plaintext_size   Size of plaintext buffer.
 * @param[out]    plaintext_length Pointer to store actual plaintext length.
 * @param[in]     tag              Tag to verify.
 * @param[in]     tag_length       Length of the tag.
 *
 * @retval PSA_SUCCESS Decryption finished successfully; tag verified.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_INVALID_SIGNATURE Tag verification failed.
 * @retval PSA_ERROR_BAD_STATE Operation not initialized.
 */
psa_status_t cracen_aead_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
				size_t tag_length);

/**
 * @brief Abort the software AEAD operation.
 *
 * Clears the operation context and any sensitive data.
 *
 * @param[in,out] operation Pointer to the AEAD operation structure.
 *
 * @retval PSA_SUCCESS Operation aborted successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 */
psa_status_t cracen_aead_abort(cracen_aead_operation_t *operation);

/**
 * @brief Perform a single software AEAD encryption.
 *
 * @param[in]  attributes              Key attributes.
 * @param[in]  key_buffer              Key material.
 * @param[in]  key_buffer_size         Size of key material.
 * @param[in]  alg                     AEAD algorithm.
 * @param[in]  nonce                   Nonce/IV.
 * @param[in]  nonce_length            Length of nonce.
 * @param[in]  additional_data         Additional data.
 * @param[in]  additional_data_length  Length of additional data.
 * @param[in]  plaintext               Plaintext to encrypt.
 * @param[in]  plaintext_length        Length of plaintext.
 * @param[out] ciphertext              Output buffer for ciphertext and tag.
 * @param[in]  ciphertext_size         Size of ciphertext buffer.
 * @param[out] ciphertext_length       Pointer to store actual output length.
 *
 * @retval PSA_SUCCESS Encryption completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Output buffer too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid parameters.
 */
psa_status_t cracen_aead_encrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *plaintext,
				 size_t plaintext_length, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length);

/**
 * @brief Perform a single software AEAD decryption.
 *
 * @param[in]  attributes              Key attributes.
 * @param[in]  key_buffer              Key material.
 * @param[in]  key_buffer_size         Size of key material.
 * @param[in]  alg                     AEAD algorithm.
 * @param[in]  nonce                   Nonce/IV.
 * @param[in]  nonce_length            Length of nonce.
 * @param[in]  additional_data         Additional data.
 * @param[in]  additional_data_length  Length of additional data.
 * @param[in]  ciphertext              Ciphertext and tag to decrypt.
 * @param[in]  ciphertext_length       Length of ciphertext (including tag).
 * @param[out] plaintext               Output buffer for plaintext.
 * @param[in]  plaintext_size          Size of plaintext buffer.
 * @param[out] plaintext_length        Pointer to store actual plaintext length.
 *
 * @retval PSA_SUCCESS Decryption and verification completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm.
 * @retval PSA_ERROR_INVALID_SIGNATURE Tag verification failed.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Output buffer too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid parameters.
 */
psa_status_t cracen_aead_decrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *ciphertext,
				 size_t ciphertext_length, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length);

/**
 * @}
 */

#endif /* CRACEN_SW_AEAD_H */
